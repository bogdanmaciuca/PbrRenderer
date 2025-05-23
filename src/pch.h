#pragma once

#include <string>
#include <vector>
#include <span>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <array>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include "SDL3/SDL.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "stb/stb_image.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlgpu3.h"

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
template<typename Key, typename T> using umap = std::unordered_map<Key, T>;
template<typename T> using unique = std::unique_ptr<T>;
template<typename T> using shared = std::shared_ptr<T>;

void Error(const string& message);
void FatalError(const string& message);

// Milliseconds
template<typename T = float>
class Timer {
public:
    Timer() {
        m_start = std::chrono::steady_clock::now();
    }
    void Reset() {
        m_start = std::chrono::steady_clock::now();
    }
    T GetTime() {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();;
        return std::chrono::duration<T, std::milli>(now - m_start).count();
    }
private:
    std::chrono::steady_clock::time_point m_start;
};

string ReadFile(const string& path);
