// ----------------------------------------------------------------------
// Project: Harmony Geometry Serialization Deserialization Library
// Copyright(c) 2025 Onur Tuncer, PhD, Istanbul Technical University
//
// SPDX - License - Identifier : BSD-3-Clause
// License - Filename : LICENSE
// ----------------------------------------------------------------------

#include <catch2/catch_test_macros.hpp>                 // <-- defines TEST_CASE, REQUIRE, REQUIRE_THAT, etc.
#include <catch2/matchers/catch_matchers_string.hpp>    // <-- string matchers: ContainsSubstring, StartsWith, EndsWith, ...
#include <catch2/matchers/catch_matchers_floating_point.hpp> // <-- WithinAbs, WithinRel, ...


#include <fstream>
#include <sstream>
#include <cstdio>
#include <filesystem>

#include <Harmony/STL/Ascii.h>

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::WithinAbs;

namespace fs = std::filesystem;

using namespace Harmony::STL;

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

TEST_CASE("Parse minimal valid ASCII STL (two triangles)") {
    const char* txt = R"(solid sample_cube
  facet normal 0 0 1
    outer loop
      vertex 0 0 0
      vertex 1 0 0
      vertex 0 1 0
    endloop
  endfacet
  facet normal 0 0 1
    outer loop
      vertex 1 0 0
      vertex 1 1 0
      vertex 0 1 0
    endloop
  endfacet
endsolid sample_cube
)";
    auto r = parse(std::string_view{txt});
    REQUIRE(r.has_value());
    const Mesh& m = *r;
    REQUIRE(m.name == "sample_cube");
    REQUIRE(m.tris.size() == 2);
    for (auto& t : m.tris) {
        CHECK(nearly_equal(t.normal.x, 0.f));
        CHECK(nearly_equal(t.normal.y, 0.f));
        CHECK(nearly_equal(t.normal.z, 1.f));
    }
}

TEST_CASE("Round-trip parse -> serialize -> parse preserves geometry") {
    Triangle t{};
    t.v[0] = {0,0,0};
    t.v[1] = {1,0,0};
    t.v[2] = {0,1,0};
    t.normal = {0,0,1};

    Mesh m;
    m.name = "rt";
    m.tris.push_back(t);

    auto s = serialize(m, /*precision*/6);
    REQUIRE_THAT(s, ContainsSubstring("solid rt"));
    REQUIRE_THAT(s, ContainsSubstring("facet normal 0.000000 0.000000 1.000000"));

    auto r2 = parse(std::string_view{s});
    REQUIRE(r2.has_value());
    const Mesh& m2 = *r2;
    REQUIRE(m2.tris.size() == 1);
    check_vec3(m2.tris[0].v[0], {0,0,0});
    check_vec3(m2.tris[0].v[1], {1,0,0});
    check_vec3(m2.tris[0].v[2], {0,1,0});
}

TEST_CASE("Missing normal gets computed when requested") {
    const char* txt = R"(solid n/a
  facet normal 0 0 0
    outer loop
      vertex 0 0 0
      vertex 1 0 0
      vertex 0 1 0
    endloop
  endfacet
endsolid
)";
    auto r = parse(std::string_view{txt}, /*compute_missing_normals=*/true);
    REQUIRE(r.has_value());
    const auto& tri = r->tris.at(0);
    // Should compute +Z normal for counter-clockwise vertices
    CHECK(nearly_equal(tri.normal.x, 0.f));
    CHECK(nearly_equal(tri.normal.y, 0.f));
    CHECK(nearly_equal(tri.normal.z, 1.f));
}

TEST_CASE("Missing normal NOT computed when disabled") {
    const char* txt = R"(solid n/a
  facet normal 0 0 0
    outer loop
      vertex 0 0 0
      vertex 1 0 0
      vertex 0 1 0
    endloop
  endfacet
endsolid
)";
    auto r = parse(std::string_view{txt}, /*compute_missing_normals=*/false);
    REQUIRE(r.has_value());
    const auto& tri = r->tris.at(0);
    CHECK(nearly_equal(tri.normal.x, 0.f));
    CHECK(nearly_equal(tri.normal.y, 0.f));
    CHECK(nearly_equal(tri.normal.z, 0.f));
}

TEST_CASE("Case-insensitive keywords and flexible whitespace") {
    const char* txt = R"(SoLiD  name   
  Facet   Normal   0   0   1
    OUTER     LOOP
      VERTEX 0 0 0
      vertex 1 0 0
      vertex 0 1 0
    ENDLOOP
  ENdFaCeT
EnDsOlId name
)";
    auto r = parse(std::string_view{txt});
    REQUIRE(r.has_value());
    REQUIRE(r->tris.size() == 1);
}

