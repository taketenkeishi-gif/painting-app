#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <QImage>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QProcess>
#include <QTimer>

#include "app/ui/ToolDescriptor.h"
#include "app/ui/UiState.h"
#include "core/ai/OnnxSegEngine.h"
#include "core/common/FPoint.h"
#include "core/common/Point.h"
#include "core/common/Rect.h"
#include "core/document/Document.h"
#include "core/render/Renderer.h"
#include "platform/skia/SkiaIntegration.h"
#ifdef PAINT_USE_SKIA
#  include "platform/skia/SkiaRenderer.h"
#endif
#include "core/tools/AiSelectTool.h"
#include "core/tools/BrushTool.h"
#include "core/tools/EraserTool.h"
#include "core/tools/EyedropperTool.h"
#include "core/tools/FillTool.h"
#include "core/tools/GradientTool.h"
#include "core/tools/HandTool.h"
#include "core/tools/ShapeTool.h"
#include "core/tools/FreeTransformTool.h"
#include "core/tools/MoveLayerTool.h"
#include "core/tools/TextTool.h"
#include "core/tools/VectorEditTool.h"
#include "core/tools/RectSelectionTool.h"
#include "core/tools/ToolManager.h"
#include "core/selection/SelectionEngine.h"
#include "core/tools/ZoomTool.h"
#include "platform/qt/HighQualityTransform.h"
#include "core/tools/MeshDeformTool.h"
#include "core/mesh/GridMeshGenerator.h"
#include "core/mesh/EdgeAdaptiveMeshGenerator.h"

namespace app::bridge {
class ComfyUiClient;

struct LayerViewModel {
  std::string name;
  bool visible {true};
  bool active {false};
  int opacityPercent {100};
  core::LayerKind kind {core::LayerKind::Raster};
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool paperLayer {false};
  bool clippedToBelow {false};
  bool hasMask {false};
  bool maskEnabled {false};
  bool locked {false};
  bool alphaLocked {false};
  bool positionLocked {false};
  uint32_t layerId  {0};   ///< 安定ID（0 = 用紙/未採番）
  uint32_t parentId {0};   ///< 親フォルダID（0 = ルート）
  bool selected {false};   ///< マルチ選択セットに含まれているか
};

struct SubToolViewModel {
  std::string id;
  std::string name;
  bool active {false};
  bool enabled {true};
  std::string hint;
};

struct ToolStateViewModel {
  core::Color color {0, 0, 0, 255};
  int size {8};
  int strokeWidth {8};
  int opacity {100};
  int hardness {100};
  int flow {100};
  int spacing {25};
  int angle {0};
  int roundness {100};
  int taperStart {0};
  int taperEnd {0};
  bool antiAlias {true};
  int stabilization {0};
  int snapAngle {0};
  int simplifyLevel {0};
  int fillThreshold {0};
  bool fillContiguous {true};
  bool fillReferAllLayers {false};
  int fillGapClose {0};
  app::ui::SelectionMode selectionMode {app::ui::SelectionMode::Rectangle};
  int autoSelectThreshold {16};
  bool autoSelectContiguous {true};
  bool autoSelectReferAllLayers {true};
  bool postCorrection {false};
  bool velocityBasedCorrection {false};
  core::BrushShapeType shapeType {core::BrushShapeType::Circle};
  core::BlendMode blendMode {core::BlendMode::Normal};
  bool buildupMode {false};
  bool eraseMode {false};
  bool lockAlphaRespect {false};
  app::ui::VectorEraserMode vectorEraseMode {app::ui::VectorEraserMode::TouchedOnly};
  bool vectorTrimOutside {false};
  bool pressureSizeEnabled {false};
  int pressureSizeMin {0};
  bool pressureOpacityEnabled {false};
  int pressureOpacityMin {0};
  // 速度感応
  bool velocitySize       {false};
  int  velocitySizeMin    {30};   ///< 0-100
  bool velocityOpacity    {false};
  int  velocityOpacityMin {30};
  // テクスチャグレイン
  bool textureGrain    {false};
  int  textureStrength {60};   ///< 0-100
  int  textureScale    {100}; ///< 10-400 (÷100 = 0.1-4.0)
  // ウェットミックス / スメア
  bool wetMix    {false};
  int  wetMixRate{50};
  bool smear     {false};
  int  smearRate {90};
  // Dab 散布 / 角度ジッター / 粒子数
  bool scatter           {false};
  int  scatterAmount     {50};   ///< 0-400 (÷100 = 0.0-4.0)
  bool angleJitter       {false};
  int  angleJitterAmount {180};  ///< 0-180 度
  int  dabCount          {1};    ///< 1-64
  // グラデーション
  int  gradientType {0};  ///< 0=Linear, 1=Radial
  int  gradientFill {0};  ///< 0=FgToBg, 1=FgToTransparent
  core::Color secondaryColor {255, 255, 255, 255};  ///< 背景色
  // 選択ツール追加オプション
  int  selectionFeather  {0};
  bool selectionAntiAlias{true};
  core::SelectionOp selectionOp   {core::SelectionOp::New};
  int  selectionExpand   {0};
  int  selectionGapClose {0};
  bool selectionEdgeSnap {false};
};

struct CanvasOverlayViewModel {
  core::ToolOverlayState     toolOverlay;
  const core::SelectionMask* selectionMask {nullptr};

