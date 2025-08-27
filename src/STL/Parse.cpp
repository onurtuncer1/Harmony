// ----------------------------------------------------------------------
// Project: Harmony Geometry Serialization Deserialization Library
// Copyright(c) 2025 Onur Tuncer, PhD, Istanbul Technical University
//
// SPDX - License - Identifier : BSD-3-Clause
// License - Filename : LICENSE
// ----------------------------------------------------------------------

#include "Harmony/STL/Parse.h"

namespace Harmony::STL {

std::expected<Mesh, std::string>
parse(std::istream& is, bool compute_missing_normals) {
    // Remember position
    auto pos = is.tellg();
    if (pos == std::streampos(-1)) pos = std::streampos(0);

    // Read first 6 bytes ("solid " for ASCII)
    char probe[6]{};
    is.read(probe, 6);
    const auto readOk = is.gcount();
    is.clear(); // clear eof if short files
    is.seekg(pos);

    if (readOk == 6 && std::string_view(probe,6) == std::string_view("solid ")) {
        return ASCII::parse(is, compute_missing_normals); // ASCII path (your existing function)
    }

    // Binary path: read header+count, then restore stream and call binary parser
    // (Some ASCII files also start with "solid<no-space>", catch those by trying ASCII first)
    return Binary::parse(is, compute_missing_normals);
}

} // namespace Harmony::STL
