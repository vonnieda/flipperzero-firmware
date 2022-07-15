#pragma once
#include <stdlib.h>
#include <storage/storage.h>
#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate a cached file stream
 * @return Stream*
 */
Stream* file_stream_cached_alloc(Storage* storage);

#ifdef __cplusplus
}
#endif
