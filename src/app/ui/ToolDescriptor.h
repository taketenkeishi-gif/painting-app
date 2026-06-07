#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/selection/SelectionMask.h"
#include "core/tools/ToolTypes.h"
#include "core/tools/ToolType.h"

namespace app::ui {

enum class TargetLayerKind {
  Raster,
  Vector,
  Both
};

enum class CursorStyle {
  Default,
  Brush,
  Cross,
  Hand,
  Zoom,
  Fill
};

enum class ToolPropertyKey {
  Color,
  Size,
  Opacity,
  Hardness,
  Flow,
  Spacing,
  AntiAlias,
  Stabilization,
  PostCorrection,
  VelocityCorrection,
  ShapeType,
  Angle,
  Roundness,
  TaperStart,
  TaperEnd,
  StrokeWidth,
  SnapAngle,
  SimplifyLevel,
  VectorEraseMode,
  VectorTrimOutside,
  BlendMode,
  EraseMode,
  LockAlphaRespect,
  FillThreshold,
  FillContiguous,
  FillReferAllLayers,
  FillGapClose,
  SelectionMode,
  SelectionOp,
  AutoSelectThreshold,
  AutoSelectContiguous,
  AutoSelectReferAllLayers,
  SelectionFeather,
  SelectionAntiAlias,
  SelectionExpand,
  SelectionGapClose,
  SelectionEdgeSnap
};

struct StrokeSettings {
  int size {8};
  int opacity {100};
  int flow {100};
  int spacing {25};
  bool antiAlias {true};
};

struct BrushShapeSettings {
  core::BrushShapeType shapeType {core::BrushShapeType::Circle};
  int hardness {100};
  int angle {0};
  int roundness {100};
  int taperStart {0};
  int taperEnd {0};
};

struct StabilizerSettings {
  int stabilization {0};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};
};

struct VectorStrokeSettings {
  int strokeWidth {8};
  int simplifyLevel {0};
  int snapAngle {0};
};

struct FillSettings {
  int threshold {0};
  bool contiguous {true};
  bool referAllLayers {false};
  int gapClose {0};
};

enum class SelectionMode {
  Rectangle,
  Lasso,
  PolygonLasso,  ///< クリック頂点追加、ダブルクリック確定
  AutoSelect,
  ObjectSelect,  ///< 全体類似色 / SAM2
};

enum class VectorEraserMode {
  TouchedOnly,
  ToIntersection,
  TrimOutside
};

struct SelectionSettings {
  SelectionMode mode {SelectionMode::Rectangle};
  core::SelectionOp op {core::SelectionOp::New};
  int autoSelectThreshold {16};
  bool autoSelectContiguous {true};
  bool autoSelectReferAllLayers {true};
  int featherRadius {0};
  bool antiAlias {true};
  int expandPixels {0};
  int gapCloseRadius {0};
  bool edgeSnap {false};
};

struct ToolBehaviorProfile {
  StrokeSettings stroke;
  BrushShapeSettings shape;
  StabilizerSettings stabilizer;
  VectorStrokeSettings vector;
  FillSettings fill;
  SelectionSettings selection;
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
  VectorEraserMode vectorEraseMode {VectorEraserMode::TouchedOnly};
  bool vectorTrimOutside {false};
  TargetLayerKind targetLayerKind {TargetLayerKind::Both};
  CursorStyle cursorStyle {CursorStyle::Default};
};

struct BrushPreset {
  // ── コンストラクタ（旧形式互換）─────────────────────────────────────────
  // 既存の旧フォーマット初期化リストとの互換性を保つため
  BrushPreset() = default;
  BrushPreset(
      int sz, int op, int hardnss, int flw, int spc, bool aa, int stab,
      bool pc, bool vpc, core::BrushShapeType st, core::BlendMode bm,
      bool em, bool la, int ang, int rnd, int ts, int te,
      TargetLayerKind tk, CursorStyle cur,
      int snap_ang = 0, int simpl_lv = 0, int stroke_w = 0)
      : size(sz), opacity(op), hardness(hardnss), flow(flw), spacing(spc),
        antiAlias(aa), stabilization(stab), postCorrection(pc),
        velocityBasedCorrection(vpc), shapeType(st), angle(ang), roundness(rnd),
        taperStart(ts), taperEnd(te), blendMode(bm), eraseMode(em),
        lockAlphaRespect(la), snapAngle(snap_ang), simplifyLevel(simpl_lv),
        strokeWidth(stroke_w), targetLayerKind(tk), cursorStyle(cur) {}

