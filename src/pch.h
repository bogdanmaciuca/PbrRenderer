#pragma once

#include <string>
#include <vector>
#include <span>
#include <algorithm>
#include <cstdint>
#include <array>
#include "SDL3/SDL.h"

using u8 = uint8_t;
using i8 = int8_t;
using u16 = uint16_t;
using i16 = int16_t;
using u32 = uint32_t;
using i32 = int32_t;
using u64 = uint64_t;
using i64 = int64_t;

using string = std::string;
template<typename T, size_t size> using array = std::array<T, size>;
template<typename T> using vector = std::vector<T>;
template<typename T, size_t size> using span = std::span<T, size>;

void FatalError(const string& message);

