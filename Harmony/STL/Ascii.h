// ----------------------------------------------------------------------
// Project: Harmony Geometry Serialization Deserialization Library
// Copyright(c) 2025 Onur Tuncer, PhD, Istanbul Technical University
//
// SPDX - License - Identifier : BSD-3-Clause
// License - Filename : LICENSE
// ----------------------------------------------------------------------

#pragma once

// #include <array>
// #include <charconv>
// #include <cmath>
#include <expected>

#include <iosfwd>
// #include <optional>
#include <string>
#include <string_view>
// #include <utility>
// #include <vector>

#include "Mesh.h"

namespace Harmony::STL::ASCII {

/// Parse an ASCII STL from a contiguous text buffer
[[nodiscard]] std::expected<Mesh, std::string>
parse(std::string_view text, bool compute_missing_normals = true) noexcept;

/// Parse from a stream (reads the whole stream text)
[[nodiscard]] std::expected<Mesh, std::string>
parse(std::istream& is, bool compute_missing_normals = true);

/// Serialize to ASCII STL string
[[nodiscard]] std::string serialize(const Mesh& mesh, int float_precision = 6);

/// Serialize to stream (returns false on I/O error)
bool serialize(std::ostream& os, const Mesh& mesh, int float_precision = 6);

} // namespace Harmony::STL::ASCII
