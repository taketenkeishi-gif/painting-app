# UI Restore TODO

**作成日:** 2026-06-15  
**目的:** 過去実装済み機能の復元管理。新規設計は禁止。  
**基準commit/tag:** `golden-core-main-2026-06-15` (8edd409)

---

## 状態定義

| 状態 | 意味 |
|------|------|
| `TODO` | 未着手 |
| `RESTORED` | コード変更完了・ビルド成功。Runtime確認待ち |
| `VERIFIED` | Dev Bridge / 目視で動作確認済み |

---

## LayerPanel

### LP-01: フォルダ展開/折りたたみ（シェブロン▼/▶）

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | a937467より前（削除 by a937467） |
| **対象ファイル** | `src/app/panels/LayerPanel.cpp` / `LayerPanel.h` |
| **復元難度** | 中 |
| **復元内容** | kDepthRole / kExpandedRole / kExpandToggleRole / kIndentWidth=16px 定数追加。paint()でシェブロン描画。editorEvent()でクリック検出。refreshLayers()で深さ計算（parentIdチェーン）と折りたたみ非表示セット計算 |
| **Runtime確認方法** | `/debug/widget-tree` でLayerListのitem数確認。フォルダ追加→chevronクリック→子レイヤー非表示 |

---

### LP-02: レイヤー名インライン編集（QLineEdit editor）

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | a937467より前 |
| **対象ファイル** | `src/app/panels/LayerPanel.cpp` |
| **復元難度** | 低 |
| **復元内容** | LayerItemDelegate に createEditor / setEditorData / setModelData / updateEditorGeometry を追加。QLineEdit スタイル（#1a2030背景 / #edf0f9テキスト / #4e8ef7ボーダー） |
| **Runtime確認方法** | レイヤー名ダブルクリック→QLineEdit表示→名前変更→Enter確定 |

---

### LP-03: マスクサムネイル表示

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | a937467より前 |
| **対象ファイル** | `src/app/panels/LayerPanel.cpp` |
| **復元難度** | 中 |
| **復元内容** | kHasMaskRole / kMaskEnabledRole 追加。paint()でmaskサムネイル描画。mask disabled時の赤X描画。maskThumbnailPixmap() / layerMaskThumbnailRect() 関数追加 |
| **Runtime確認方法** | マスク付きレイヤー作成→レイヤー行にマスクサムネイル表示確認 |

---

### LP-04: イメージ/マスク編集ターゲット切り替え（Shift+クリック）

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | a937467より前 |
| **対象ファイル** | `src/app/panels/LayerPanel.cpp` |
| **復元難度** | 中 |
| **復元内容** | kEditTargetRole（0=Image, 1=Mask）追加。editorEvent()でShift+クリック検出→editTarget toggle |
| **Runtime確認方法** | LP-03が復元済みの状態で: マスクサムネイルShift+クリック→編集ターゲット切替確認 |
| **依存** | LP-03 |

---

### LP-05: 複数レイヤー選択（ExtendedSelection）

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | a937467より前 |
| **対象ファイル** | `src/app/panels/LayerPanel.cpp` |
| **復元難度** | 低 |
| **復元内容** | setSelectionMode(QAbstractItemView::ExtendedSelection)。onLayerItemSelectionChanged()スロット追加 |
| **Runtime確認方法** | Shift+クリックで複数レイヤー選択→両行ハイライト確認 |

---

### LP-06: checkableロックボタン

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | a937467より前 |
| **対象ファイル** | `src/app/panels/LayerPanel.cpp` |
| **復元難度** | 低 |
| **復元内容** | m_clipButton / m_lockButton / m_lockAlphaButton / m_lockPositionButton に setCheckable(true)追加 |
| **Runtime確認方法** | clip/lockボタンクリック→ボタンが押下状態（checked）を保持するか確認 |

---

### LP-07: ブレンドモード全30+種（分類セパレータ付き）

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | a937467より前（Normal/Multiply/Addの3種のみに縮小） |
| **対象ファイル** | `src/app/panels/LayerPanel.cpp` / `src/core/layer/Layer.h`（enum） |
| **復元難度** | 低 |
| **復元内容** | blendModeName() switch拡張: Dissolve / Darken / ColorBurn / LinearBurn / Lighten / Screen / ColorDodge / LinearDodge / Overlay / SoftLight / HardLight / VividLight / LinearLight / PinLight / HardMix / Difference / Exclusion / Subtract / Divide / Hue / HslSat / HslColor / Luminosity。コンボに分類セパレータ（"── 暗くする ──"等）挿入 |
| **Runtime確認方法** | BlendModeコンボ展開→30+種表示・セパレータ確認 |

---

## MainWindow / Dock

### MW-01: ColorSwatch 高品質描画

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 760c942（高品質版）→ a937467でダウンサイズ |
| **対象ファイル** | `src/app/panels/ColorWheelWidget.cpp`（またはColorSwatchWidget） |
| **復元難度** | 中 |
| **復元内容** | FG swatch: 32×32 rounded corner（3px radius）+ shadow。BG swatch: 24×24 at offset(12,12)。checker pattern: 4×4 cell QRectF rounded clip。drop shadow + inner white outline。DPR対応 QImage使用 |
| **Runtime確認方法** | カラースウォッチのビジュアル確認（shadow / rounded corner / checker） |

