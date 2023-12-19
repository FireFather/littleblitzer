#pragma once

using S8 = int8_t;
using U8 = uint8_t;
using S16 = int16_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

constexpr auto MAX_S32 = 2147483647;
constexpr auto MIN_S16 = -32768;
constexpr auto MAX_S16 = 32767;
constexpr auto MIN_U16 = 0;
constexpr auto MAX_U16 = 65536;
constexpr auto MIN_S8 = -128;
constexpr auto MAX_S8 = 127;

constexpr auto CACHE_LINE = 64;
#define CACHE_ALIGN __declspec(align(CACHE_LINE))
