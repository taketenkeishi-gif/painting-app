#pragma once

#include <array>
#include <cstddef>

#include "core/layer/Layer.h"

namespace core::registry {

enum class LayerRenderRoute {
  None,
  RasterBuffer,
  VectorPaths
};

class RendererRegistry {
public:
  RendererRegistry() noexcept;

  void setRoute(LayerKind kind, LayerRenderRoute route) noexcept;
  LayerRenderRoute routeFor(const Layer& layer) const noexcept;

private:
  static std::size_t toIndex(LayerKind kind) noexcept;

  std::array<LayerRenderRoute, 3> m_routes;
};

} // namespace core::registry
