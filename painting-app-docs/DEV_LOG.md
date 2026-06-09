# 開発ログ — Paint App

セッションごとの作業内容・完了タスク・次のアクションを記録。

---

## 2026-06-08 (最新)

### セッション概要

**型式:** ドキュメント管理の標準化  
**対象:** docs/ と .claude/ の構造整理

### 作業内容

- [ ] docs/SPEC.md を確認・SPEC2.md との役割分け整理
- [ ] architecture.md の内容を SPEC2.md に統合
- [x] PROJECT_STATUS.md 新規作成（実装進捗管理）
- [x] DEV_LOG.md 新規作成（本ファイル）
- [x] DECISIONS.md 新規作成（技術判断記録）
- [x] FAILED_ATTEMPTS.md 作成スタブ
- [ ] docs/adr/ フォルダ作成 + ADR テンプレート

### 完了項目

- ✅ PROJECT_STATUS.md を MV-studio-app 形式で作成
- ✅ 実装進捗を Phase 0 / Phase 1 に分類
- ✅ タスク優先度リストを整理

### 次のアクション

- [ ] docs/adr/ + ADR-001 を作成（技術判断の履歴化）
- [ ] 古い管理ファイル（SESSION_LOG.md など）をアーカイブ
- [ ] ドキュメント統合の完了確認

---

## 2026-05-XX (過去セッション例)

### セッション概要

**型式:** Feature / Phase 0-2 ブラシ品質向上  
**対象:** src/core/brush/ / src/core/tools/

### 作業内容

- Velocity dynamics 実装（速度感応で size/opacity 変更）
- Texture grain 追加（セルノイズで決定論的グレイン）
- Wet-mix / smear 実装
- AA 描画（gaussianFalloff の代替）

### 完了項目

- ✅ BrushTool に velocity パラメータ追加
- ✅ テクスチャグレイン実装（core/math/cellnoise.cpp）
- ✅ Smear アルゴリズム
- ✅ stampCircleAA / drawSegmentAA 実装
- ✅ テスト通過

### 次のアクション

- [ ] UI exposure（ToolPropertyPanel に velocity toggle 追加）
- [ ] Skia 統合（Phase 0-1）

---

## 記録フォーマット

### セッション開始時

```markdown
## YYYY-MM-DD HH:MM 〜 HH:MM (所要時間)

### セッション概要

**型式:** [Feature / Fix / Debug / Refactor / Doc]  
**対象:** [モジュール / ファイル]  
**コミット:** [短縮ハッシュ]

### 作業内容

- タスク 1
- タスク 2

### 完了項目

- ✅ 実装: ...
- ✅ テスト: ...
- ✅ ドキュメント: ...

### 次のアクション

- [ ] TODO 1
- [ ] TODO 2

### 参考

- [参照ドキュメント](docs/SPEC.md)
- 関連コミット: `abc1234`
```

---

## インデックス

| 日付 | セッション | 状態 | コミット |
|---|---|---|---|
| 2026-06-08 | ドキュメント管理標準化 | 進行中 | — |
| 2026-05-xx | Phase 0-2 ブラシ品質 | DONE | `98f89c0` |
| 2026-05-xx | Skia 統合基盤 | DONE | `ff2b231` |

