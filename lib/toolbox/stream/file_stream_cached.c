#include "file_stream.h"
#include "file_stream_i.h"
#include "file_stream_cached.h"

#define FILE_STREAM_READ_CACHE_SIZE 512U

typedef struct {
    uint8_t* data;
    size_t data_size;
    size_t offset;
} FileStreamCache;

typedef struct {
    FileStream file_stream;
    FileStreamCache read_cache;
} FileStreamCached;

static void file_stream_cached_free(FileStreamCached* stream);
static bool file_stream_cached_eof(FileStreamCached* stream);
static void file_stream_cached_clean(FileStreamCached* stream);
static bool
    file_stream_cached_seek(FileStreamCached* stream, int32_t offset, StreamOffset offset_type);
static size_t file_stream_cached_tell(FileStreamCached* stream);
static size_t file_stream_cached_size(FileStreamCached* stream);
static size_t file_stream_cached_write(FileStreamCached* stream, const uint8_t* data, size_t size);
static size_t file_stream_cached_read(FileStreamCached* stream, uint8_t* data, size_t size);
static bool file_stream_cached_delete_and_insert(
    FileStreamCached* stream,
    size_t delete_size,
    StreamWriteCB write_callback,
    const void* ctx);

static void file_stream_cached_fill(FileStreamCache* cache, FileStream* file_stream);
static size_t file_stream_cached_take(FileStreamCache* cache, uint8_t* data, size_t size);
static void file_stream_cached_invalidate(FileStreamCache* cache);

const StreamVTable file_stream_cached_vtable = {
    .free = (StreamFreeFn)file_stream_cached_free,
    .eof = (StreamEOFFn)file_stream_cached_eof,
    .clean = (StreamCleanFn)file_stream_cached_clean,
    .seek = (StreamSeekFn)file_stream_cached_seek,
    .tell = (StreamTellFn)file_stream_cached_tell,
    .size = (StreamSizeFn)file_stream_cached_size,
    .write = (StreamWriteFn)file_stream_cached_write,
    .read = (StreamReadFn)file_stream_cached_read,
    .delete_and_insert = (StreamDeleteAndInsertFn)file_stream_cached_delete_and_insert,
};

Stream* file_stream_cached_alloc(Storage* storage) {
    FileStreamCached* stream = malloc(sizeof(FileStreamCached));

    stream->file_stream.file = storage_file_alloc(storage);
    stream->file_stream.storage = storage;
    stream->file_stream.stream_base.vtable = &file_stream_cached_vtable;

    stream->read_cache.data = malloc(FILE_STREAM_READ_CACHE_SIZE);
    return (Stream*)stream;
}

static void file_stream_cached_free(FileStreamCached* stream) {
    free(stream->read_cache.data);
    file_stream_free((FileStream*)stream);
}

static bool file_stream_cached_eof(FileStreamCached* stream) {
    UNUSED(stream);
    furi_crash("Using eof() on a cached file stream is not implemented");
}

static void file_stream_cached_clean(FileStreamCached* stream) {
    UNUSED(stream);
    furi_crash("Using clean() on a cached file stream is not implemented");
}

static bool
    file_stream_cached_seek(FileStreamCached* stream, int32_t offset, StreamOffset offset_type) {
    UNUSED(stream);
    UNUSED(offset);
    UNUSED(offset_type);
    furi_crash("Using seek() on a cached file stream is not implemented");
}

static size_t file_stream_cached_tell(FileStreamCached* stream) {
    UNUSED(stream);
    furi_crash("Using tell() on a cached file stream is not implemented");
}

static size_t file_stream_cached_size(FileStreamCached* stream) {
    UNUSED(stream);
    furi_crash("Using size() on a cached file stream is not implemented");
}

static size_t
    file_stream_cached_write(FileStreamCached* stream, const uint8_t* data, size_t size) {
    UNUSED(stream);
    UNUSED(data);
    UNUSED(size);
    furi_crash("Using write() on a cached file stream is not implemented");
}

static size_t file_stream_cached_read(FileStreamCached* stream, uint8_t* data, size_t size) {
    FileStreamCache* cache = &stream->read_cache;
    size_t need_to_read = size;

    if(size >= FILE_STREAM_READ_CACHE_SIZE) {
        file_stream_cached_invalidate(cache);
        need_to_read -= file_stream_read((FileStream*)stream, data, size);
    } else {
        if(cache->data_size) {
            need_to_read -= file_stream_cached_take(cache, data, size);
        }

        if(need_to_read) {
            file_stream_cached_fill(cache, (FileStream*)stream);
            need_to_read -= file_stream_cached_take(cache, data, size);
        }
    }

    return size - need_to_read;
}

static bool file_stream_cached_delete_and_insert(
    FileStreamCached* stream,
    size_t delete_size,
    StreamWriteCB write_callback,
    const void* ctx) {
    UNUSED(stream);
    UNUSED(delete_size);
    UNUSED(write_callback);
    UNUSED(ctx);
    furi_crash("Using delete_and_insert() on a cached file stream is not implemented");
}

// TODO Move these methods to a separate FileStreamCache class
static void file_stream_cached_fill(FileStreamCache* cache, FileStream* file_stream) {
    cache->data_size = file_stream_read(file_stream, cache->data, FILE_STREAM_READ_CACHE_SIZE);
    cache->offset = 0;
}

static void file_stream_cached_invalidate(FileStreamCache* cache) {
    cache->data_size = 0;
    cache->offset = 0;
}

static size_t file_stream_cached_take(FileStreamCache* cache, uint8_t* data, size_t size) {
    const size_t size_taken = MIN(size, cache->data_size);
    memcpy(data, cache->data + cache->offset, size_taken);
    cache->data_size -= size_taken;
    cache->offset += size_taken;
    return size - size_taken;
}