  // FreeTransformTool フローティングプレビュー
  bool   hasTransformPreview  {false};
  QImage transformFloatingImage;
  float  transformCenterX     {0.f};
  float  transformCenterY     {0.f};
  float  transformSx          {1.f};
  float  transformSy          {1.f};
  float  transformRot         {0.f};  ///< ラジアン
  float  transformHalfW       {0.f};
  float  transformHalfH       {0.f};

  // MeshDeformTool プレビュー
  bool   hasMeshDeformPreview  {false};
  QImage meshDeformPreviewImage;
  int    meshDeformPreviewOffX {0};
  int    meshDeformPreviewOffY {0};
};

class AppController : public QObject {
  Q_OBJECT

public:
  explicit AppController(QObject* parent = nullptr);

  const core::Document& document() const noexcept { return m_document; }
  const core::PixelBuffer& compositedBuffer() const noexcept { return m_composited; }
  const core::SelectionMask& documentSelection() const noexcept { return m_document.selection(); }
  std::uint64_t compositeRevision() const noexcept { return m_compositeRevision; }
  bool isDirty() const noexcept { return m_dirty; }
  void markClean() noexcept { setDirty(false); }
  CanvasOverlayViewModel canvasOverlay() const;

  std::vector<LayerViewModel> layerViewModels() const;

  // ── マルチレイヤー選択 ────────────────────────────────────────────────────
  /// 現在の複数選択セットを置き換える。アクティブレイヤーは変更しない。
  void setSelectedLayerIds(const std::unordered_set<uint32_t>& ids);
  /// 現在の複数選択セット（layerId の集合）を返す。
  const std::unordered_set<uint32_t>& selectedLayerIds() const noexcept { return m_selectedLayerIds; }
  /// 複数の layerId に対応するレイヤーをまとめて削除する（フォルダは子ごと削除）。
  bool removeLayersByIds(const std::vector<uint32_t>& ids);
  std::vector<SubToolViewModel> subToolViewModels() const;
  ToolStateViewModel toolState() const noexcept;

