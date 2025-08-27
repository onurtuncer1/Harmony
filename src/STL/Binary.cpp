// ----------------------------------------------------------------------
// Project: Harmony Geometry Serialization Deserialization Library
// Copyright(c) 2025 Onur Tuncer, PhD, Istanbul Technical University
//
// SPDX - License - Identifier : BSD-3-Clause
// License - Filename : LICENSE
// ----------------------------------------------------------------------

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cmath>
#include <istream>
#include <ostream>
#include <span>
#include <array>
#include <string>
#include <string_view>
#include <expected>
#include <type_traits>
#include <cstring>   // std::memcpy

#include "Harmony/STL/Mesh.h"
#include "Harmony/STL/Binary.h"

namespace Harmony::STL::Binary {  

    using namespace Harmony::STL;

    std::expected<Mesh, std::string> parse(std::istream& is, bool compute_missing_normals) {
    Mesh mesh;
    mesh.tris.clear();

    // Header (80 bytes) + uint32 count
    std::byte header[80];
    if (!read_exact(is, header)) {
        return std::unexpected(std::string("Binary STL: failed to read 80-byte header"));
    }

    std::byte countBuf[4];
    if (!read_exact(is, countBuf)) {
        return std::unexpected(std::string("Binary STL: failed to read triangle count"));
    }
    const std::uint32_t triCount = load_le<std::uint32_t>(std::span<const std::byte,4>(countBuf, 4));

    // Optional: set mesh name from header (trim trailing zeros/spaces)
    {
        std::string name(reinterpret_cast<const char*>(header), 80);
        // shrink trailing NULs/spaces
        auto pos = name.find_last_not_of(std::string("\0 \t\r\n", 5));
        mesh.name = (pos == std::string::npos) ? std::string{} : name.substr(0, pos + 1);
    }

    mesh.tris.reserve(triCount);

    // Each triangle: normal(3f) + v0(3f) + v1(3f) + v2(3f) + attr(2B)
    std::byte rec[50]; // 12 floats * 4 = 48 + 2 attribute bytes
    for (std::uint32_t i = 0; i < triCount; ++i) {
        if (!read_exact(is, rec)) {
            return std::unexpected(std::string("Binary STL: unexpected EOF in triangle data"));
        }
        const auto f = [&](size_t idx)->float {
            std::span<const std::byte,4> s{rec + idx*4, 4};
            return load_le<float>(s);
        };

        Triangle t{};
        t.normal = Vec3{ f(0),  f(1),  f(2)  };
        t.v[0]   = Vec3{ f(3),  f(4),  f(5)  };
        t.v[1]   = Vec3{ f(6),  f(7),  f(8)  };
        t.v[2]   = Vec3{ f(9),  f(10), f(11) };

        // attribute byte count: last 2 bytes (ignored)
        // If normal is zero and requested, compute
        if (compute_missing_normals) {
            if (std::abs(t.normal.x) + std::abs(t.normal.y) + std::abs(t.normal.z) < 1e-20f) {
                t.normal = face_normal(t);
            }
        }
        mesh.tris.push_back(t);
    }

    return mesh;
}

bool serialize(std::ostream& os,
                      const Mesh& mesh,
                      std::string_view header,
                      std::uint16_t attribute_byte_count) {
    // 80-byte header
    std::byte hdr[80]{};
    // copy header truncated/padded
    const size_t copyN = std::min<size_t>(80, header.size());
    std::memcpy(hdr, header.data(), copyN);
    if (!write_exact(os, std::span<const std::byte>(hdr, 80))) return false;

    // triangle count (uint32 LE)
    std::byte cnt[4];
    store_le<std::uint32_t>(static_cast<std::uint32_t>(mesh.tris.size()),
                            std::span<std::byte,4>(cnt, 4));
    if (!write_exact(os, cnt)) return false;

    // records
    std::byte rec[50];
    for (const auto& tIn : mesh.tris) {
        // ensure nonzero normal for better compatibility
        Triangle t = tIn;
        if (std::abs(t.normal.x)+std::abs(t.normal.y)+std::abs(t.normal.z) < 1e-20f)
            t.normal = face_normal(t);

        const float vals[12] = {
            t.normal.x, t.normal.y, t.normal.z,
            t.v[0].x,   t.v[0].y,   t.v[0].z,
            t.v[1].x,   t.v[1].y,   t.v[1].z,
            t.v[2].x,   t.v[2].y,   t.v[2].z
        };
        for (size_t i=0;i<12;++i) {
            store_le<float>(vals[i],
                std::span<std::byte,4>{rec + i*4, 4});
        }
        // attribute bytes
        store_le<std::uint16_t>(attribute_byte_count,
            std::span<std::byte,2>{rec + 48, 2});

        if (!write_exact(os, std::span<const std::byte>(rec, 50))) return false;
    }
    return static_cast<bool>(os);
}

  
} // namespace Harmony::STL::Binary