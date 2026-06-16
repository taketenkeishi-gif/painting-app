# UI Restore TODO

**作成日:** 2026-06-15  
**最終更新:** 2026-06-15 (LP-02〜LP-06 Dev_Bridge coverage追加・DEV_BRIDGE_VERIFIED昇格)  
**目的:** 過去実装済み機能の復元管理。新規設計は禁止。  
**基準commit/tag:** `golden-core-main-2026-06-15` (8edd409)

---

## 状態定義

| 状態 | 意味 |
|------|------|
| `TODO` | 未着手 |
| `ALREADY_PRESENT` | 調査の結果、現HEADに既に実装済みと確認 |
| `RESTORED` | コード変更完了・ビルド成功。Runtime確認待ち |
| `VERIFIED` | Dev Bridge / 目視で動作確認済み |
| `DEV_BRIDGE_VERIFIED` | Dev_Bridge action/state で動作変化を確認済み |
| `NEEDS_VISUAL_CONFIRM` | Dev_Bridge action/state で到達不能。ユーザー目視確認待ち |
| `RUNTIME_FAIL` | Runtime実行で動作不確認 |

---

## 調査結果サマリー (2026-06-15)

Restore Loop 実行中に全項目をソース確認した結果、
**CW-01 を除く全項目が現HEAD(05332ff)に既に実装済み**であることを確認。

過去の監査(UI欠落リスト)は `git show` / `git diff` ではなく
静的推測に基づいていたため、一部誤判定が含まれていた。

実際に欠落していた項目: **CW-01のみ** → 復元完了 (commit: 971e66a)

---

## LayerPanel

### LP-01: フォルダ展開/折りたたみ（シェブロン▼/▶）

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | kDepthRole/kExpandedRole/kExpandToggleRole/kIndentWidth/chevron描画/m_collapsedFolderIds 全て実装済み |
| **確認commit** | HEAD (05332ff) |

---

### LP-02: レイヤー名インライン編集（QLineEdit editor）

| 項目 | 内容 |
|------|------|
| **状態** | `DEV_BRIDGE_VERIFIED` |
| **静的確認** | createEditor/setEditorData/setModelData/updateEditorGeometry 全て実装済み (HEAD 05332ff) |
| **Dev_Bridge確認** | `rename-layer` action 追加済み。index=0, name="TestLayer" → layers.names[0]="TestLayer" 変化確認 (2026-06-15) |

---

### LP-03: マスクサムネイル表示

| 項目 | 内容 |
|------|------|
| **状態** | `DEV_BRIDGE_VERIFIED` |
| **静的確認** | kHasMaskRole/kMaskEnabledRole/maskThumbnailPixmap/赤X描画 全て実装済み (HEAD 05332ff) |
| **Dev_Bridge確認** | `add-layer-mask` action 追加済み。layers.hasMask[0]: false→true 変化確認。activeHasMask=true 一致確認 (2026-06-15) |

---

### LP-04: イメージ/マスク編集ターゲット切り替え（Shift+クリック）

| 項目 | 内容 |
|------|------|
| **状態** | `DEV_BRIDGE_VERIFIED` |
| **静的確認** | kEditTargetRole (0=Image, 1=Mask) / Shift+クリック検出 実装済み (HEAD 05332ff) |
| **Dev_Bridge確認** | `set-edit-target` action 追加済み (mode=0/1)。layers.editTarget: 0→1 変化確認 (2026-06-15) |

---

### LP-05: 複数レイヤー選択（ExtendedSelection）

| 項目 | 内容 |
|------|------|
| **状態** | `DEV_BRIDGE_VERIFIED` |
| **静的確認** | setSelectionMode(ExtendedSelection) / onLayerItemSelectionChanged() 実装済み (HEAD 05332ff) |
| **Dev_Bridge確認** | `select-layers` action 追加済み (indices=[0,1])。layers.selected: [false,false]→[true,true] 変化確認 (2026-06-15) |

---

### LP-06: checkableロックボタン

| 項目 | 内容 |
|------|------|
| **状態** | `DEV_BRIDGE_VERIFIED` |
| **静的確認** | setCheckable(true)/setChecked() が clip/lock/lockAlpha/lockPosition 全ボタンに実装済み (HEAD 05332ff) |
| **Dev_Bridge確認** | `set-layer-lock` action 追加済み。layers.locked[0]: false→true 変化確認。`/debug/components` dock-button に `checkable`/`checked` 出力追加 (16個の checkable button 確認) (2026-06-15) |

---

### LP-07: ブレンドモード全30+種（分類セパレータ付き）

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | blendModeName() 26種 + コンボに分類セパレータ（暗くする/明るくする/コントラスト/比較/HSL）実装済み |
| **確認commit** | HEAD (05332ff) |

---

## MainWindow / Dock

