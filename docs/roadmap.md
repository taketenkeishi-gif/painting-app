# Roadmap

詳細ビジョン・技術選定・フェーズ詳細は `docs/SPEC.md` を参照。

## Phase 0 — 描画エンジン根本再構築（現在地・最優先）
- [ ] Renderer のベクター描画を float 精度 + AA に置き換え
- [ ] BrushTool の antiAlias フラグを実装・UI 接続
- [ ] ペンタブ筆圧の正常取得確認
- [ ] （中期）Skia バックエンド移行

## Phase 1 — 描画アプリ標準機能の完全実装
- フォルダ階層・クリッピンググループ・複数レイヤー選択
- 投げ縄/自動選択・変形ハンドル・自由変形
- 調整レイヤー（トーンカーブ・色相彩度）
- Wintab / Windows Ink ネイティブ筆圧
- グラデーション・テキストツール

## Phase 2 — 静止画MV制作ソフトとの連携
- 独自レイヤーフォーマット策定
- 共有メモリ / Named Pipe でのリアルタイム連携
- 「埋め込みエディタ」起動プロトコル

## Phase 3 — ローカルAI統合（Krita AI Diffusion 超過版）
- stable-diffusion.cpp / ComfyUI ローカル統合
- img2img / インペイント / スタイル転送
- MVタイムライン連携アニメーション補間AI生成
- **外部APIゼロ**が絶対条件
