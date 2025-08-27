// ----------------------------------------------------------------------
// Project: Harmony Geometry Serialization Deserialization Library
// Copyright(c) 2025 Onur Tuncer, PhD, Istanbul Technical University
//
// SPDX - License - Identifier : BSD-3-Clause
// License - Filename : LICENSE
// ----------------------------------------------------------------------

#include <algorithm>
#include <cctype>
#include <istream>
#include <ostream>
#include <sstream>
#include <ranges>

#include "Harmony/STL/Ascii.h"

namespace Harmony::STL {

namespace {

inline std::string to_lower(std::string s) {
    std::ranges::transform(s, s.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}
inline std::string_view trim(std::string_view sv) {
    auto not_space = [](unsigned char c){ return !std::isspace(c); };
    while (!sv.empty() && !not_space(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
    while (!sv.empty() && !not_space(static_cast<unsigned char>(sv.back())))  sv.remove_suffix(1);
    return sv;
}
inline bool starts_with_icase(std::string_view sv, std::string_view prefix) {
    if (sv.size() < prefix.size()) return false;
    for (size_t i=0;i<prefix.size();++i) {
        if (std::tolower(static_cast<unsigned char>(sv[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) return false;
    }
    return true;
}
inline std::vector<std::string_view> split_ws(std::string_view sv) {
    std::vector<std::string_view> out;
    sv = trim(sv);
    while (!sv.empty()) {
        // skip leading space
        size_t i = 0;
        while (i<sv.size() && std::isspace(static_cast<unsigned char>(sv[i]))) ++i;
        sv.remove_prefix(i);
        if (sv.empty()) break;
        // find end
        size_t j = 0;
        while (j<sv.size() && !std::isspace(static_cast<unsigned char>(sv[j]))) ++j;
        out.emplace_back(sv.substr(0, j));
        sv.remove_prefix(j);
    }
    return out;
}
template<class T>
inline std::expected<T, std::string> from_chars_sv(std::string_view sv) {
    T value{};
    auto* first = sv.data();
    auto* last  = sv.data() + sv.size();
    if (auto [ptr, ec] = std::from_chars(first, last, value); ec == std::errc{} && ptr==last) {
        return value;
    }
    return std::unexpected(std::format("Failed to parse number: '{}'", sv));
}

inline std::expected<std::array<float,3>, std::string>
parse_three_floats_tail(std::string_view line_after_keyword) {
    auto toks = split_ws(line_after_keyword);
    if (toks.size() < 3) return std::unexpected("Expected three floats");
    auto fx = from_chars_sv<float>(toks[0]);
    if (!fx) return std::unexpected(fx.error());
    auto fy = from_chars_sv<float>(toks[1]);
    if (!fy) return std::unexpected(fy.error());
    auto fz = from_chars_sv<float>(toks[2]);
    if (!fz) return std::unexpected(fz.error());
    return std::array<float,3>{*fx, *fy, *fz};
}

inline std::string lower_sv(std::string_view sv) {
    std::string s(sv);
    std::ranges::transform(s, s.begin(), [](unsigned char c){ return char(std::tolower(c)); });
    return s;
}

inline bool tok_is(std::string_view sv, std::string_view expect_lc) {
    // compare case-insensitively without allocating for expect
    if (sv.size() != expect_lc.size()) {
        // allow different sizes with lower for sv then compare
        return lower_sv(sv) == expect_lc;
    }
    for (size_t i=0;i<sv.size();++i)
        if (std::tolower((unsigned char)sv[i]) != expect_lc[i]) return false;
    return true;
}

// Parse three floats from tokens[t..t+2]
inline std::expected<std::array<float,3>, std::string>
parse_three_floats_tokens(const std::vector<std::string_view>& toks, size_t t, size_t line_no) {
    if (toks.size() < t+3) return std::unexpected(std::format("Line {}: Expected three floats", line_no));
    auto fx = from_chars_sv<float>(toks[t+0]); if (!fx) return std::unexpected(std::format("Line {}: {}", line_no, fx.error()));
    auto fy = from_chars_sv<float>(toks[t+1]); if (!fy) return std::unexpected(std::format("Line {}: {}", line_no, fy.error()));
    auto fz = from_chars_sv<float>(toks[t+2]); if (!fz) return std::unexpected(std::format("Line {}: {}", line_no, fz.error()));
    return std::array<float,3>{*fx, *fy, *fz};
}


} // namespace

// std::expected<Mesh, std::string> parse(std::string_view text, bool compute_missing_normals) noexcept {
//     Mesh mesh;
//     mesh.tris.clear();

//     // iterate lines with ranges (keeps views, but we'll copy where needed)
//     bool in_solid = false;
//     Triangle current{};
//     enum class Phase { idle, have_facet, in_loop, have_v1, have_v2 } phase = Phase::idle;
//     size_t line_no = 0;

//     for (auto line_view : text | std::views::split('\n')) {
//         ++line_no;
//         std::string_view line{ &*line_view.begin(), static_cast<size_t>(std::ranges::distance(line_view)) };
//         auto line_trim = trim(line);
//         if (line_trim.empty()) continue;

//         // Header: solid <name...>
//         if (!in_solid) {
//             if (!starts_with_icase(line_trim, "solid"))
//                 return std::unexpected(std::format("Line {}: expected 'solid'", line_no));
//             // name is remainder after 'solid'
//             auto name_sv = trim(line_trim.substr(std::min<size_t>(line_trim.size(), 5)));
//             mesh.name = std::string{name_sv};
//             in_solid = true;
//             continue;
//         }

//         // Try to match keywords (case-insensitive)
//         if (starts_with_icase(line_trim, "endsolid")) {
//             // optionally check name; we accept any
//             in_solid = false;
//             break;
//         }

//         if (starts_with_icase(line_trim, "facet normal")) {
//             if (phase != Phase::idle)
//                 return std::unexpected(std::format("Line {}: 'facet' where not expected", line_no));
//             auto rest = trim(line_trim.substr(12)); // after "facet normal"
//             auto arr = parse_three_floats_tail(rest);
//             if (!arr) return std::unexpected(std::format("Line {}: {}", line_no, arr.error()));
//             current.normal = Vec3{(*arr)[0], (*arr)[1], (*arr)[2]};
//             phase = Phase::have_facet;
//             continue;
//         }

//         if (starts_with_icase(line_trim, "outer loop")) {
//             if (phase != Phase::have_facet)
//                 return std::unexpected(std::format("Line {}: 'outer loop' without facet", line_no));
//             phase = Phase::in_loop;
//             continue;
//         }

//         if (starts_with_icase(line_trim, "vertex")) {
//             if (phase != Phase::in_loop && phase != Phase::have_v1 && phase != Phase::have_v2)
//                 return std::unexpected(std::format("Line {}: 'vertex' outside of loop", line_no));
//             auto rest = trim(line_trim.substr(6)); // after "vertex"
//             auto arr = parse_three_floats_tail(rest);
//             if (!arr) return std::unexpected(std::format("Line {}: {}", line_no, arr.error()));
//             if (phase == Phase::in_loop) {
//                 current.v[0] = Vec3{(*arr)[0], (*arr)[1], (*arr)[2]};
//                 phase = Phase::have_v1;
//             } else if (phase == Phase::have_v1) {
//                 current.v[1] = Vec3{(*arr)[0], (*arr)[1], (*arr)[2]};
//                 phase = Phase::have_v2;
//             } else {
//                 current.v[2] = Vec3{(*arr)[0], (*arr)[1], (*arr)[2]};
//                 // keep phase, expect endloop
//             }
//             continue;
//         }

//         if (starts_with_icase(line_trim, "endloop")) {
//             if (phase != Phase::have_v2)
//                 return std::unexpected(std::format("Line {}: 'endloop' before three vertices", line_no));
//             // ok
//             continue;
//         }

//         if (starts_with_icase(line_trim, "endfacet")) {
//             if (phase != Phase::have_v2)
//                 return std::unexpected(std::format("Line {}: 'endfacet' without complete triangle", line_no));
//             // Fill normal if zero or requested
//             if (compute_missing_normals) {
//                 if (std::abs(current.normal.x) + std::abs(current.normal.y) + std::abs(current.normal.z) < 1e-20f) {
//                     current.normal = face_normal(current);
//                 }
//             }
//             mesh.tris.push_back(current);
//             current = Triangle{};
//             phase = Phase::idle;
//             continue;
//         }

//         // Allow 'endsolid name' anywhere after triangles
//         // Otherwise ignore unknown/extra whitespace-only lines
//         if (starts_with_icase(line_trim, "solid")) {
//             // Some files repeat 'solid name' before facets; accept and reset name
//             mesh.name = std::string{trim(line_trim.substr(5))};
//             continue;
//         }

//         // If the line is none of the known constructs, treat as error
//         return std::unexpected(std::format("Line {}: unexpected content: '{}'", line_no, std::string(line_trim)));
//     }

//     if (in_solid) {
//         // If we never saw endsolid, still return what we parsed
//         // but ensure we are not mid-triangle
//         if (phase != Phase::idle)
//             return std::unexpected("Unexpected EOF: unterminated facet/loop");
//     }
//     return mesh;
// }

// In namespace Harmony::STL
std::expected<Mesh, std::string>
parse(std::string_view text, bool compute_missing_normals) noexcept {
    Mesh mesh;
    mesh.tris.clear();

    // State
    bool in_solid = false;
    Triangle current{};
    enum class Phase { idle, have_facet, in_loop, have_v1, have_v2 } phase = Phase::idle;
    size_t vertex_count = 0;   // vertices seen inside current outer loop
    size_t line_no = 0;

    // Helpers (local lambdas)
    auto trim_sv = [](std::string_view sv) {
        auto issp = [](unsigned char c){ return std::isspace(c); };
        while (!sv.empty() && issp((unsigned char)sv.front())) sv.remove_prefix(1);
        while (!sv.empty() && issp((unsigned char)sv.back()))  sv.remove_suffix(1);
        return sv;
    };
    auto tokenize_ws = [&](std::string_view sv) {
        std::vector<std::string_view> out;
        sv = trim_sv(sv);
        while (!sv.empty()) {
            size_t i = 0; while (i < sv.size() && std::isspace((unsigned char)sv[i])) ++i;
            sv.remove_prefix(i);
            if (sv.empty()) break;
            size_t j = 0; while (j < sv.size() && !std::isspace((unsigned char)sv[j])) ++j;
            out.emplace_back(sv.substr(0, j));
            sv.remove_prefix(j);
        }
        return out;
    };
    auto eq_ci = [](std::string_view a, std::string_view b_lc) {
        // compare a (any case) to b in lowercase
        if (a.size() != b_lc.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (char(std::tolower((unsigned char)a[i])) != b_lc[i]) return false;
        return true;
    };
    auto three_floats_from = [&](const std::vector<std::string_view>& toks, size_t start) -> std::expected<std::array<float,3>, std::string> {
        if (toks.size() < start + 3) return std::unexpected(std::string("Expected three floats"));
        auto readf = [](std::string_view sv) -> std::expected<float,std::string> {
            float v{};
            const char* f = sv.data();
            const char* l = sv.data() + sv.size();
            if (auto [p, ec] = std::from_chars(f, l, v); ec == std::errc{} && p == l) return v;
            return std::unexpected(std::format("Failed to parse number: '{}'", sv));
        };
        auto fx = readf(toks[start+0]); if (!fx) return std::unexpected(fx.error());
        auto fy = readf(toks[start+1]); if (!fy) return std::unexpected(fy.error());
        auto fz = readf(toks[start+2]); if (!fz) return std::unexpected(fz.error());
        return std::array<float,3>{*fx, *fy, *fz};
    };
    auto join_name = [](const std::vector<std::string_view>& toks, size_t start) {
        std::string name;
        for (size_t i = start; i < toks.size(); ++i) {
            if (i > start) name.push_back(' ');
            name.append(toks[i].begin(), toks[i].end());
        }
        return name;
    };

    for (auto line_view : text | std::views::split('\n')) {
        ++line_no;
        std::string_view line{ &*line_view.begin(), static_cast<size_t>(std::ranges::distance(line_view)) };
        auto line_trim = trim_sv(line);
        if (line_trim.empty()) continue;

        auto toks = tokenize_ws(line_trim);
        if (toks.empty()) continue;

        if (!in_solid) {
            // Expect: solid [name...]
            if (!eq_ci(toks[0], "solid")) {
                return std::unexpected(std::format("Line {}: expected 'solid'", line_no));
            }
            mesh.name = (toks.size() > 1) ? join_name(toks, 1) : std::string{};
            in_solid = true;
            continue;
        }

        // endsolid [name...] (name optional/ignored)
        if (eq_ci(toks[0], "endsolid")) {
            in_solid = false;
            break;
        }

        // facet normal i j k
        if (eq_ci(toks[0], "facet")) {
            if (toks.size() < 2 || !eq_ci(toks[1], "normal"))
                return std::unexpected(std::format("Line {}: 'facet' where not expected", line_no));
            if (phase != Phase::idle)
                return std::unexpected(std::format("Line {}: 'facet' where not expected", line_no));
            auto arr = three_floats_from(toks, 2);
            if (!arr) return std::unexpected(std::format("Line {}: {}", line_no, arr.error()));
            current.normal = Vec3{(*arr)[0], (*arr)[1], (*arr)[2]};
            phase = Phase::have_facet;
            continue;
        }

        // outer loop
        if (eq_ci(toks[0], "outer")) {
            if (toks.size() < 2 || !eq_ci(toks[1], "loop"))
                return std::unexpected(std::format("Line {}: unexpected content: '{}'", line_no, std::string(line_trim)));
            if (phase != Phase::have_facet)
                return std::unexpected(std::format("Line {}: 'outer loop' without facet", line_no));
            phase = Phase::in_loop;
            vertex_count = 0;
            continue;
        }

        // vertex x y z
        if (eq_ci(toks[0], "vertex")) {
            if (phase != Phase::in_loop && phase != Phase::have_v1 && phase != Phase::have_v2)
                return std::unexpected(std::format("Line {}: 'vertex' outside of loop", line_no));
            auto arr = three_floats_from(toks, 1);
            if (!arr) return std::unexpected(std::format("Line {}: {}", line_no, arr.error()));

            if (vertex_count == 0)      { current.v[0] = Vec3{(*arr)[0], (*arr)[1], (*arr)[2]}; phase = Phase::have_v1; }
            else if (vertex_count == 1) { current.v[1] = Vec3{(*arr)[0], (*arr)[1], (*arr)[2]}; phase = Phase::have_v2; }
            else if (vertex_count == 2) { current.v[2] = Vec3{(*arr)[0], (*arr)[1], (*arr)[2]}; /* keep Phase::have_v2 */ }
            else {
                return std::unexpected(std::format("Line {}: too many vertices in loop", line_no));
            }
            ++vertex_count;
            continue;
        }

        // endloop
        if (eq_ci(toks[0], "endloop")) {
            if (vertex_count != 3)
                return std::unexpected(std::format("Line {}: 'endloop' before three vertices", line_no));
            continue;
        }

        // endfacet
        if (eq_ci(toks[0], "endfacet")) {
            if (vertex_count != 3 || phase != Phase::have_v2)
                return std::unexpected(std::format("Line {}: 'endfacet' without complete triangle", line_no));
            if (compute_missing_normals) {
                if (std::abs(current.normal.x) + std::abs(current.normal.y) + std::abs(current.normal.z) < 1e-20f) {
                    current.normal = face_normal(current);
                }
            }
            mesh.tris.push_back(current);
            current = Triangle{};
            phase = Phase::idle;
            continue;
        }

        // tolerate repeated "solid name" lines inside (rare but seen)
        if (eq_ci(toks[0], "solid")) {
            mesh.name = (toks.size() > 1) ? join_name(toks, 1) : std::string{};
            continue;
        }

        // Unknown token
        return std::unexpected(std::format("Line {}: unexpected content: '{}'", line_no, std::string(line_trim)));
    }

    if (in_solid) {
        if (phase != Phase::idle)
            return std::unexpected(std::string("Unexpected EOF: unterminated facet/loop"));
    }
    return mesh;
}

std::expected<Mesh, std::string> parse(std::istream& is, bool compute_missing_normals) {
    std::ostringstream oss;
    oss << is.rdbuf();
    if (!is.good() && !is.eof()) {
        return std::unexpected("I/O error while reading stream");
    }
    return parse(oss.str(), compute_missing_normals);
}

std::string serialize(const Mesh& mesh, int float_precision) {
    std::string out;
    out.reserve(std::max<size_t>(128, mesh.tris.size() * 160));
    out += "solid ";
    out += mesh.name;
    out += '\n';
    const auto fmt = std::format("{{:.{}f}}", float_precision);

    auto fmt3 = [&](const Vec3& v){
        return std::format("{} {} {}",
            std::vformat(fmt, std::make_format_args(v.x)),
            std::vformat(fmt, std::make_format_args(v.y)),
            std::vformat(fmt, std::make_format_args(v.z))
        );
    };

    for (const auto& t : mesh.tris) {
        Vec3 n = t.normal;
        // If normal is zero, compute one to keep exporters/readers happy
        if (std::abs(n.x)+std::abs(n.y)+std::abs(n.z) < 1e-20f) {
            n = face_normal(t);
        }
        out += "  facet normal ";
        out += fmt3(n);
        out += '\n';
        out += "    outer loop\n";
        out += "      vertex " + fmt3(t.v[0]) + '\n';
        out += "      vertex " + fmt3(t.v[1]) + '\n';
        out += "      vertex " + fmt3(t.v[2]) + '\n';
        out += "    endloop\n";
        out += "  endfacet\n";
    }
    out += "endsolid ";
    out += mesh.name;
    out += '\n';
    return out;
}

bool serialize(std::ostream& os, const Mesh& mesh, int float_precision) {
    auto s = serialize(mesh, float_precision);
    os.write(s.data(), static_cast<std::streamsize>(s.size()));
    return static_cast<bool>(os);
}

} // namespace STL
