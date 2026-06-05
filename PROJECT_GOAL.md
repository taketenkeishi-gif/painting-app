# PROJECT GOAL — Painting-app ai-night-test

## 最優先ソース読み込み順
1. docs/SPEC.md（最優先）
2. CLAUDE.md / PROJECT_RULES.md
3. README.md
4. docs/*.md
5. src/ ソースコード

---

## 耐久試験フェーズの目的

「壊さず働く」を証明する。

新機能追加・大型設計変更は禁止。
SPECとの差分把握 → 小さいバグ修正・既存機能の完成のみ。

---

## 初回 Manager タスク（必須・最優先）

**PROJECT_ANALYSIS.md を作成する（実装禁止）**

以下の情報をソースコードから読み取って記録する:

### 記載内容
1. **アーキテクチャ概要** — レイヤー構成、主要クラスの役割
2. **描画パイプライン** — ストローク入力 → レンダリング → 表示の流れ
3. **UI構成** — MainWindow, Dock, Panel の構成
4. **レイヤー構造** — LayerManager, Layer種別
5. **未完成部分** — system_status.md の TODO/PARTIAL を整理
6. **SPECとの差分** — docs/SPEC.md と実装の差分（Phase 0 / Phase 1）

作成後: `git add PROJECT_ANALYSIS.md && git commit -m "analysis: PROJECT_ANALYSIS.md created"`

---

## タスク生成ルール（耐久試験）

### 初回は最大10件まで生成可
### 優先順位

**High**: system_status.md の WIP / PARTIAL → 完成させる  
**Medium**: system_status.md の TODO のうち小さいもの（50行以内で実装可能）  
**Low**: ドキュメント整備

### 絶対禁止
- Skia移行（大型、別フェーズ）
- libmypaint統合（大型、別フェーズ）
- OSS大量移植
- 全面リファクタリング
- ファイルフォーマット対応（PSD/ORA）—大型
- AIセレクト機能拡張

### REVIEW_REQUIRED条件
- UIの変更（見た目・文言）
- 新規外部依存追加
- CMakeLists.txt の変更
- vcpkg.json の変更
- 既存の動作を変える可能性があるロジック変更
- 50行以上の削除

---

## ビルド確認コマンド

```powershell
cmake --build build --config Release
```

成功 = エラー0件（警告は許容）

## テスト確認

```powershell
cmake --build build --target run_tests --config Release
```

または `.\launch.bat` で起動確認

---

## Developer 完了条件（build成功だけでは不可）

必須確認:
1. `cmake --build` でエラー0
2. 変更した機能の実行経路をコードで確認（コメントで記録）
3. UIを変えた場合は `.\launch.bat` 起動 → 目視確認が必要 → TASK_REVIEW
4. 変更理由をコミットメッセージに記録