  void newDocument(int width, int height, int dpi = 72);
  /// ロード済み Document でアプリ状態を完全置換する（LpaImporter 用）
  void replaceDocument(core::Document doc);
  bool resizeCanvas(int newWidth, int newHeight, int offsetX = 0, int offsetY = 0);
  int  documentDpi() const noexcept { return m_document.dpi(); }
  void setDocumentDpi(int dpi) noexcept { m_document.setDpi(dpi); emit documentChanged(); }
  void addLayer();
  void addRasterLayer();
  void addVectorLayer();
  void addFolderLayer();
  bool setLayerParent(std::size_t layerIndex, uint32_t newParentId);
  bool duplicateLayer(std::size_t index);
  bool duplicateActiveLayer();
  bool removeLayer(std::size_t index);
  bool mergeLayerDown(std::size_t index);
  bool mergeActiveLayerDown();
  bool rasterizeLayer(std::size_t index);
  bool rasterizeActiveLayer();
  bool renameLayer(std::size_t index, const std::string& name);
  bool moveLayer(std::size_t fromIndex, std::size_t toIndex);
  bool moveLayerUp(std::size_t index);
  bool moveLayerDown(std::size_t index);
  bool moveActiveLayerUp();
  bool moveActiveLayerDown();
  void setActiveLayer(std::size_t index);
  void setLayerVisible(std::size_t index, bool visible);
  void setLayerOpacity(std::size_t index, int opacityPercent);
  void setActiveLayerOpacity(int opacityPercent);
  int activeLayerOpacity() const noexcept;
  void setLayerBlendMode(std::size_t index, core::BlendMode mode);
  void setActiveLayerBlendMode(core::BlendMode mode);
  core::BlendMode activeLayerBlendMode() const noexcept;
  bool toggleActiveLayerVisible();
  bool toggleActiveLayerClipToBelow();
  bool toggleActiveLayerMask();
  bool removeActiveLayerMask();
  bool toggleActiveLayerLock();
  bool toggleActiveLayerAlphaLock();
  bool toggleActiveLayerPositionLock();

  // LayerMask Photoshop-style operations
  bool createLayerMaskFromSelection(bool invertMask = false);
  bool deleteLayerMask();
  bool enableLayerMask(bool enable);
  bool invertLayerMask();
  bool applyLayerMask();
  bool clearLayerMask();

  app::ui::UiState::EditTarget editTarget() const noexcept { return m_uiState.editTarget; }
  void setEditTarget(app::ui::UiState::EditTarget target);

  // AdjustmentLayer
  std::size_t addAdjustmentLayerByKind(core::AdjustmentKind kind);
  /// アクティブな調整レイヤーのパラメータを更新し、アンドゥに記録する。
  /// アクティブレイヤーが調整レイヤーでない場合は何もしない。
  void setActiveLayerAdjustmentParams(const core::AdjustmentParams& params);

  bool clearSelection();
  bool selectAll();
  bool deselect();
  bool invertSelection();
  bool expandSelection(int radiusPixels);
  bool contractSelection(int radiusPixels);

  // ── クイックマスクモード（Photoshop 相当） ───────────────────────────────
  bool isQuickMaskMode() const noexcept { return m_quickMaskMode; }
  bool toggleQuickMaskMode();  ///< Alt+Q で切り替え

  bool fillSelectionOrCanvas();
  bool deleteSelectionPixels();
  core::PixelBuffer exportSelectionOrCanvasFromComposite() const;
  void importFlattenedBuffer(const core::PixelBuffer& buffer, const std::string& layerName = "Imported");
  bool pasteBufferAsNewRasterLayer(const core::PixelBuffer& buffer, const std::string& layerName = "Pasted Layer");
  /// 貼り付けた画像をキャンバス外のピクセルも保持したまま変形モードで開く。
  /// Ctrl+V 時に呼び出す。コミット時にキャンバスにラスタライズされる。
  bool pasteBufferAsNewRasterLayerAndTransform(const core::PixelBuffer& buffer, const std::string& layerName = "貼り付けレイヤー");
  bool paperVisible() const noexcept;
  void setPaperVisible(bool visible);
  core::Color paperColor() const noexcept;
  void setPaperColor(const core::Color& color);
  std::optional<core::Rect> consumeDirtyCompositeRect();

  std::vector<core::ToolKind> availableTools() const;
  std::string toolDisplayName(core::ToolKind kind) const;
  bool canUseToolOnActiveLayer(core::ToolKind kind) const;
  bool setCurrentTool(core::ToolKind kind);
  core::ToolKind currentTool() const noexcept;
  /// ToolPanel のボタン強調表示用カテゴリ種別（Hand は MoveLayer に統合）
  core::ToolKind currentToolCategoryKind() const noexcept;
  bool setCurrentSubTool(const std::string& subToolId);
  std::string currentSubToolId() const;
  bool createCurrentSubTool();
  bool duplicateCurrentSubTool();
  bool renameCurrentSubTool(const std::string& displayName);
  bool deleteCurrentSubTool();
  bool resetCurrentSubTool();
  bool saveSubToolSettings();

