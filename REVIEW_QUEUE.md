# REVIEW QUEUE — Painting-app
<!-- scheduler が自動追記します。確認後は status を reviewed に変更してください -->


---

## task-001
- **追加日時**: 2026-06-05 10:17:38
- **status**: pending_review
- **担当worker**: worker-b
- **理由**: UIの変更を含むため目視確認が必要。ブラシツール選択時に形状セクション（角度/真円率/入り抜き始点・終点スライダー）が「詳細を表示」クリックなしで表示されること、他ツール選択時に非表示になることを確認してください。
- **確認してほしい操作**: ```powershell
cmake --build build --config Release
.\launch.bat   # 起動後、ToolPropertyPanel でスライダーが表示・操作できることを目視確認
```

### review_required_when
- UI変更が含まれるため必須

