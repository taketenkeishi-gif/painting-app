# Pen Tablet Input Foundation — 実装状況レポート

**分析日:** 2026-06-08  
**状態:** ✅ **ほぼ完全実装済み**

---

## 📋 実装状況サマリー

### ✅ 既に実装されているもの

| 項目 | 実装位置 | 状態 |
|---|---|---|
| **QTabletEvent ハンドリング** | `CanvasWidget::tabletEvent()` | ✅ 完全実装 |
| **Pressure 取得** | `QTabletEvent::pressure()` | ✅ 実装 |
| **Tilt 取得** | `QTabletEvent::xTilt() / yTilt()` | ✅ 実装 |
| **Core-neutral event 構造** | `ToolPointerEvent` | ✅ 定義済み |
| **Pressure → size dynamics** | `BrushTool::computePressureSize()` | ✅ 実装 |
| **Pressure → opacity dynamics** | `BrushTool::computePressureOpacity()` | ✅ 実装 |
| **UI パネル（筆圧→サイズ）** | `ToolPropertyPanel` | ✅ UI 公開 |
| **Pressure curve** | `BrushCurve` | ✅ 実装 |

---

## 🔍 詳細実装レビュー

### 1. Input Pipeline（CanvasWidget.cpp, 1103-1194 行目）

```cpp
void CanvasWidget::tabletEvent(QTabletEvent* event) {
  // 1. Modifier キーセット
  m_controller->setInputModifiers(
      event->modifiers().testFlag(Qt::ShiftModifier),
      event->modifiers().testFlag(Qt::ControlModifier),
      event->modifiers().testFlag(Qt::AltModifier));
  
  // 2. Pressure 取得
  const float pressure = static_cast<float>(event->pressure());
  
  // 3. イベント型に応じた処理
  switch (event->type()) {
    case QEvent::TabletPress:
      // pressure + tilt 付きで beginStrokeF
      m_controller->beginStrokeF(fpt->x, fpt->y, pressure,
          static_cast<float>(event->xTilt()),
          static_cast<float>(event->yTilt()));
      break;
    case QEvent::TabletMove:
      // continueStrokeF に pressure 渡す
      m_controller->continueStrokeF(fpt->x, fpt->y, pressure,
          static_cast<float>(event->xTilt()),
          static_cast<float>(event->yTilt()));
      break;
    case QEvent::TabletRelease:
      m_controller->endStroke();
      break;
  }
}
```

**評価:** ✅ 完全実装。Pressure / Tilt 取得、Core 中立イベントへの変換正常。

---

### 2. Core-Neutral Event（ToolTypes.h, 16-27 行目）

```cpp
struct ToolPointerEvent {
  Point point;           // 整数キャンバス座標
  FPoint fpoint;         // float 精度
  float pressure {1.0f}; // 筆圧 0.0-1.0 ← ★ Core 中立
  float tiltX {0.0f};    // ペン傾き X
  float tiltY {0.0f};    // ペン傾き Y
  bool isTablet {false}; // タブレット識別フラグ
  bool shift / ctrl / alt;
  bool isDblClick;
};
```

**評価:** ✅ 完全設計。Qt 依存なし、core で使用可能。

---

### 3. AppController ストロークパイプライン（AppController.cpp, 1990-2063）

```cpp
void AppController::beginStrokeF(float x, float y, float pressure, float tiltX, float tiltY) {
  // ...
  core::ToolPointerEvent pressEvent;
  pressEvent.point = m_lastPointer;
  pressEvent.fpoint = m_lastFPointer;
  pressEvent.pressure = std::clamp(pressure, 0.0f, 1.0f); // ★ クランプ
  pressEvent.tiltX = tiltX;
  pressEvent.tiltY = tiltY;
  pressEvent.isTablet = true; // ★ タブレット識別
  pressEvent.shift / ctrl / alt = ...; // Modifier
  core::ToolResult result = m_toolManager.pointerPress(context, pressEvent);
}

void AppController::continueStrokeF(float x, float y, float pressure, float tiltX, float tiltY) {
  // ...
  moveEvent.pressure = std::clamp(pressure, 0.0f, 1.0f);
  moveEvent.isTablet = true;
  core::ToolResult result = m_toolManager.pointerMove(context, moveEvent);
}
```

