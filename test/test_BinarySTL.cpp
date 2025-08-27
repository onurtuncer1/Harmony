// ----------------------------------------------------------------------
// Project: Harmony Geometry Serialization Deserialization Library
// Copyright(c) 2025 Onur Tuncer, PhD, Istanbul Technical University
//
// SPDX - License - Identifier : BSD-3-Clause
// License - Filename : LICENSE
// ----------------------------------------------------------------------

// SPDX-License-Identifier: BSD-3-Clause
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Harmony/STL/Ascii.h"
#include "Harmony/STL/Parse.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>

namespace fs = std::filesystem;

using namespace Harmony::STL::Binary;

using Harmony::STL::Mesh;
using Harmony::STL::Triangle;
using Harmony::STL::Vec3;

static bool nearly_equal(float a, float b, float eps=1e-5f) {
    return std::fabs(a-b) <= eps;
}
static void check_vec3(const Vec3& a, const Vec3& b, float eps=1e-5f) {
    REQUIRE(nearly_equal(a.x, b.x, eps));
    REQUIRE(nearly_equal(a.y, b.y, eps));
    REQUIRE(nearly_equal(a.z, b.z, eps));
}

TEST_CASE("Binary STL: serialize -> parse round-trip (stringstream)") {
    Mesh m;
    m.name = "bin-mesh";
    {
        Triangle t{};
        t.v[0] = {0,0,0};
        t.v[1] = {1,0,0};
        t.v[2] = {0,1,0};
        t.normal = {0,0,1};
        m.tris.push_back(t);
    }
    {
        Triangle t{};
        t.v[0] = {0,0,1};
        t.v[1] = {1,0,1};
        t.v[2] = {0,1,1};
        t.normal = {0,0,1};
        m.tris.push_back(t);
    }

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(serialize(ss, m, "Header: bin test", /*attr*/0));

    // rewind
    ss.seekg(0);
    auto r = parse(ss);
    REQUIRE(r.has_value());
    const auto& m2 = *r;
    // Name is derived from 80-byte header, not m.name
    REQUIRE_THAT(m2.name, ContainsSubstring("Header: bin test"));
    REQUIRE(m2.tris.size() == 2);
    check_vec3(m2.tris[0].v[0], {0,0,0});
    check_vec3(m2.tris[0].v[1], {1,0,0});
    check_vec3(m2.tris[0].v[2], {0,1,0});
    check_vec3(m2.tris[1].v[0], {0,0,1});
    check_vec3(m2.tris[1].v[1], {1,0,1});
    check_vec3(m2.tris[1].v[2], {0,1,1});
    // normals preserved
    check_vec3(m2.tris[0].normal, {0,0,1});
    check_vec3(m2.tris[1].normal, {0,0,1});
}

TEST_CASE("Binary STL: writer computes normal if zero") {
    Mesh m;
    m.name = "nfix";
    HSTL::Triangle t{};
    t.v[0] = {0,0,0};
    t.v[1] = {1,0,0};
    t.v[2] = {0,1,0};
    t.normal = {0,0,0}; // zero; writer should compute
    m.tris.push_back(t);

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(HSTL::serialize(ss, m));
    ss.seekg(0);
    auto r = HSTL::parse_binary(ss);
    REQUIRE(r.has_value());
    const auto& tri = r->tris.at(0);
    REQUIRE(nearly_equal(tri.normal.x, 0.f));
    REQUIRE(nearly_equal(tri.normal.y, 0.f));
    REQUIRE(nearly_equal(tri.normal.z, 1.f));
}

TEST_CASE("Binary STL: file I/O round-trip with non-zero attribute bytes") {
    HSTL::Mesh m;
    m.name = "attr";
    HSTL::Triangle t{};
    t.v[0] = {0,0,0};
    t.v[1] = {0,1,0};
    t.v[2] = {0,0,1};
    t.normal = {1,0,0};
    m.tris.push_back(t);

    const auto tmp = fs::temp_directory_path() / "harmony_bin_stl_test.stl";
    {
        std::ofstream out(tmp, std::ios::binary);
        REQUIRE(out.good());
        REQUIRE(HSTL::serialize_binary(out, m, "attr-header", /*attribute_byte_count*/ 2));
    }
    {
        std::ifstream in(tmp, std::ios::binary);
        REQUIRE(in.good());
        auto r = HSTL::parse_binary(in);
        REQUIRE(r.has_value());
        REQUIRE(r->tris.size() == 1);
        check_vec3(r->tris[0].v[0], {0,0,0});
        check_vec3(r->tris[0].v[1], {0,1,0});
        check_vec3(r->tris[0].v[2], {0,0,1});
        check_vec3(r->tris[0].normal, {1,0,0});
    }
    std::error_code ec;
    fs::remove(tmp, ec);
}

