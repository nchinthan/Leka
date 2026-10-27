#pragma once
#include "document/DocumentOperation.h"
#include "renderer/DirtyRegion.h"
#include "state/EditorContext.h"

namespace ced::cursor {

std::size_t offsetOf(const EditorContext& ctx);                       // document offset of the cursor
void setFromOffset(EditorContext& ctx, std::size_t offset);           // line/col from a document offset
void clamp(EditorContext& ctx);                                       // keep the cursor inside the document
DirtyRegion dirtyFor(const IDocumentView& doc, const ChangeInfo& c);  // invalidation for an edit

} // namespace ced::cursor
