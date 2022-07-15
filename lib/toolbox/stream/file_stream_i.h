#pragma once
#include "stream.h"
#include "stream_i.h"

typedef struct {
    Stream stream_base;
    Storage* storage;
    File* file;
} FileStream;

void file_stream_free(FileStream* stream);
bool file_stream_eof(FileStream* stream);
void file_stream_clean(FileStream* stream);
bool file_stream_seek(FileStream* stream, int32_t offset, StreamOffset offset_type);
size_t file_stream_tell(FileStream* stream);
size_t file_stream_size(FileStream* stream);
size_t file_stream_write(FileStream* stream, const uint8_t* data, size_t size);
size_t file_stream_read(FileStream* stream, uint8_t* data, size_t size);
bool file_stream_delete_and_insert(
    FileStream* stream,
    size_t delete_size,
    StreamWriteCB write_callback,
    const void* ctx);
