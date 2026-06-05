# TASK QUEUE — Painting-app ai-night-test
<!-- scheduler が status を更新します -->

## task-001
status: review_required
priority: high
role: developer

### 目的
ToolPropertyPanel に angle / roundness / taperStart / taperEnd スライダーを追加し、
AppController の既存 setter (setBrushAngle 等) に接続する。
BrushSettings に値は保持されているがパネル UI が未接続のため、PARTIAL 状態を解消する。

### 成功条件
- ToolPropertyPanel に angle/roundness/taperStart/taperEnd の各スライダーが表示される
- スライダー操作時に AppController::setBrushAngle 等が呼ばれ ToolStateViewModel に反映される
- ブラシツール選択時のみ表示、他ツールでは非表示または無効

### 禁止事項
- BrushSettings / AppController / core:: 側の変更（setter は既に実装済み）
- 新規外部依存の追加

### 検証方法
```powershell
cmake --build build --config Release
.\launch.bat   # 起動後、ToolPropertyPanel でスライダーが表示・操作できることを目視確認
```

### review_required_when
- UI変更が含まれるため必須

---

## task-002
status: in_progress
priority: high
role: developer

### 目的
FillTool が SelectionMask がアクティブなとき、選択範囲内のピクセルのみ塗りつぶすように修正する。
現状は SelectionMask を無視して全域塗りつぶしが発生している（system_status: PARTIAL）。

### 成功条件
- 選択範囲が存在するとき、塗りつぶしが選択範囲内のみに適用される
- 選択範囲がない場合は従来どおり全域または contiguous fill が動作する
- undo/redo が正常に機能する

### 禁止事項
- SelectionMask や FillTool の設計変更（既存インターフェースの範囲内で修正）
- fillGapClose / referAllLayers 等の他機能への影響

### 検証方法
```powershell
cmake --build build --config Release
# 起動後: 矩形選択 → Fill → 選択範囲外が塗りつぶされないことを確認
```

### review_required_when
- 既存の動作を変える可能性があるロジック変更のため必須

---

## task-003
status: pending
priority: medium
role: developer

### 目的
BrushSettings.antiAlias フラグを実際に機能させる。
現状 stampCircleAA / drawSegmentAA は呼ばれているが、antiAlias=false のとき
ハードエッジ描画（stampCircle / drawSegment 整数版）への切り替えが未実装。
SPEC Phase 0-4 の「AA ON/OFF 制御」を完成させる。

### 成功条件
- antiAlias=true: 現状と同じ AA 描画（変化なし）
- antiAlias=false: ハードエッジ（Gaussian なし）描画に切り替わる
- ToolPropertyPanel の antiAlias チェックボックスが実際に効果をもつ

### 禁止事項
- Skia 移行・libmypaint 統合（別フェーズ）
- stampCircleAA の削除や大規模リファクタ

### 検証方法
```powershell
cmake --build build --config Release
# 起動後: antiAlias OFF でブラシストロークがハードエッジになることを確認
```

### review_required_when
- 既存の描画動作を変えるロジック変更のため必須

---

## task-004
status: pending
priority: medium
role: developer

### 目的
Navigator パネル (m_infoDock) に「100%」「Fit」クイックズームボタンを追加し、
CanvasWidget::resetZoom / fitToScreen に接続する。
system_status: PARTIAL「navigator mini-canvas preview + quick zoom buttons」を完成させる。

### 成功条件
- Navigator パネルに「100%」「Fit」ボタンが表示される
- クリックで CanvasWidget のズームが切り替わる
- 既存のナビゲータープレビュー表示が壊れない

### 禁止事項
- CanvasWidget / AppController の変更（resetZoom / fitToScreen は既に実装済み）
- Navigator 以外のパネルへの変更

### 検証方法
```powershell
cmake --build build --config Release
.\launch.bat   # Navigator に 100%/Fit ボタンが表示・動作することを目視確認
```

### review_required_when
- UI変更が含まれるため必須

---

## task-005
status: pending
priority: medium
role: developer

### 目的
ColorWheelWidget または color dock ツールバーに FG/BG swap ボタンと
B/W reset ボタンを追加し、MainWindow::onSwapColors / onResetBlackWhiteColors に接続する。
system_status: PARTIAL「color system commands」を Panel ボタンとしても操作できるようにする。

### 成功条件
- color dock 内に swap (↔) ボタンと B/W リセットボタンが表示される
- クリックで既存 Action と同じ動作をする（色が切り替わる）
- 既存のショートカット/メニュー操作と競合しない

### 禁止事項
- AppController や core:: 側の変更（動作は既存 Action に委譲するだけ）
- ColorWheelWidget の HSV 描画ロジックへの変更

### 検証方法
```powershell
cmake --build build --config Release
.\launch.bat   # color dock に swap/reset ボタンが表示・動作することを目視確認
```

### review_required_when
- UI変更が含まれるため必須
