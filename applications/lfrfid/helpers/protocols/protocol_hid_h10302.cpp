#include "protocol_hid_h10302.h"
#include <furi.h>

typedef uint32_t HID10302CardData;
constexpr uint8_t HID10302Count = 3;
constexpr uint8_t HID10302BitSize = sizeof(HID10302CardData) * 8;

static void write_raw_bit(bool bit, uint8_t position, HID10302CardData* card_data) {
    if(bit) {
        card_data[position / HID10302BitSize] |=
            1UL << (HID10302BitSize - (position % HID10302BitSize) - 1);
    } else {
        card_data[position / HID10302BitSize] &=
            ~(1UL << (HID10302BitSize - (position % HID10302BitSize) - 1));
    }
}

static void write_bit(bool bit, uint8_t position, HID10302CardData* card_data) {
    write_raw_bit(bit, position + 0, card_data);
    write_raw_bit(!bit, position + 1, card_data);
}

uint8_t ProtocolHID10302::get_encoded_data_size() {
    return sizeof(HID10302CardData) * HID10302Count;
}

uint8_t ProtocolHID10302::get_decoded_data_size() {
    return 5;
}

void ProtocolHID10302::encode(
    const uint8_t* decoded_data,
    const uint8_t decoded_data_size,
    uint8_t* encoded_data,
    const uint8_t encoded_data_size) {
    furi_check(decoded_data_size >= get_decoded_data_size());
    furi_check(encoded_data_size >= get_encoded_data_size());

    HID10302CardData card_data[HID10302Count] = {0, 0, 0};

    // pack the data needed for parity into a single value
    uint64_t fc_cn = (((uint64_t) decoded_data[0]) << 27)
        | (((uint64_t) decoded_data[1]) << 19)
        | (((uint64_t) decoded_data[2]) << 16)
        | (((uint64_t) decoded_data[3]) << 8)
        | (((uint64_t) decoded_data[4]) << 0)
        ;

    // even parity sum calculation
    uint8_t even_parity_sum = 0;
    for(int8_t i = 18; i < 37; i++) {
        if(((fc_cn >> i) & 1) == 1) {
            even_parity_sum++;
        }
    }

    // odd parity sum calculation
    uint8_t odd_parity_sum = 1;
    for(int8_t i = 0; i < 18; i++) {
        if(((fc_cn >> i) & 1) == 1) {
            odd_parity_sum++;
        }
    }

    // 0x1D preamble
    write_raw_bit(0, 0, card_data);
    write_raw_bit(0, 1, card_data);
    write_raw_bit(0, 2, card_data);
    write_raw_bit(1, 3, card_data);
    write_raw_bit(1, 4, card_data);
    write_raw_bit(1, 5, card_data);
    write_raw_bit(0, 6, card_data);
    write_raw_bit(1, 7, card_data);

    // company / OEM code
    write_bit(0, 8, card_data);
    write_bit(0, 10, card_data);
    write_bit(0, 12, card_data);
    write_bit(0, 14, card_data);
    write_bit(0, 16, card_data);
    write_bit(0, 18, card_data);
    write_bit(0, 20, card_data);

    // even parity bit
    write_bit((even_parity_sum % 2), 22, card_data);

    // data
    for(uint8_t i = 0; i < 35; i++) {
        write_bit((fc_cn >> (34 - i)) & 1, 24 + (i * 2), card_data);
    }

    // odd parity bit
    write_bit((odd_parity_sum % 2), 94, card_data);

    memcpy(encoded_data, &card_data, get_encoded_data_size());
}

static uint16_t machester_decode(uint32_t encoded) {
    uint16_t decoded = 0;
    for (int i = 0; i < 16; i++) {
        decoded >>= 1;
        if ((encoded & 0b11) == 0b10) {
            decoded |= (1 << 15);
        }
        encoded >>= 2;
    }
    return decoded;
}

void ProtocolHID10302::decode(
    const uint8_t* encoded_data,
    const uint8_t encoded_data_size,
    uint8_t* decoded_data,
    const uint8_t decoded_data_size) {
    furi_check(decoded_data_size >= get_decoded_data_size());
    furi_check(encoded_data_size >= get_encoded_data_size());

    const HID10302CardData* card_data = reinterpret_cast<const HID10302CardData*>(encoded_data);

    // data decoding
    uint64_t result = 0;
    result |= machester_decode(card_data[0]);
    result <<= 16;
    result |= machester_decode(card_data[1]);
    result <<= 16;
    result |= machester_decode(card_data[2]);

    // format output
    uint16_t facility_code = (result >> 20) & ((1 << 16) - 1);
    uint32_t card_number = (result >> 1) & ((1 << 19) - 1);
    uint8_t data[5] = {
        (uint8_t)(facility_code >> 8), 
        (uint8_t)(facility_code >> 0), 
        (uint8_t)(card_number >> 16), 
        (uint8_t)(card_number >> 8), 
        (uint8_t)(card_number >> 0)
    };

    memcpy(decoded_data, &data, get_decoded_data_size());
}

bool ProtocolHID10302::can_be_decoded(const uint8_t* encoded_data, const uint8_t encoded_data_size) {
    furi_check(encoded_data_size >= get_encoded_data_size());

    const HID10302CardData* card_data = reinterpret_cast<const HID10302CardData*>(encoded_data);

    // packet preamble
    // raw data
    if((card_data[0] >> 24 & ((1 << 8) - 1)) != 0x1D) {
        return false;
    }

    // encoded company/oem
    // coded with 01 = 0, 10 = 1 transitions
    // stored in word 0
    if((card_data[0] >> 10 & ((1 << 14) - 1)) != 0x1555) {
        return false;
    }

    // data decoding
    uint64_t result = 0;
    result |= machester_decode(card_data[0]);
    result <<= 16;
    result |= machester_decode(card_data[1]);
    result <<= 16;
    result |= machester_decode(card_data[2]);

    // trailing parity (odd) test
    uint8_t parity_sum = 0;
    for(int8_t i = 0; i < 18; i++) {
        if(((result >> i) & 1) == 1) {
            parity_sum++;
        }
    }

    if((parity_sum % 2) != 1) {
        return false;
    }

    // leading parity (even) test
    parity_sum = 0;
    for(int8_t i = 18; i < 37; i++) {
        if(((result >> i) & 1) == 1) {
            parity_sum++;
        }
    }

    if((parity_sum % 2) == 1) {
        return false;
    }

    return true;
}