**評価:** ✅ 完全実装。Pressure クランプ、タブレット識別、Core 呼び出し。

---

### 4. Brush Dynamics — Pressure を Size/Opacity に反映（BrushTool.cpp）

#### computePressureSize（240-250 行目）

```cpp
float BrushTool::computePressureSize(float pressure) const {
  if (!m_settings.dynamics.pressureSize) {
    return 1.0f; // 無効化可能
  }
  const float minRatio = m_settings.dynamics.pressureSizeMin; // 0.1 = 最小 10%
  const auto& curve = m_settings.dynamics.pressureSizeCurve;  // カーブ対応
  const float p = curve.isLinear()
      ? std::clamp(pressure, 0.0f, 1.0f)
      : curve.evaluate(pressure);  // γ カーブ等対応
  return minRatio + (1.0f - minRatio) * p;
}
```

**使用位置:**
- `onPointerMove()`: line 921 → `radius = baseRadius * computePressureSize(pressure)`
- `strokeSegment()`: line 798, 843 → Catmull-Rom 補間時

#### computePressureOpacity（252-262 行目）

```cpp
float BrushTool::computePressureOpacity(float pressure) const {
  // Size と同じロジック、opacity に適用
  // ...
  return minRatio + (1.0f - minRatio) * p;
}
```

**評価:** ✅ 完全実装。圧力カーブ、最小値制御、有効/無効切り替え。

---

### 5. UI Panel（ToolPropertyPanel.cpp）

#### Pressure → Size Widget（158-159, 242-243 行目）

```cpp
m_pressureSizeCheck = new QCheckBox("筆圧→サイズ", this);  // ★ ON/OFF
m_pressureSizeMinSlider = new QSlider(Qt::Horizontal, this);
m_pressureSizeMinSpin = new QSpinBox(this);
// ...
m_pressureSizeMinSlider->setRange(0, 100);  // 0-100% で min 値制御
```

#### 接続（Line 427-431 等）

```cpp
connect(m_pressureSizeCheck, QOverload<int>::of(&QCheckBox::stateChanged),
    this, [this](int state) {
      auto* brush = dynamic_cast<core::BrushTool*>(m_activeTool);
      if (brush) brush->setPressureSizeEnabled(state == Qt::Checked);
      emit brushSettingsChanged();
    });
```

**評価:** ✅ 完全実装。チェック、スライダー、スピンボックスで制御可能。

---

## 🧪 テスト状態

### 実行テスト（要確認）

| テスト項目 | 状態 | 優先度 |
|---|---|---|
| **Tablet pressure → brush size** | 要確認 | 🔴 |
| **Mouse input still works** | 要確認 | 🔴 |
| **Pressure 0 → min size** | 要確認 | 🟡 |
| **Pressure 1.0 → full size** | 要確認 | 🟡 |
| **Pressure curve（γ）動作** | 要確認 | 🟡 |
| **Unit tests pass** | 要確認 | 🟡 |

### 実装の健全性チェック

✅ **Core 非依存:** `ToolPointerEvent` に Qt 依存なし  
✅ **Pressure 計算:** `BrushTool` が core 側で実装  
✅ **Mouse 互換:** `pressure = 1.0f`（デフォルト）で mouse と同じ動作  
✅ **Tablet 識別:** `isTablet` フラグで区別可能  

---

## 🚀 実装完了の3つのステップ

### Step 1: ✅ 完了 — QTabletEvent 取得

**現状:** CanvasWidget::tabletEvent で pressure / tilt 取得済み

```cpp
const float pressure = static_cast<float>(event->pressure());
// ✅ Pressure [0.0, 1.0] で取得
```

### Step 2: ✅ 完了 — Core-neutral input event に変換

**現状:** `ToolPointerEvent` で pressure / isTablet 設定済み