  std::string currentToolDisplayName() const;
  std::string currentSubToolDisplayName() const;
  std::string currentToolGuide() const;
  std::string activeLayerKindDisplayName() const;
  bool canUseCurrentToolOnActiveLayer() const;
  std::string currentLayerCompatibilityHint() const;

  bool currentToolSupportsColor() const noexcept;
  bool currentToolSupportsSize() const noexcept;
  bool currentToolSupportsOpacity() const noexcept;
  bool currentToolSupportsHardness() const noexcept;
  bool currentToolSupportsFlow() const noexcept;
  bool currentToolSupportsSpacing() const noexcept;
  bool currentToolSupportsAntiAlias() const noexcept;
  bool currentToolSupportsStabilization() const noexcept;
  bool currentToolSupportsPostCorrection() const noexcept;
  bool currentToolSupportsVelocityCorrection() const noexcept;
  bool currentToolSupportsShapeType() const noexcept;
  bool currentToolSupportsAngle() const noexcept;
  bool currentToolSupportsRoundness() const noexcept;
  bool currentToolSupportsTaperStart() const noexcept;
  bool currentToolSupportsTaperEnd() const noexcept;
  bool currentToolSupportsBlendMode() const noexcept;
  bool currentToolSupportsEraseMode() const noexcept;
  bool currentToolSupportsLockAlphaRespect() const noexcept;
  bool currentToolSupportsSnapAngle() const noexcept;
  bool currentToolSupportsSimplifyLevel() const noexcept;
  bool currentToolSupportsVectorEraseMode() const noexcept;
  bool currentToolSupportsVectorTrimOutside() const noexcept;
  bool currentToolSupportsFillThreshold() const noexcept;
  bool currentToolSupportsFillContiguous() const noexcept;
  bool currentToolSupportsFillReferAllLayers() const noexcept;
  bool currentToolSupportsFillGapClose() const noexcept;
  bool currentToolSupportsSelectionMode() const noexcept;
  bool currentToolSupportsAutoSelectThreshold() const noexcept;
  bool currentToolSupportsAutoSelectContiguous() const noexcept;
  bool currentToolHasProperty(app::ui::ToolPropertyKey key) const noexcept;
  bool currentToolSupportsAutoSelectReferAllLayers() const noexcept;

  void beginStroke(int x, int y);
  void continueStroke(int x, int y);
  void endStroke();
  void beginStrokeF(float x, float y, float pressure = 1.0f, float tiltX = 0.0f, float tiltY = 0.0f);
  void continueStrokeF(float x, float y, float pressure = 1.0f, float tiltX = 0.0f, float tiltY = 0.0f);
  /// ダブルクリック（多角形ラッソ確定など）
  void doubleClickAt(float x, float y);
  bool pickColorAt(int x, int y);
  void setInputModifiers(bool shift, bool ctrl, bool alt);

  // ── 自由変形セッション（Ctrl+T） ───────────────────────────────────────────
  bool beginTransformSession();
  bool commitTransformSession();
  bool cancelTransformSession();
  bool isInTransformMode() const noexcept;

  // ── Mesh deform session ─────────────────────────────────────────────────
  bool beginMeshDeformSession();
  bool commitMeshDeformSession();
  bool cancelMeshDeformSession();
  bool isInMeshDeformMode() const noexcept;

  // Pin management (coordinates in canvas space)
  int  meshDeformAddPin(float canvasX, float canvasY);
  void meshDeformMovePin(int id, float canvasX, float canvasY);
  void meshDeformRemovePin(int id);

  // Settings
  void meshDeformSetMode(core::mesh::DeformMode mode);
  core::mesh::DeformMode meshDeformMode() const noexcept;
  void meshDeformSetGridDensity(int rows, int cols);
  void meshDeformSetGeneratorType(int type);  // 0=Grid, 1=EdgeAdaptive
  void meshDeformRegenerateMesh();