---

### MW-02: DockTitleBar ハンドカーソル

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 79a6766（導入）→ その後削除 |
| **対象ファイル** | `src/app/mainwindow/MainWindow.cpp`（DockTitleBar class） |
| **復元難度** | 低 |
| **復元内容** | mouseMoveEvent()でArrowCursor / OpenHandCursor切り替え（grip領域でOpenHand、それ以外Arrow） |
| **Runtime確認方法** | DockTitleBarのgrip領域にマウスを移動→カーソルがhand形状に変化 |

---

### MW-03: DockTitleBar ドラッグゾーン最小幅

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 79a6766 |
| **対象ファイル** | `src/app/mainwindow/MainWindow.cpp`（DockTitleBar class） |
| **復元難度** | 低 |
| **復元内容** | m_stretch->setMinimumWidth(40)（ドラッグしやすさ確保） |
| **Runtime確認方法** | `/debug/layout`でDockTitleBarのstretch widthが40px以上か確認 |

---

### MW-04: DockTitleBar sizeHint override

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 79a6766 |
| **対象ファイル** | `src/app/mainwindow/MainWindow.cpp`（DockTitleBar class） |
| **復元難度** | 低 |
| **復元内容** | minimumSizeHint() / sizeHint() override → QSize(54, 22) |
| **Runtime確認方法** | DockTitleBarの最小サイズが54×22以上か確認 |

---

## CanvasWidget

### CW-01: ズーム倍率適応スムース補間

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 760c942（実装）→ その後常にNearestに変更 |
| **対象ファイル** | `src/app/canvasview/CanvasWidget.cpp` |
| **復元難度** | 低 |
| **復元内容** | drawImage()前に `bool smooth = (state.zoom < 8.0)` で判定。setRenderHint(SmoothPixmapTransform, smooth) |
| **Runtime確認方法** | ズーム100%以下でSmooth描画→800%以上でNearest描画の切り替えを目視確認 |

---

### CW-02: FreeTransformフロートプレビュー

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 3f0774a（実装）→ その後削除 |
| **対象ファイル** | `src/app/canvasview/CanvasWidget.cpp` / `src/app/bridge/AppController.cpp` |
| **復元難度** | 高 |
| **復元内容** | overlay.hasTransformPreview / transformFloatingImage / transformIsDistort / transformDistortCorners / perspTransform / affine transform。quadToQuad透視変換ロジック。distort/affine分岐 |
| **Runtime確認方法** | FreeTransformツール選択→コーナーハンドルドラッグ→変換プレビュー表示確認 |
| **注意** | データモデル拡張必要。実装前にスコープ確認必須 |

---

### CW-03: ポリゴンラッソ ベジェ曲線ハンドル

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 760c942より後に削除 |
| **対象ファイル** | `src/app/canvasview/CanvasWidget.cpp` / `src/core/` |
| **復元難度** | 高 |
| **復元内容** | SmoothNode構造（anchor + handleOut FPoint）。polyLassoNodes vector。paint()でcubic-to path描画。ハンドル円形描画（橙=normal / 青=dragging）。ドラッグ中ハンドル編集 |
| **Runtime確認方法** | ポリゴンラッソツール→ノード追加→ドラッグでベジェハンドル表示確認 |
| **注意** | core層にSmoothNode構造の再設計が必要。実装前にスコープ確認必須 |

---

### CW-04: FreeTransform 回転カーソル

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 3f0774aより後に削除 |
| **対象ファイル** | `src/app/canvasview/CanvasWidget.cpp` |
| **復元難度** | 低 |
| **復元内容** | rotateCursor() / transformHandleCursor() 関数復元。3/4円弧 QPainterPath描画 + キャッシュ。mouseMoveEvent()でhandle状態に応じてcursor変更 |
| **Runtime確認方法** | FreeTransformツールのコーナー外側にマウス→回転カーソル表示確認 |

---

### CW-05: VectorEdit / Shapeツール専用カーソル

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 760c942 / 3f0774a |
| **対象ファイル** | `src/app/canvasview/CanvasWidget.cpp` |
| **復元難度** | 低 |
| **復元内容** | cursorForTool() switchにVectorEdit / Shapeのcase追加 |
| **Runtime確認方法** | 各ツール選択時にcanvasカーソルが適切に変化するか確認 |

---

## Tool UI

### TU-01: 選択サブツール専用アイコン描画

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 760c942（drawSelectionIcon実装）→ v2026.04.25-1624-layer-ui で削除 |
| **対象ファイル** | `src/app/panels/SubToolPanel.cpp` |
| **復元難度** | 中 |
| **復元内容** | drawSelectionIcon() / isSelectionSubTool() 関数復元。rect=dashed rect / lasso=bezier curve / poly=polygon / auto_select=sparkle star / object_select=fill rect。SubToolItemDelegate::paint()でisSelectionSubTool()チェック後に呼び出し |
| **Runtime確認方法** | 選択ツール選択→SubToolPanel各サブツールに専用アイコン表示確認 |

