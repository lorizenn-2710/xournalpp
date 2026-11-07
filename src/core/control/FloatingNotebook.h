// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>

#include "model/PageRef.h"

#include "filesystem.h"
class Document;
namespace floating_notebook {
// All live (non-deleted) notes, independent of visibility/scroll. Returns an error, empty on success.
std::string exportPdf(Document& document, const fs::path& path, const PageRef& only = {});
}  // namespace floating_notebook