TEST_CASE("Binary STL: autodetect parses both ASCII and Binary") {
    // 1) ASCII
    const char* asciiTxt = R"(solid auto
  facet normal 0 0 1
    outer loop
      vertex 0 0 0
      vertex 1 0 0
      vertex 0 1 0
    endloop
  endfacet
endsolid auto
)";
    {
        std::stringstream ss; ss << asciiTxt; // text is fine; parse_auto peeks then ASCII path
        auto r = HSTL::parse_auto(ss);
        REQUIRE(r.has_value());
        REQUIRE(r->tris.size() == 1);
        check_vec3(r->tris[0].normal, {0,0,1});
    }

    // 2) Binary
    {
        HSTL::Mesh m;
        m.name = "auto-binary";
        HSTL::Triangle t{}; t.v[0]={0,0,0}; t.v[1]={1,0,0}; t.v[2]={0,1,0}; t.normal={0,0,1};
        m.tris.push_back(t);
        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        REQUIRE(HSTL::serialize_binary(ss, m, "auto-bin", 0));
        ss.seekg(0);
        auto r = Harmony::STL::parse(ss);
        REQUIRE(r.has_value());
        REQUIRE(r->tris.size() == 1);
        check_vec3(r->tris[0].v[1], {1,0,0});
    }
}

TEST_CASE("Binary STL: malformed files -> clear errors") {
    // Helper to write little-endian u32
    auto put_u32_le = [](std::ostream& os, std::uint32_t v){
        unsigned char b[4]{
            (unsigned char)(v & 0xFF),
            (unsigned char)((v >> 8) & 0xFF),
            (unsigned char)((v >> 16) & 0xFF),
            (unsigned char)((v >> 24) & 0xFF)
        };
        os.write(reinterpret_cast<const char*>(b), 4);
    };

    // 1) Too short header
    {
        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        // write <80 bytes on purpose
        ss.write("short", 5);
        ss.seekg(0);
        auto r = parse(ss);
        REQUIRE_FALSE(r.has_value());
        REQUIRE_THAT(r.error(), ContainsSubstring("80-byte header"));
    }

    // 2) Count says 2 triangles but only 1 provided -> EOF in triangle data
    {
        std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
        // 80-byte header
        std::string hdr(80, 'H');
        ss.write(hdr.data(), 80);
        // tri count = 2
        put_u32_le(ss, 2);
        // write ONE (50-byte) record only
        std::string rec(50, '\0');
        ss.write(rec.data(), 50);
        ss.seekg(0);
        auto r = parse(ss);
        REQUIRE_FALSE(r.has_value());
        REQUIRE_THAT(r.error(), ContainsSubstring("unexpected EOF in triangle data"));
    }
}

TEST_CASE("Binary STL: large mesh reserve and precision tolerance") {
    // Build N simple triangles and ensure the reader gets them all.
    constexpr int N = 128;
    Mesh m; 
    m.name = "bulk";
    for (int i=0;i<N;++i) {
        Triangle t{};
        t.v[0] = {float(i), 0, 0};
        t.v[1] = {float(i), 1, 0};
        t.v[2] = {float(i), 0, 1};
        t.normal = {0,1,0};
        m.tris.push_back(t);
    }

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(HSTL::serialize_binary(ss, m));
    ss.seekg(0);
    auto r = parse(ss);
    REQUIRE(r.has_value());
    REQUIRE(r->tris.size() == size_t(N));
    // spot-check a few values
    check_vec3(r->tris[0].v[0], {0,0,0});
    check_vec3(r->tris[127].v[2], {127,0,1});
    // float tolerance example
    REQUIRE_THAT(r->tris[10].v[0].x, WithinAbs(10.0f, 1e-6f));
}
