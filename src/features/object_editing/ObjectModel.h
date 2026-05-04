#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/tools/ToolTypes.h"

namespace features::object_editing {

enum class ObjectKind {
  Text,
  Ruler,
  Comic,
  VectorPath
};

struct ObjectTransform {
  float tx {0.0F};
  float ty {0.0F};
  float sx {1.0F};
  float sy {1.0F};
  float rotationDeg {0.0F};
};

struct ObjectStyle {
  core::Color strokeColor {0, 0, 0, 255};
  core::Color fillColor {0, 0, 0, 0};
  int strokeWidth {1};
  float opacity {1.0F};
};

struct TextPayload {
  std::string text;
  std::string fontFamily {"Yu Gothic UI"};
  int fontSize {16};
  bool vertical {false};
  int align {0};
};

struct RulerPayload {
  core::Point start {0, 0};
  core::Point end {0, 0};
  bool snapEnabled {true};
  bool guideVisible {true};
};

struct ComicPayload {
  int cornerRadius {0};
  bool hasBalloonTail {false};
  std::vector<core::Point> tailPoints;
};

struct VectorPathPayload {
  std::vector<core::Point> points;
};

using ObjectPayload = std::variant<TextPayload, RulerPayload, ComicPayload, VectorPathPayload>;

struct ObjectModel {
  std::string id;
  std::string layerId;
  ObjectKind kind {ObjectKind::VectorPath};
  core::Rect bounds {0, 0, 0, 0};
  ObjectTransform transform;
  bool visible {true};
  bool locked {false};
  ObjectStyle style;
  ObjectPayload payload;
};

core::Rect boundsFromPayload(const ObjectPayload& payload, const core::Rect& fallback) noexcept;

} // namespace features::object_editing

