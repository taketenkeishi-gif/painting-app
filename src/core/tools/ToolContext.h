#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "core/buffer/PixelBuffer.h"
#include "core/color/Color.h"
#include "core/common/FPoint.h"
#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/document/Document.h"
#include "core/selection/SelectionEngine.h"

namespace core {

struct ToolPointerEvent {
  Point point;           // 整数キャンバス座標（後方互換）
  FPoint fpoint;         // float精度キャンバス座標
  float pressure {1.0f}; // 筆圧 0.0-1.0
  float tiltX {0.0f};    // ペン傾き X (-60 to +60度)
  float tiltY {0.0f};    // ペン傾き Y
  bool isTablet {false};
  bool shift {false};
  bool ctrl {false};
  bool alt {false};
  bool isDblClick {false}; // ダブルクリック（PolygonLasso確定など）
};

enum class OverlayCursorHint {
  Default,
  Cross,         // 新規選択中
  Move,          // 選択範囲移動中
  AddSelection,  // Shift: 追加
  SubSelection,  // Alt: 減算
};

struct ToolOverlayState {
  bool hasLine {false};
  Point lineStart;
  Point lineEnd;

  bool hasRect {false};
  Rect rect;

  // Polygon / freehand path overlay (e.g. lasso selection preview)
  bool hasPolygon {false};
  bool polygonClosed {false};           // draw as closed polygon vs open path
  std::vector<Point> polygonPoints;

  // 多角形ラッソ: ベジェ対応ノードリスト + マウス追従線
  struct PolyLassoNodeView {
    FPoint anchor;
    FPoint handleOut;  ///< アウトハンドル（アンカー相対）。(0,0) = コーナー
  };
  bool hasPolyLasso {false};
  std::vector<PolyLassoNodeView> polyLassoNodes;   ///< 確定ノード
  FPoint polyLassoMouse {0, 0};                    ///< 現在マウス位置
  bool   polyLassoIsDragging   {false};            ///< ハンドルドラッグ中
  FPoint polyLassoDragAnchor   {0, 0};             ///< ドラッグ中アンカー
  FPoint polyLassoDragHandle   {0, 0};             ///< ドラッグ中ハンドル（アンカー相対）

  // ベクターストロークのライブプレビュー（描画中のみ有効）
  bool hasVectorPreview {false};
  std::vector<FPoint> vectorPreviewPoints;
  Color vectorPreviewColor {0, 0, 0, 255};
  float vectorPreviewWidth {2.0f};

  // ベクター制御点編集オーバーレイ（VectorEditTool使用中）
  bool hasVectorEdit {false};
  std::vector<FPoint> vectorEditPoints;        ///< 全制御点（パス順に平坦化）
  std::vector<int>    vectorEditPointPath;     ///< 各点が属するパスインデックス
  std::vector<bool>   vectorEditPointSelected; ///< 各点の選択状態

  // テキストツール編集オーバーレイ
  bool        hasTextEdit      {false};
  Point       textEditOrigin   {0, 0};  ///< 配置原点（キャンバス座標）
  std::string textEditContent;          ///< 現在入力中のテキスト

  OverlayCursorHint cursorHint {OverlayCursorHint::Default};

  // FreeTransformTool 変形ボックス
  bool   hasTransformBox       {false};
  FPoint transformCorners[4]   {};   // TL TR BR BL (canvas px)
  FPoint transformHandles[9]   {};   // 0-7: スケール, 8: 回転
  int    transformActiveHandle {-1};

  // MeshDeformTool ワイヤーフレームオーバーレイ
  bool hasMeshDeform {false};
  std::vector<core::FPoint> meshDeformVertices;      ///< deformed vertex positions
  std::vector<std::array<int, 3>> meshDeformTris;    ///< triangle index triples
  std::vector<core::FPoint> meshDeformPinCurrents;   ///< current pin positions
  std::vector<core::FPoint> meshDeformPinOriginals;  ///< original pin positions
  int meshDeformActivePin {-1};                      ///< highlighted pin index

  // Rotoブラシ FG/BG ストロークオーバーレイ (AiSelectTool RotoBrush モード)
  struct RotoStroke {
    bool  isForeground {true};  ///< true=前景(緑), false=背景(赤)
    std::vector<FPoint> points;
  };
  bool                    hasRotoStrokes  {false};
  std::vector<RotoStroke> rotoStrokes;             ///< 確定済みストローク
  std::vector<FPoint>     rotoActiveStroke;        ///< 描画中ストローク
  bool                    rotoActiveFg   {true};   ///< 描画中ストロークが前景か
  float                   rotoBrushRadius {8.f};   ///< 表示用ブラシ半径(px)
};

struct ToolResult {
  bool pixelsChanged {false};
  bool selectionChanged {false};
  bool viewportChanged {false};
  std::optional<Color> sampledColor;
  std::optional<Rect> dirtyRect;
};

struct ToolContext {
  Document& document;
  const PixelBuffer& composited;
  Color currentColor;
  Color secondaryColor {255, 255, 255, 255};  ///< 背景色（グラデーション用）
  int brushSize {1};
  bool maskEditMode {false};  ///< true = brush/eraser writes to layer maskBuffer (grayscale)
  SelectionEngine* selectionEngine {nullptr};  ///< 新プロバイダーパイプライン
  Layer*           paintTarget     {nullptr};  ///< 描画先レイヤー (nullptr = activeLayer())
};

} // namespace core
