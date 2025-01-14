#pragma once
#include <cstdint>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <assert.h>

using u8 = uint8_t;
using s8 = int8_t;
using u16 = uint16_t;
using s16 = int16_t;
using u32 = uint32_t;
using s32 = int32_t;
using u64 = uint64_t;
using s64 = int64_t;

u32 setBit(u32 val, u8 bitToSet);
u32 resetBit(u32 val, u8 bitToReset);
u8 testBit(u32 val, u8 bitToTest);

//Return value within bit range of from - to
u32 getNthBits(u32 value, u8 from, u8 to);
u32 signExtend32(s32 value, u8 start);
u16 signExtend16(s16 value, u8 start);
u8 signExtend8(s8 value, u8 start);

//Return the number of bits set
u8 numSetBitsU16(u16 value);
u8 numSetBitsU8(u8 value);

u8 toBcd(s32 value);