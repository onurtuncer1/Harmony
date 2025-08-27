// ----------------------------------------------------------------------
// Project: Harmony Geometry Serialization Deserialization Library
// Copyright(c) 2025 Onur Tuncer, PhD, Istanbul Technical University
//
// SPDX - License - Identifier : BSD-3-Clause
// License - Filename : LICENSE
// ----------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <array>
#include <span>
#include <string>
#include <string_view>
#include <expected>
#include <istream>
#include <ostream>
#include <bit>
#include <algorithm>
#include <type_traits>
#include <cstring> // std::memcpy

#include "Mesh.h"

namespace Harmony::STL::Binary {

/// Parse a binary STL from a contiguous text buffer
[[nodiscard]] std::expected<Mesh, std::string>
parse(std::string_view text, bool compute_missing_normals = true) noexcept;

/// Parse from a stream (reads the whole stream text)
[[nodiscard]] std::expected<Mesh, std::string>
parse(std::istream& is, bool compute_missing_normals = true);

bool serialize(std::ostream& os,
                      const Mesh& mesh,
                      std::string_view header,
                      std::uint16_t attribute_byte_count);

// ---- LE load/store helpers (templates must be in header) ----
template <class T>
inline T load_le(std::span<const std::byte, sizeof(T)> bytes) {
    static_assert(std::is_trivially_copyable_v<T>);
    T v{};
    std::memcpy(&v, bytes.data(), sizeof(T));
    if constexpr (std::endian::native == std::endian::big) {
        auto* p = reinterpret_cast<std::byte*>(&v);
        std::reverse(p, p + sizeof(T));
    }
    return v;
}

template <class T>
inline void store_le(const T& value, std::span<std::byte, sizeof(T)> out) {
    static_assert(std::is_trivially_copyable_v<T>);
    T v = value;
    if constexpr (std::endian::native == std::endian::big) {
        auto* p = reinterpret_cast<std::byte*>(&v);
        std::reverse(p, p + sizeof(T));
    }
    std::memcpy(out.data(), &v, sizeof(T));
}

// ---- stream helpers ----
inline bool read_exact(std::istream& is, std::span<std::byte> buf) {
    std::size_t got = 0;
    while (got < buf.size()) {
        is.read(reinterpret_cast<char*>(buf.data() + got),
                static_cast<std::streamsize>(buf.size() - got));
        if (!is) return false;
        got += static_cast<std::size_t>(is.gcount());
    }
    return true;
}

inline bool write_exact(std::ostream& os, std::span<const std::byte> buf) {
    os.write(reinterpret_cast<const char*>(buf.data()),
             static_cast<std::streamsize>(buf.size()));
    return static_cast<bool>(os);
}

} // namespace Harmony::STL::Binary