---

### TU-02: SubToolPanel カード型レイアウト

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 760c942 |
| **対象ファイル** | `src/app/panels/SubToolPanel.cpp` |
| **復元難度** | 低 |
| **復元内容** | paint()でQPainterPath cardPath rounded rect描画。border色条件分岐（selected / hovered / disabled）。iconW=28 + textRect計算。bold font適用 |
| **Runtime確認方法** | SubToolPanelの各カードにrounded border / テキスト表示確認 |

---

### TU-03: ToolPanel レスポンシブ列数（QScrollArea）

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 760c942（導入）→ その後QScrollArea削除 |
| **対象ファイル** | `src/app/panels/ToolPanel.cpp` |
| **復元難度** | 中 |
| **復元内容** | QScrollArea m_buttonScrollArea追加。setWidgetResizable(true) / setHorizontalScrollBarPolicy(AlwaysOff)。columnCountForWidth()動的計算関数復元。relayoutButtons()でavailableWidth = viewport()->width()使用。QGraphicsOpacityEffectでdisabledツール表示 |
| **Runtime確認方法** | ToolPanelの幅変更→ボタン列数が動的に変化するか確認 |

---

### TU-04: ツールアイコン filled silhouette版

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 760c942（SVGファイル更新）→ その後変更 |
| **対象ファイル** | `src/app/resources/icons/` 以下 SVGファイル |
| **復元難度** | 低（SVG差替のみ） |
| **復元内容** | brush / eraser / eyedropper / fill / select / move / hand / zoom / line の各SVGを760c942版（filled silhouette、#b0b5c8 / #787a90 / #d0d4e0）に差替 |
| **Runtime確認方法** | ToolPanelの各アイコンがfilled silhouette表示になるか確認 |
| **注意** | SVG差替のみ。git show 760c942:src/app/resources/icons/*.svg で復元 |

---

## ショートカット

### SC-01: conflict解消済みショートカット確認

| 項目 | 内容 |
|------|------|
| **状態** | `VERIFIED` |
| **元commit** | c0ae0a9で修正済み |
| **対象ファイル** | `src/app/mainwindow/MainWindow.cpp` |
| **確認結果** | conflictCount=0 (2026-06-15 Dev Bridge確認済み) |
| **Runtime確認方法** | `/debug/input` → conflictCount: 0 |

---

### SC-02: QuickMask Alt+Q ショートカット

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | a937467より前に存在 |
| **対象ファイル** | `src/app/mainwindow/MainWindow.cpp` |
| **復元難度** | 低 |
| **復元内容** | m_quickMaskAction->setShortcut(Qt::ALT | Qt::Key_Q) 追加（m_quickMaskActionが存在する場合） |
| **Runtime確認方法** | `/debug/input` でAlt+Qアクション確認。Alt+Q押下でQuickMask toggle動作確認 |

---

## 復元優先度マトリクス

| 項目 | 難度 | 影響 | 優先 |
|------|------|------|------|
| LP-07 ブレンドモード全種 | 低 | 高 | 🔴 高 |
| LP-06 checkableボタン | 低 | 中 | 🔴 高 |
| CW-01 スムース補間 | 低 | 中 | 🔴 高 |
| LP-01 フォルダ階層 | 中 | 高 | 🟡 中 |
| LP-02 インライン編集 | 低 | 中 | 🟡 中 |
| LP-05 複数選択 | 低 | 中 | 🟡 中 |
| MW-02〜04 DockTitleBar | 低 | 低 | 🟡 中 |
| SC-02 QuickMask shortcut | 低 | 低 | 🟡 中 |
| LP-03 マスクサムネイル | 中 | 高 | 🟡 中（依存あり） |
| LP-04 マスク編集ターゲット | 中 | 高 | 🟢 LP-03後 |
| TU-01 選択サブツールアイコン | 中 | 低 | 🟢 低 |
| TU-02 SubTool card layout | 低 | 低 | 🟢 低 |
| TU-03 ToolPanel responsive | 中 | 低 | 🟢 低 |
| TU-04 filled silhouette icon | 低 | 低 | 🟢 低 |
| MW-01 ColorSwatch高品質 | 中 | 低 | 🟢 低 |
| CW-04 回転カーソル | 低 | 低 | 🟢 低 |
| CW-05 ツール別カーソル | 低 | 低 | 🟢 低 |
| CW-02 FreeTransformプレビュー | 高 | 中 | ⏸️ 保留 |
| CW-03 ポリゴンラッソBezier | 高 | 中 | ⏸️ 保留 |

---

## 禁止事項

- 新規UX案追加（過去実装にない設計）
- CSP/Krita参考の再設計
- 「ついで修正」「整理」
- リファクタリング
- 高難度項目（CW-02/CW-03）の確認なし着手

---

*作成: 2026-06-15 | 基準: golden-core-main-2026-06-15 (8edd409)*
