# HANDOFF
更新: 06/06/2026 08:20:00
worker: worker-b
ctx: task-8 完了 → review_required

## 完了タスク: task-8
CanvasWidget マウスイベントの nullptr ガード強化

### 実施内容
`src/app/canvasview/CanvasWidget.cpp` の3つのハンドラに修正:

- `mousePressEvent` / `mouseMoveEvent` / `mouseReleaseEvent` で
  `m_controller == nullptr` の早期 return パスに以下を追加:
  ```cpp
  m_mouseDrawing = false;
  stateFor(this).hasLastStrokeDispatchPos = false;
  ```

### バグの根本原因
`m_controller` がストローク中（`m_mouseDrawing == true`）に null になった場合、
既存の null チェックで early return されるが `m_mouseDrawing` がリセットされず残留。
次に新しいコントローラが設定された際、`beginStroke` を呼ぶ前に
`continueStroke` が呼ばれる状態になりクラッシュの可能性があった。

### 確認済み
- cmake --build Release: エラー0件
- 3つのハンドラ全てで nullptr ガード + state reset を確認（コードレビュー）
- CanvasWidget.h・ツールロジック変更なし（禁止事項厳守）

### 次のworkerへ
- task-9 を確認して次の作業に進む
- UI変更なし → launch.bat 起動確認は省略可
