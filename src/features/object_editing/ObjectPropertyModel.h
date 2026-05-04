#pragma once

#include <optional>
#include <string>

namespace features::object_editing {

struct TextProperties {
  std::string text;
  std::string font;
  int size {16};
  bool vertical {false};
  int align {0};
};

struct RulerProperties {
  float angleDeg {0.0F};
  bool snapEnabled {true};
};

struct ComicProperties {
  int strokeWidth {1};
  int cornerRadius {0};
  bool balloonTail {false};
};

struct ObjectPropertyModel {
  std::optional<TextProperties> text;
  std::optional<RulerProperties> ruler;
  std::optional<ComicProperties> comic;
};

} // namespace features::object_editing