  /// VectorEdit ツールで選択中の制御点を削除してアンドゥを記録する。
  bool deleteSelectedVectorPoints();
  /// テキストツール: 入力中かどうか。
  bool isInTextEditMode() const noexcept;
  /// テキストツール: 文字入力を転送する。
  void dispatchTextInput(const std::string& text);
  void dispatchTextBackspace();
  void dispatchTextNewline();
  /// テキストツール: 確定して文字レイヤーを作成する。
  void commitTextEdit();
  void setCanvasZoom(double zoom);
  void setTransformInterpolation(platform::qt::HighQualityTransform::InterpolationMethod method) noexcept {
    m_transformInterpolation = method;
  }

  bool undo();
  bool redo();
  bool canUndo() const noexcept;
  bool canRedo() const noexcept;
  std::string nextUndoActionName() const;
  std::string nextRedoActionName() const;

  void setBrushColor(const core::Color& color);
  void setBrushSize(int size);
  void adjustBrushSize(int delta);
  void setBrushOpacity(int opacity);
  void setBrushHardness(int hardness);
  void setBrushFlow(int flow);
  void setBrushSpacing(int spacing);
  void setBrushAntiAlias(bool antiAlias);
  void setBrushStabilization(int stabilization);
  void setBrushPostCorrection(bool enabled);
  void setBrushVelocityBasedCorrection(bool enabled);
  void setBrushShapeType(core::BrushShapeType shapeType);
  void setBrushAngle(int angle);
  void setBrushRoundness(int roundness);
  void setBrushTaperStart(int taperStart);
  void setBrushTaperEnd(int taperEnd);
  void setBrushBlendMode(core::BlendMode blendMode);
  void setBrushBuildupMode(bool buildup);
  void setBrushEraseMode(bool eraseMode);
  void setBrushLockAlphaRespect(bool enabled);
  void setLineSnapAngle(int snapAngle);
  void setLineSimplifyLevel(int simplifyLevel);
  void setVectorEraseMode(app::ui::VectorEraserMode mode);
  void setVectorTrimOutside(bool enabled);
  void setFillThreshold(int threshold);
  void setFillContiguous(bool contiguous);
  void setFillReferAllLayers(bool enabled);
  void setFillGapClose(int gapClose);
  void setSelectionMode(app::ui::SelectionMode mode);
  void setSelectionOp(core::SelectionOp op);
  void setAutoSelectThreshold(int threshold);
  void setAutoSelectContiguous(bool contiguous);
  void setAutoSelectReferAllLayers(bool enabled);
  void setSelectionFeather(int radius);
  void setSelectionAntiAlias(bool enabled);
  void setSelectionExpand(int pixels);
  void setSelectionGapClose(int radius);
  void setSelectionEdgeSnap(bool enabled);
  void setPressureSizeEnabled(bool enabled);
  void setPressureSizeMin(int value);
  void setPressureOpacityEnabled(bool enabled);
  void setPressureOpacityMin(int value);
  // 速度感応
  void setVelocitySize(bool v);
  void setVelocitySizeMin(int value);
  void setVelocityOpacity(bool v);
  void setVelocityOpacityMin(int value);
  // テクスチャグレイン
  void setTextureGrain(bool v);
  void setTextureStrength(int value);
  void setTextureScale(int value);
  // ウェットミックス / スメア
  void setWetMix(bool v);
  void setWetMixRate(int value);
  void setSmear(bool v);
  void setSmearRate(int value);
  // Dab 散布 / 角度ジッター / 粒子数 (OSS 吸収改善)
  void setScatter(bool v);
  void setScatterAmount(float v);
  void setAngleJitter(bool v);
  void setAngleJitterAmount(float v);
  void setDabCount(int v);

  // ── グラデーション ──────────────────────────────────────────────────────────
  void setSecondaryColor(const core::Color& color);
  core::Color secondaryColor() const noexcept { return m_secondaryColor; }

