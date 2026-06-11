# ADR（Architecture Decision Record）

重要な技術判断・アーキテクチャ決定を記録するディレクトリ。

## 概要

Painting-app の重要な技術判断（フレームワーク選定、アーキテクチャ設計、ライブラリ採用など）を時系列で記録。

**目的:**
- 決定理由を明示（なぜその判断をしたのか）
- 代替案とのトレードオフを記録
- チーム内で判断の履歴を共有
- 将来の参考資料として活用

## ADR リスト

| ADR | タイトル | Status | 日付 |
|---|---|---|---|
| [ADR-001](ADR-001-skia-backend.md) | Google Skia を 2D レンダリングバックエンド として採用 | ✅ IMPLEMENTED | 2026-05-xx |
| ADR-002 | ブラシエンジンは MyPaint / Skia ハイブリッド | ✅ IMPLEMENTED | 2026-05-xx |
| ADR-003 | ベクターレイヤーは「記録 + ラスタライズ合成」戦略 | ✅ IMPLEMENTED | 2026-06-xx |
| ADR-004 | ToolDescriptor は「Code-driven」 | ✅ IMPLEMENTED | 2026-05-xx |
| ADR-005 | LayerPanel の「行インデックス明示」設計 | ✅ IMPLEMENTED | 2026-05-xx |
| ADR-006 | SAM2 ONNX を AI 選択ツール として採用 | ✅ IMPLEMENTED | 2026-06-xx |
| ADR-007 | Dark Theme を「Color Token」ベース で実装 | ✅ IMPLEMENTED | 2026-06-xx |
| [ADR-008](ADR-008-layer-offset-model.md) | レイヤーオフセットモデル（Canvas-Bound から Independent PixelBuffer へ） | 🟡 ACCEPTED | 2026-06-10 |

## ステータス

- **🟢 PROPOSED** — 提案段階（議論中）
- **🟡 ACCEPTED** — 受理（実装予定）
- **🟢 IMPLEMENTED** — 実装完了
- **⚫ SUPERSEDED** — 別の決定で置き換わった
- **🔴 REJECTED** — 不採用（検討したが採用せず）

## 記録方法

### 新規 ADR 作成

1. `ADR-NNN-title.md` ファイルを作成
2. [ADR-000-template.md](ADR-000-template.md) をコピー＆修正
3. 上記 ADR リストに追加
4. git commit: `docs: add ADR-NNN-title`

### テンプレート

- [ADR-000-template.md](ADR-000-template.md) — 記述フォーマット

### 記述ガイドライン

- **シンプルに:** 1 判断 = 1 ADR
- **トレードオフを明示:** 代替案との比較が重要
- **理由を詳しく:** 「なぜこれを選んだか」を明確に
- **実装と紐付け:** コード例・ファイル構成を含める

## 参照

- [DECISIONS.md](../.claude/DECISIONS.md) — 全判断の要約
- [SPEC.md](../SPEC.md) — ビジョン・フェーズ
- [SPEC2.md](../SPEC2.md) — 実装仕様
- [PROJECT_STATUS.md](../.claude/PROJECT_STATUS.md) — 進捗

