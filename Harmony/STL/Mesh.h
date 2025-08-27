// ----------------------------------------------------------------------
// Project: Harmony Geometry Serialization Deserialization Library
// Copyright(c) 2025 Onur Tuncer, PhD, Istanbul Technical University
//
// SPDX - License - Identifier : BSD-3-Clause
// License - Filename : LICENSE
// ----------------------------------------------------------------------

#pragma once

#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace Harmony::STL {

    // Consider using vectors from cadence library
struct Vec3 {
    float x{}, y{}, z{};
};

struct Triangle {
    Vec3 normal{};
    std::array<Vec3,3> v{};
};

struct Mesh {
    std::string name;
    std::vector<Triangle> tris;
};

/// Utility: compute geometric normal of a triangle (right-handed)
[[nodiscard]] inline Vec3 face_normal(const Triangle& t) {
    auto sub = [](const Vec3& a, const Vec3& b){ return Vec3{a.x-b.x, a.y-b.y, a.z-b.z}; };
    auto cross = [](const Vec3& a, const Vec3& b){
        return Vec3{
            a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x
        };
    };
    auto n = cross(sub(t.v[1], t.v[0]), sub(t.v[2], t.v[0]));
    const float len = std::sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
    if (len > 0.0f) { n.x/=len; n.y/=len; n.z/=len; }
    return n;
}

} // namespace Harmony::STL