  // ── 画像調整 ───────────────────────────────────────────────────────────────
  /// アクティブレイヤーの明るさ・コントラストを調整する。
  /// brightness: -100 〜 +100, contrast: -100 〜 +100 (Photoshop 互換)
  bool adjustBrightnessContrast(int brightness, int contrast);
  /// アクティブレイヤーの色相・彩度・明度を調整する。
  /// hue: -180 〜 +180 度, saturation: -100 〜 +100, lightness: -100 〜 +100
  bool adjustHueSaturationLightness(int hue, int saturation, int lightness);

  // ── AI / ComfyUI ──────────────────────────────────────────────────────────
  ComfyUiClient* comfyUiClient() noexcept { return m_comfyUiClient; }
  void connectComfyUi(const QString& url = "http://localhost:8188");
  bool isComfyUiConnected() const noexcept;
  void applyAiSelectResult(core::SelectionMask mask);
  void fetchAiModels();

  // ── ONNX ローカル推論 ─────────────────────────────────────────────────
  bool isOnnxLoaded() const noexcept;
  bool initOnnxEngine(const QString& encoderPath, const QString& decoderPath);
  void setAiSelectGranularity(int granularity);
  int  aiSelectGranularity() const noexcept { return m_onnxGranularity; }

  struct InpaintParams {
    QString prompt;
    QString negativePrompt;
    QString checkpoint  {"v1-5-pruned-emaonly.ckpt"};
    int     steps       {20};
    float   cfg         {7.5f};
    float   denoise     {0.75f};
    int     seed        {-1};
  };
  /// 選択範囲をマスクとしてインペイントを実行。
  /// 選択がない場合は selectionMissing() を emit して返す（全体 inpaint は禁止）。
  void runInpaint(const InpaintParams& params, int batchCount = 1);

  struct Txt2ImgParams {
    QString prompt;
    QString negativePrompt;
    QString checkpoint  {"v1-5-pruned-emaonly.ckpt"};
    int     width       {512};
    int     height      {512};
    int     steps       {20};
    float   cfg         {7.5f};
    int     seed        {-1};
  };
  /// テキストから新規レイヤーに画像を生成
  void runTextToImage(const Txt2ImgParams& params, int batchCount = 1);

  /// ユーザー指定ワークフロー JSON を実行。prompt/seed を注入して batchCount 回キューに追加。
  void runWorkflow(const QJsonObject& workflow,
                   const QString& positivePrompt,
                   const QString& negativePrompt,
                   int seed,
                   const QString& checkpoint,
                   int batchCount = 1);

  /// 実行中の AI 生成をキャンセル
  void cancelAiGeneration();

  /// バッチ候補 Pixmap を新規レイヤーとして貼り付け
  void applyBatchCandidate(const QPixmap& px, const QString& layerName = "AI 生成");

signals:
  void canvasChanged();
  void documentChanged();
  void layersChanged();
  void toolStateChanged();
  void foregroundColorUsed();
  void overlayChanged();
  void comfyUiStateChanged(bool connected);
  void aiSelectionRefined();   ///< ComfyUI 推論で選択が更新されたとき
  void aiModelsLoaded(QStringList models);
  /// step/total ステップ数 + 現在ノード ID
  void aiProgressUpdate(int step, int totalSteps, QString nodeId);
  /// KSampler 中間プレビュー画像
  void aiPreviewReceived(QPixmap preview);
  /// 単一完了 (バッチ 1 or 各バッチ要素)
  void aiGenerationComplete(QString operationType);
  /// バッチ全候補が揃ったとき
  void aiBatchCandidatesReady(QList<QPixmap> candidates);
  void aiGenerationError(QString message);
  /// インペイント時に選択範囲がなかった
  void selectionMissing();
  void dirtyChanged(bool dirty);

private:
  void setDirty(bool dirty) noexcept;
  enum class HistoryKind {
    Stroke,
    LayerVisibility,
    LayerOrder,
    Selection,
    StrokeWithSelection,  ///< ピクセル移動＋選択範囲移動を一括アンドゥするための複合エントリ
    LayerAdd,             ///< レイヤー追加（afterLayer = 追加レイヤー、beforeIndex = 操作前アクティブ）
    LayerRemove,          ///< レイヤー削除（beforeLayer = 削除レイヤー、afterIndex = 削除後アクティブ）
  };

