// ----------------------------------------------------------------------
// Project: Harmony Geometry Serialization Deserialization Library
// Copyright(c) 2025 Onur Tuncer, PhD, Istanbul Technical University
//
// SPDX - License - Identifier : BSD-3-Clause
// License - Filename : LICENSE
// ----------------------------------------------------------------------

#pragma once

#include <expected>
#include <iosfwd>
#include <string>

#include "Mesh.h"
#include "Ascii.h"
#include "Binary.h"

namespace Harmony::STL {

    std::expected<Mesh, std::string> parse(std::istream& is, bool compute_missing_normals);

} // namespace Harmony::STL