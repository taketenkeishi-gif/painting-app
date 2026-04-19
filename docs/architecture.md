# Architecture (Phase 1 MVP)

## 目的

レイヤー付きキャンバス基盤を、UI とコアを分離した状態で成立させる。

## 層構造

- `app` (Qt依存):
  - `MainWindow`: メニュー・レイアウト
  - `CanvasWidget`: 表示とマウス入力
  - `LayerPanel`: レイヤー一覧と操作
  - `AppController`: UIイベントをコア操作へ変換
- `core` (Qt非依存):
  - `PixelBuffer`, `Layer`, `Document`, `Renderer`, `BrushTool`
- `platform/qt`:
  - `QtImageConverter`: `PixelBuffer` -> `QImage`

## データフロー

1. UI が入力を受ける
2. `AppController` がコア操作へ変換
3. `Renderer` が全レイヤー合成
4. `QtImageConverter` が表示用画像へ変換
5. `CanvasWidget` が再描画

## 設計ルール

- `core` に Qt 型を入れない
- `app` から `core` の内部配列を直接触らない
- 将来の UI 差し替えを想定し、変換は `platform/qt` に限定する
