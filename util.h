#pragma once

#include <stdint.h>

#define STR_EQUAL(x,y) (strcmp((x), (y)) == 0)

typedef uint8_t   u8;
typedef int8_t    s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;

int read_file(const char* filepath, char* ret_buf, u32 max_buffer_size);
