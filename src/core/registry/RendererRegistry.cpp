#include "core/registry/RendererRegistry.h"

namespace core::registry {

RendererRegistry::RendererRegistry() noexcept
    : m_routes {LayerRenderRoute::RasterBuffer, LayerRenderRoute::VectorPaths, LayerRenderRoute::None} {}

void RendererRegistry::setRoute(LayerKind kind, LayerRenderRoute route) noexcept {
  m_routes[toIndex(kind)] = route;
}

LayerRenderRoute RendererRegistry::routeFor(const Layer& layer) const noexcept {
  return m_routes[toIndex(layer.kind())];
}

std::size_t RendererRegistry::toIndex(LayerKind kind) noexcept {
  switch (kind) {
    case LayerKind::Raster:
      return 0;
    case LayerKind::Vector:
      return 1;
    case LayerKind::Folder:
    default:
      return 2;
  }
}

} // namespace core::registry
