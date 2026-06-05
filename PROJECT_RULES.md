# PROJECT RULES — Painting-app

## スタック
- C++17 / Qt 6.7.2 / CMake 4.x / MSVC 2022
- Qt パス: `C:\Qt\6.7.2\msvc2019_64`
- ビルド: `cmake --build build --config Release`

## ⛔ 絶対禁止
- Skia 移行（別フェーズ、大型作業）
- libmypaint 統合（別フェーズ）
- CMakeLists.txt の変更（REVIEW_REQUIRED）
- vcpkg.json の変更（REVIEW_REQUIRED）
- 外部ライブラリの追加（REVIEW_REQUIRED）
- main ブランチへのコミット
- 50行以上の削除（REVIEW_REQUIRED）
- 動かない仮実装・TODOだけ追加して完了報告
- ビルドエラーが出る状態でのコミット

## ✅ 必須手順
1. 変更前に関連ファイルを読む（SPEC.md → CLAUDE.md → src/）
2. `cmake --build build --config Release` でエラー0を確認
3. 変更理由をコミットメッセージに記録
4. UIを変えた場合は TASK_REVIEW（目視確認は人間が行う）

## REVIEW_REQUIRED 条件
- 見た目・文言の変更
- ビルド設定の変更
- 既存動作を変える可能性のあるロジック
- 50行以上の削除・大規模変更
- 新しい依存の追加

## コミットメッセージ形式
```
{task-id}: {50文字以内の要約}

変更理由: {なぜこの変更が必要か}
影響範囲: {どのファイル・機能に影響するか}
```