### MW-01: ColorSwatch 高品質描画

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | 48×48 / checker(4px cell rounded clip) / drop shadow / inner white outline / DPR対応 QImage 全て実装済み |
| **確認commit** | HEAD (05332ff) |

---

### MW-02: DockTitleBar ハンドカーソル

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | mouseMoveEvent() で ArrowCursor/OpenHandCursor 切り替え実装済み |
| **確認commit** | HEAD (05332ff) |

---

### MW-03: DockTitleBar ドラッグゾーン最小幅

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | m_stretch->setMinimumWidth(40) 実装済み（コメント付き） |
| **確認commit** | HEAD (05332ff) |

---

### MW-04: DockTitleBar sizeHint override

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | minimumSizeHint()/sizeHint() → QSize(54, 22) 実装済み |
| **確認commit** | HEAD (05332ff) |

---

## CanvasWidget

### CW-01: ズーム倍率適応スムース補間

| 項目 | 内容 |
|------|------|
| **状態** | `RESTORED` |
| **元commit** | 760c942 |
| **対象ファイル** | `src/app/canvasview/CanvasWidget.cpp` |
| **復元内容** | `const bool smooth = (state.zoom < 8.0)` で条件分岐。SmoothPixmapTransform を動的切り替え |
| **復元commit** | `971e66a` |
| **Runtime確認方法** | ズーム100%以下でSmooth描画→800%以上でNearest描画の切り替えを目視確認 |

---

### CW-02: FreeTransformフロートプレビュー

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | overlay.hasTransformPreview / transformFloatingImage / rotateCursor / transformHandleCursor 全て実装済み |
| **確認commit** | HEAD (05332ff) |

---

### CW-03: ポリゴンラッソ ベジェ曲線ハンドル

| 項目 | 内容 |
|------|------|
| **状態** | `TODO` |
| **元commit** | 760c942より後に削除 |
| **対象ファイル** | `src/app/canvasview/CanvasWidget.cpp` / `src/core/` |
| **復元難度** | 高 |
| **復元内容** | SmoothNode構造（anchor + handleOut FPoint）/ cubic-to path描画 / ドラッグハンドル編集 |
| **Runtime確認方法** | ポリゴンラッソ→ノード追加→ドラッグでベジェハンドル表示確認 |
| **注意** | core層にSmoothNode構造の再設計が必要。実装前にユーザー確認必須 |

---

### CW-04: FreeTransform 回転カーソル

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | rotateCursor() / transformHandleCursor() 実装済み。mouseMoveEvent()で使用 |
| **確認commit** | HEAD (05332ff) |

---

### CW-05: VectorEdit / Shapeツール専用カーソル

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | cursorForTool() にVectorEdit/Shapeのcase実装済み |
| **確認commit** | HEAD (05332ff) |

---

## Tool UI

### TU-01: 選択サブツール専用アイコン描画

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | isSelectionSubTool() / drawSelectionIcon() 実装済み。SubToolItemDelegate::paint()で呼び出し済み |
| **確認commit** | HEAD (05332ff) |

---

### TU-02: SubToolPanel カード型レイアウト

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | QPainterPath cardPath rounded rect / border色条件分岐(selected/hovered) 実装済み |
| **確認commit** | HEAD (05332ff) |

---

### TU-03: ToolPanel レスポンシブ列数（QScrollArea）

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | QScrollArea / setWidgetResizable(true) / columnCountForWidth() / relayoutButtons() 全て実装済み |
| **確認commit** | HEAD (05332ff) |

---

### TU-04: ツールアイコン filled silhouette版

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | SVGファイルが760c942版と同一内容（filled silhouette、#b0b5c8/#b0b5c8/#787a90パレット）を確認 |
| **確認commit** | HEAD (05332ff) |

---

## ショートカット

### SC-01: conflict解消済みショートカット確認

| 項目 | 内容 |
|------|------|
| **状態** | `VERIFIED` |
| **確認結果** | conflictCount=0 (2026-06-15 Dev Bridge確認済み) |

---

### SC-02: QuickMask Alt+Q ショートカット

| 項目 | 内容 |
|------|------|
| **状態** | `ALREADY_PRESENT` |
| **確認内容** | m_quickMaskAction->setShortcut(Qt::ALT \| Qt::Key_Q) 実装済み |
| **確認commit** | HEAD (05332ff) |

---

## 残課題

### 継続TODO（高難度・要ユーザー確認）

| ID | 機能 | 理由 |
|----|------|------|
| CW-01 | ズーム補間 | RESTORED — Runtime目視確認待ち |
| CW-03 | ポリゴンラッソ ベジェ | core層SmoothNode構造が必要。ユーザー確認後に着手 |

---

## 禁止事項

- 新規UX案追加（過去実装にない設計）
- CSP/Krita参考の再設計
- 「ついで修正」「整理」
- リファクタリング

---

*作成: 2026-06-15 | 最終更新: 2026-06-15 | 基準: golden-core-main-2026-06-15 (8edd409)*