```cpp
pressEvent.pressure = std::clamp(pressure, 0.0f, 1.0f);
pressEvent.isTablet = true;
// ✅ Qt フリーな event 構造体で core に渡す
```

### Step 3: ✅ 完了 — Pressure を brush dynamics に接続

**現状:** `BrushTool::computePressureSize()` で size に反映済み

```cpp
const float sizeMultiplier = computePressureSize(event.pressure);
radius = baseRadius * sizeMultiplier;
// ✅ Pressure がブラシサイズに直接反映
```

---

## 📝 実装仕様の確認

### Pressure 値の正規化

| 段階 | Pressure 値 | 変換 | 説明 |
|---|---|---|---|
| **QTabletEvent** | [0.0, 1.0] | `static_cast<float>()` | Qt raw value |
| **ToolPointerEvent** | [0.0, 1.0] | `clamp()` | Core event |
| **BrushTool** | [0.0, 1.0] | `computePressureSize()` | Curve + min で size 計算 |

### Pressure Curve（BrushCurve）

```cpp
// リニア（デフォルト）
BrushCurve::linear()  // p → p

// γ カーブ（指数）
BrushCurve::soft()    // p^γ (γ≈1.5)
BrushCurve::hard()    // p^γ (γ≈0.67)

// S カーブ（コントラスト）
BrushCurve::sCurve()  // S字カーブ
```

---

## 🔧 テスト実行方法

### 簡易テスト（動作確認）

```bash
# ビルド
cmake --build build --config Release

# 実行
.\launch.bat

# 操作手順:
# 1. ブラシツール選択
# 2. ToolPropertyPanel で「筆圧→サイズ」ON
# 3. ペンタブで描画（圧力変更）
# 4. ブラシサイズが pressure で変わることを確認
```

### ユニットテスト（CI）

```bash
# テストビルド
.\scripts\build-tests.ps1

# テスト実行
ctest --test-dir build-tests --output-on-failure

# 特定テスト
ctest -R BrushTool -V --output-on-failure
```

---

## ⚠️ 既知の制限・今後の拡張

### 現在の制限

1. **Pressure curve UI がない** 
   - 設定ダイアログで curve 選択不可（code-driven のみ）
   - UI 追加は Phase 1+ で検討

2. **Tilt はデータ取得のみ**
   - Brush angle に未接続（`setAngle()` は UI のみ）
   - Tilt → angle 自動接続は Phase 1-D で実装予定

3. **Wintab ネイティブ対応なし**
   - 現在は Qt 抽象化のみ
   - Wintab / Windows Ink は Phase 1-D で検討

### 拡張可能性

✅ **Pressure → opacity** — 実装済み、UI 追加予定  
✅ **Pressure → hardness** — 実装済み、UI 追加予定  
✅ **Pressure → flow** — 実装済み、UI 追加予定  
⏳ **Tilt → angle** — Core 実装済み、UI 接続待ち  
⏳ **Rotation** — `event->rotation()` 取得可能、接続待ち  

---

## 🎯 結論

### ペンタブレット入力基盤は **ほぼ完成**

- ✅ QTabletEvent ハンドリング — **完全実装**
- ✅ Pressure データ取得 — **完全実装**
- ✅ Core-neutral イベント変換 — **完全実装**
- ✅ Pressure → brush size 接続 — **完全実装**
- ✅ UI パネル — **完全実装**

### 実行テストが必須

現在のコード構造では、実装は完全ですが、**実行時の動作確認が必要**です。

#### テストシナリオ

```
1. Tablet pressure affects brush stroke ← テスト必要
2. Mouse input still works ← テスト必要
3. Existing tests pass ← テスト実行必要
```

---

## 次のアクション

### 短期（このセッション）

1. **テスト実行** — 実装動作確認
2. **バグ修正**（あれば）
3. **ドキュメント更新**

### 中期（Phase 1-D）

1. **UI 拡張** — Pressure curve selector
2. **Tilt 接続** — Tilt → brush angle
3. **Wintab 準備** — Windows Ink API 統合
4. **Pressure curve UI** — ディアログ実装

