#pragma once

#include "core/buffer/PixelBuffer.h"
#include "core/common/Rect.h"
#include "core/document/Document.h"
#include "core/registry/RendererRegistry.h"

namespace core {

class Renderer {
public:
  PixelBuffer composite(const Document& document) const;
  void compositeInto(const Document& document, PixelBuffer& target, const Rect& dirtyRect) const;

  registry::RendererRegistry& registry() noexcept { return m_registry; }
  const registry::RendererRegistry& registry() const noexcept { return m_registry; }

private:
  registry::RendererRegistry m_registry;
};

} // namespace core