  struct StrokeHistoryEntry {
    HistoryKind kind {HistoryKind::Stroke};
    std::string actionName {"Stroke"};
    std::size_t layerIndex {0};
    /// 安定レイヤーID。0 = 未設定（後方互換エントリ）。
    /// undo/redo 時に ID でレイヤーを検索し、現インデックスを補正する。
    uint32_t layerId {0};
    std::optional<core::Layer> beforeLayer;
    std::optional<core::Layer> afterLayer;
    bool beforeVisible {true};
    bool afterVisible {true};
    std::size_t beforeIndex {0};
    std::size_t afterIndex {0};
    core::SelectionMask beforeSelection;
    core::SelectionMask afterSelection;
  };

  struct PendingStrokeState {
    bool trackPixels {false};
    bool trackSelection {false};
    std::string actionName {"Stroke"};
    std::size_t layerIndex {0};
    uint32_t layerId {0};  ///< ストローク開始時のレイヤー ID
    std::optional<core::Layer> beforeLayer;
    core::SelectionMask beforeSelection;
  };

  /// 安定ID でレイヤーを検索し、現在のフラット配列インデックスを返す。
  /// 見つからない場合は std::nullopt。
  std::optional<std::size_t> findLayerIndexById(uint32_t id) const noexcept;

  /// candidateId が ancestorId の子孫（直接・間接）かどうかを判定する。
  /// parentId チェーンを辿り、循環は layerCount() でキャップして安全に停止する。
  bool isDescendantOf(uint32_t candidateId, uint32_t ancestorId) const noexcept;

  static bool toolWritesPixels(core::ToolKind kind) noexcept;
  static bool toolWritesSelection(core::ToolKind kind) noexcept;
  static std::string actionNameForTool(core::ToolKind kind);
  bool isSubToolCompatibleWithLayerKind(const app::ui::SubToolDescriptor& subTool, core::LayerKind layerKind) const noexcept;
  bool isCurrentSubToolCompatibleWithActiveLayer() const noexcept;
  const app::ui::SubToolDescriptor* firstCompatibleSubTool(core::ToolKind kind, core::LayerKind layerKind) const noexcept;
  void ensureCurrentSubToolCompatibility();

  const app::ui::ToolDescriptor* currentToolDescriptor() const noexcept;
  const app::ui::SubToolDescriptor* currentSubToolDescriptor() const noexcept;
  bool selectSubToolInternal(std::string_view subToolId, bool emitSignal);
  void applyUiStateToTools();
  void resetToolStateFromDescriptor(const app::ui::SubToolDescriptor& subTool);
  void syncCurrentSubToolFromUiState();
  void loadSubToolCatalogFromSettings();
  void saveSubToolCatalogToSettings() const;

  core::ToolContext makeToolContext();
  void applyToolResult(const core::ToolResult& result);
  void finishPendingStrokeHistory();
  void pushHistoryEntry(StrokeHistoryEntry entry);
  void pushSelectionHistoryIfChanged(const core::SelectionMask& before, const std::string& actionName);
  void clearStrokeHistory() noexcept;
  void rerender();
  void rerenderDirty(const core::Rect& dirtyRect);

  core::Document m_document;
#ifdef PAINT_USE_SKIA
  platform::skia::SkiaRenderer m_renderer;
#else
  core::Renderer m_renderer;
#endif
  core::PixelBuffer m_composited;

  core::SelectionEngine m_selectionEngine;
  core::ToolManager m_toolManager;
  core::BrushTool*         m_brushTool         {nullptr};
  core::EraserTool*        m_eraserTool         {nullptr};
  core::LineTool*          m_lineTool           {nullptr};
  core::CurveTool*         m_curveTool          {nullptr};
  core::RectSelectionTool* m_rectSelectionTool  {nullptr};
  core::FillTool*          m_fillTool           {nullptr};
  core::GradientTool*      m_gradientTool       {nullptr};
  core::AiSelectTool*      m_aiSelectTool       {nullptr};
  core::FreeTransformTool* m_freeTransformTool  {nullptr};
  core::VectorEditTool*    m_vectorEditTool     {nullptr};
  core::TextTool*          m_textTool           {nullptr};