TEST_CASE("Serialize precision control") {
    Mesh m;
    m.name = "p";
    Triangle t{};
    t.v[0] = {0.12345678f, 0, 0};
    t.v[1] = {0, 0.12345678f, 0};
    t.v[2] = {0, 0, 0.12345678f};
    t.normal = {0,0,1};
    m.tris.push_back(t);

    auto s3 = serialize(m, /*precision*/3);
    REQUIRE_THAT(s3, ContainsSubstring("0.123")); // rounded
    REQUIRE(s3.find("0.1234") == std::string::npos);

    auto s1 = serialize(m, /*precision*/1);
    REQUIRE_THAT(s1, ContainsSubstring("0.1"));
}

TEST_CASE("Stream parsing from std::istream") {
    std::stringstream ss;
    ss << "solid s\n"
          "  facet normal 0 0 1\n"
          "    outer loop\n"
          "      vertex 0 0 0\n"
          "      vertex 1 0 0\n"
          "      vertex 0 1 0\n"
          "    endloop\n"
          "  endfacet\n"
          "endsolid s\n";
    auto r = parse(ss);
    REQUIRE(r.has_value());
    REQUIRE(r->tris.size() == 1);
}

TEST_CASE("File I/O round-trip") {
    // Build a mesh
    Mesh m;
    m.name = "file";
    Triangle t{};
    t.v = { Vec3{0,0,0}, Vec3{1,0,0}, Vec3{0,1,0} };
    t.normal = {0,0,1};
    m.tris.push_back(t);

    // Serialize to a temp file
    auto tmp = fs::temp_directory_path() / "ascii_stl_test_file.stl";
    {
        std::ofstream out(tmp);
        REQUIRE(out.good());
        REQUIRE(serialize(out, m, 6));
    }

    // Read back
    {
        std::ifstream in(tmp);
        REQUIRE(in.good());
        auto r = parse(in);
        REQUIRE(r.has_value());
        REQUIRE(r->name == "file");
        REQUIRE(r->tris.size() == 1);
        check_vec3(r->tris[0].v[0], {0,0,0});
        check_vec3(r->tris[0].v[1], {1,0,0});
        check_vec3(r->tris[0].v[2], {0,1,0});
    }

    // Cleanup
    std::error_code ec;
    fs::remove(tmp, ec);
}

TEST_CASE("Graceful EOF without 'endsolid' (allowed when not mid-facet)") {
    const char* txt = R"(solid loose
  facet normal 0 0 1
    outer loop
      vertex 0 0 0
      vertex 1 0 0
      vertex 0 1 0
    endloop
  endfacet
)"; // no endsolid
    auto r = parse(std::string_view{txt});
    REQUIRE(r.has_value());
    REQUIRE(r->tris.size() == 1);
}

TEST_CASE("Errors: unexpected token and structure violations") {
    SECTION("vertex outside loop") {
        const char* txt = R"(solid bad
  vertex 0 0 0
endsolid bad
)";
        auto r = parse(std::string_view{txt});
        REQUIRE_FALSE(r.has_value());
        REQUIRE_THAT(r.error(), ContainsSubstring("vertex"));
    }

    SECTION("endloop before three vertices") {
        const char* txt = R"(solid bad
  facet normal 0 0 1
    outer loop
      vertex 0 0 0
      vertex 1 0 0
    endloop
  endfacet
endsolid bad
)";
        auto r = parse(std::string_view{txt});
        REQUIRE_FALSE(r.has_value());
        REQUIRE_THAT(r.error(), ContainsSubstring("three vertices"));
    }

    SECTION("garbage line") {
        const char* txt = R"(solid s
  nonsense here
endsolid s
)";
        auto r = parse(std::string_view{txt});
        REQUIRE_FALSE(r.has_value());
        REQUIRE_THAT(r.error(), ContainsSubstring("unexpected content"));
    }

    SECTION("bad float") {
        const char* txt = R"(solid s
  facet normal 0 0Z 1
    outer loop
      vertex 0 0 0
      vertex 1 0 0
      vertex 0 1 0
    endloop
  endfacet
endsolid s
)";
        auto r = parse(std::string_view{txt});
        REQUIRE_FALSE(r.has_value());
        REQUIRE_THAT(r.error(), ContainsSubstring("Failed to parse number"));
    }
}

TEST_CASE("Serializer computes normals if zero before writing") {
    Mesh m;
    m.name = "nfix";
    Triangle t{};
    t.v = { Vec3{0,0,0}, Vec3{1,0,0}, Vec3{0,1,0} };
    t.normal = {0,0,0}; // zero -> should compute
    m.tris.push_back(t);

    auto s = serialize(m, 6);
    // Expect a normalized +Z normal in output
    REQUIRE_THAT(s, ContainsSubstring("facet normal 0.000000 0.000000 1.000000"));
}