  // ── ストローク基本 ──────────────────────────────────────────────────────
  int size {8};
  int opacity {100};
  int hardness {100};
  int flow {100};
  int spacing {25};
  bool antiAlias {true};
  int stabilization {0};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};

  // ── 形状パラメータ ──────────────────────────────────────────────────────
  core::BrushShapeType shapeType {core::BrushShapeType::Circle};
  int angle {0};
  int roundness {100};
  int taperStart {0};
  int taperEnd {0};

  // ── 筆圧応答 ──────────────────────────────────────────────────────────
  bool velocitySize {false};
  int velocitySizeMin {30};
  bool velocityOpacity {false};
  int velocityOpacityMin {30};

  // ── テクスチャ＆湿潤 ──────────────────────────────────────────────────
  bool textureGrain {false};
  int textureStrength {60};
  int textureScale {100};
  bool wetMix {false};
  int wetMixRate {50};
  bool smear {false};
  int smearRate {90};

  // ── Dab 散布 ──────────────────────────────────────────────────────────
  bool scatter {false};
  int scatterAmount {50};
  bool angleJitter {false};
  int angleJitterAmount {180};
  int dabCount {1};

  // ── ブレンド＆書き込み ────────────────────────────────────────────────
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
  bool buildupMode {false};   // Krita積み上げ OFF / Photoshop ON

  // ── ベクターストローク ────────────────────────────────────────────────
  TargetLayerKind targetLayerKind {TargetLayerKind::Both};

  // ── UI ────────────────────────────────────────────────────────────────
  CursorStyle cursorStyle {CursorStyle::Default};

  // ── ツール固有（Line/Curve/Fill/Selection など）────────────────────
  int snapAngle {0};
  int simplifyLevel {0};
  int strokeWidth {0};
  int fillThreshold {0};
  bool fillContiguous {true};
  bool fillReferAllLayers {false};
  int fillGapClose {0};
  SelectionMode selectionMode {SelectionMode::Rectangle};
  core::SelectionOp selectionOp {core::SelectionOp::New};
  int autoSelectThreshold {16};
  bool autoSelectContiguous {true};
  bool autoSelectReferAllLayers {true};
  int selectionFeather {0};
  bool selectionAntiAlias {true};
  int selectionExpand {0};
  int selectionGapClose {0};
  bool selectionEdgeSnap {false};
  VectorEraserMode vectorEraseMode {VectorEraserMode::TouchedOnly};
  bool vectorTrimOutside {false};

  // ── グラデーション ────────────────────────────────────────────────────
  int gradientType {0};       // 0=Linear, 1=Radial
  int gradientFill {0};       // 0=ForegroundToBackground, 1=ForegroundToTransparent
};

struct SubToolDescriptor {
  std::string id;
  std::string displayName;
  BrushPreset preset;
  ToolBehaviorProfile profile;
  std::vector<ToolPropertyKey> editableProperties;
  std::string guide;
  // サブツール選択時に実際に起動する ToolKind。未設定なら親 ToolDescriptor::kind を使う。
  std::optional<core::ToolKind> targetToolKind;
};

struct ToolDescriptor {
  core::ToolKind kind {core::ToolKind::Brush};
  std::string id;
  std::string displayName;
  std::vector<SubToolDescriptor> subTools;
  std::vector<ToolPropertyKey> availableProperties;
  std::string guide;
  std::string shortcut;  ///< keyboard shortcut key label (e.g. "B"), empty if none
};

class ToolCatalog {
public:
  ToolCatalog();

  const std::vector<ToolDescriptor>& tools() const noexcept { return m_tools; }
  const ToolDescriptor* findTool(core::ToolKind kind) const noexcept;
  ToolDescriptor* findToolMutable(core::ToolKind kind) noexcept;
  const SubToolDescriptor* findSubTool(core::ToolKind kind, std::string_view subToolId) const noexcept;
  SubToolDescriptor* findSubToolMutable(core::ToolKind kind, std::string_view subToolId) noexcept;
  const SubToolDescriptor* defaultSubTool(core::ToolKind kind) const noexcept;
  bool duplicateSubTool(core::ToolKind kind, std::string_view sourceSubToolId, const std::string& newDisplayName);
  bool createSubTool(core::ToolKind kind, const std::string& newDisplayName);
  bool renameSubTool(core::ToolKind kind, std::string_view subToolId, const std::string& newDisplayName);
  bool removeSubTool(core::ToolKind kind, std::string_view subToolId);
  bool resetSubTool(core::ToolKind kind, std::string_view subToolId);

private:
  static std::string makeSubToolId(std::string_view baseId, const std::vector<SubToolDescriptor>& existing);

  std::vector<ToolDescriptor> m_defaultTools;
  std::vector<ToolDescriptor> m_tools;
};

} // namespace app::ui