  struct TransformSession {
    std::optional<core::Layer> savedLayer;
    core::SelectionMask        savedSelection;
    std::size_t                layerIndex {0};
    QImage                     floatingImage;
  };
  std::optional<TransformSession> m_transformSession;
  platform::qt::HighQualityTransform::InterpolationMethod m_transformInterpolation {
      platform::qt::HighQualityTransform::InterpolationMethod::Bilinear };

  // Mesh deform session
  std::optional<core::MeshDeformTool> m_meshDeformTool;
  core::mesh::MeshGenConfig m_meshGenConfig;
  int m_meshGenType {0};  // 0=Grid, 1=EdgeAdaptive
  core::mesh::GridMeshGenerator     m_gridMeshGen;
  core::mesh::EdgeAdaptiveMeshGenerator m_edgeMeshGen;

  // ── ONNX セグメンテーションエンジン ──────────────────────────────────
  std::unique_ptr<core::ai::OnnxSegEngine> m_onnxSegEngine;
  std::uint64_t m_onnxLastEncodedRevision {static_cast<std::uint64_t>(-1)};
  int           m_onnxGranularity         {1};
  void setupOnnxInferenceCallback();

  // ── ComfyUI サーバー管理 ────────────────────────────────────────────
  void ensureComfyUiRunning(const QUrl& serverUrl);
  void onComfyUiServerStartupTimeout();
  QProcess*    m_comfyUiServerProcess    {nullptr};
  QTimer*      m_comfyUiStartupTimer     {nullptr};
  int          m_comfyUiStartupRetries   {0};

  ComfyUiClient*           m_comfyUiClient      {nullptr};
  enum class AiOpType { None, SamSelect, Inpaint, TextToImage, CustomWorkflow };
  AiOpType                 m_currentAiOp        {AiOpType::None};
  // バッチ追跡
  int                      m_batchCount         {1};
  int                      m_batchRemaining     {0};
  int                      m_batchFired         {0};
  QList<QPixmap>           m_batchImages;
  // 次のバッチ用コールバック (upload 済みの場合に再利用)
  std::function<void()>    m_batchQueueNext;

  app::ui::ToolCatalog m_toolCatalog;
  app::ui::UiState m_uiState;
  std::unordered_map<core::ToolKind, std::string> m_selectedSubToolByTool;
  // ToolPanel ボタン強調用カテゴリ。Hand は MoveLayer に統合されるため別管理。
  core::ToolKind m_activeCategoryKind {core::ToolKind::Brush};

  bool m_stroking {false};
  std::size_t m_layerCounter {1};
  std::optional<PendingStrokeState> m_pendingStroke;
  core::Point m_lastPointer {0, 0};
  core::FPoint m_lastFPointer {0.0f, 0.0f};
  core::Color m_currentColor   {0, 0, 0, 255};
  core::Color m_secondaryColor {255, 255, 255, 255};
  bool m_shiftModifier {false};
  bool m_ctrlModifier {false};
  bool m_altModifier {false};

  std::unordered_set<uint32_t> m_selectedLayerIds;  ///< マルチ選択中の layerId セット

  std::vector<StrokeHistoryEntry> m_undoHistory;
  std::vector<StrokeHistoryEntry> m_redoHistory;
  std::size_t m_maxStrokeHistory {50};
  std::optional<core::Rect> m_lastCompositeDirtyRect;
  std::uint64_t m_compositeRevision {0};
  bool m_dirty {false};

  // ── クイックマスクモード ──────────────────────────────────────────────────
  bool m_quickMaskMode {false};
  core::SelectionMask m_quickMaskSnapshot;  ///< mode 再開時の復帰用
};

} // namespace app::bridge
