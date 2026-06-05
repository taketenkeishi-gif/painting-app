# TASK QUEUE ?EPainting-app ai-night-test
<!-- scheduler ��Estatus ��X�V���܂�E-->

## task-006
status: completed   
priority: high
role: developer

### ��?EToolPropertyPanel.cpp �Ɂu�u���V�T�C�Y�v�X���C�_�[�� ToolTip ��ǉ�����AE���� BrushSizeSlider �ɂ� setToolTip ���ݒ肳��Ă�E??��E??E�ȉ�?E��Ƃ�i�K�I�Ɏ��{���邱��:

1. ToolPropertyPanel.cpp / ToolPropertyPanel.h ��ERead �`E?E���œǂ�Ō���c��
2. BrushSizeSlider �܂�?E�Ή�����EQSlider ���?E3. setToolTip("�u���V�T�C�Y (1-500)") ��ǉ�
4. AppController.h ��EBrushSettings.h �ŃT�C�Y�E??��m�F���� ToolTip ?E???E�𐳊m�ɋL�q
5. cmake --build build --config Release �Ńr���h�m?E6. HANDOFF_STATE.json ��X�V?E?Ebjective / completed / current_files / next_steps ��?E??���߂�IE
### �������
- ToolPropertyPanel �̃u���V�T�C�Y�X���C�_�[�� ToolTip ���\�������E- cmake --build �ŃG���[0��

### �֎~����E- ToolPropertyPanel �ȊO?E�t�@�C���̕ύX
- �V�K�N���X�E�V�K�t�@�C���̍�?E
- setToolTip �ȊO?E UI �ύX

### ���ؕ���E`powershell
cmake --build build --config Release
`

---

## task-001
status: completed
priority: high
role: developer

### ��?EToolPropertyPanel �� angle / roundness / taperStart / taperEnd �X���C�_�[��ǉ����AEAppController �̊�?Esetter (setBrushAngle ?E �ɐڑ�����AEBrushSettings �ɒl�͕ێ�����Ă�E??���p�l�� UI �����ڑ�?E���߁APARTIAL ��Ԃ�������AE
### �������
- ToolPropertyPanel �� angle/roundness/taperStart/taperEnd �̙�E??���C�_�[���\�������E- �X���C�_�[���쎞�� AppController::setBrushAngle �����Ă΂�EToolStateViewModel �ɔ��f�����E- �u���V�`E?E���I����̂ݕ\���A���c�[���ł͔�\���܂�?E����

### �֎~����E- BrushSettings / AppController / core:: ���̕ύX?E?Eetter �͊��Ɏ���E??��?E?E- �V�K�O���ˑ�?E�ǉ�

### ���ؕ���E```powershell
cmake --build build --config Release
.\launch.bat   # �N����AToolPropertyPanel �ŃX���C�_�[���\���E����ł��邱�Ƃ�ڎ��m?E```

### review_required_when
- UI�ύX���܂܂�邽��?E??E
---

## task-002
status: blocked
priority: high
role: developer

### ��?EFillTool ��ESelectionMask ���A�N�`E??�u�ȂƂ��AE??���E???E?E�s�N�Z���̂ݓh��Ԃ��悟E??�C������AE����� SelectionMask �𖳎����đS��h��Ԃ����������Ă�E???E?Eystem_status: PARTIAL?E?�AE
### �������
- �I���E??�����݂���Ƃ��A�h��Ԃ����I���E???E?E�݂ɓK�p�����E- �I���E??���Ȃ�E??��?E�]���ǂ���S��܂�?E contiguous fill �����삷��E- undo/redo ������ɋ@?E����

### �֎~����E- SelectionMask ��EFillTool �̐݌v�ύX?E?�����C���^�[�t�F�[�X���E???E??�C��?E?E- fillGapClose / referAllLayers ��?E���@?E�ւ̉e��

### ���ؕ���E```powershell
cmake --build build --config Release
# �N����E ��`�I?E?EFill ?E�I���E??�O���h��Ԃ���Ȃ�E??�Ƃ�m?E```

### review_required_when
- ����?E�����ς���\�������郍�W�`E??�ύX�̂���?E??E
---

## task-003
status: blocked
priority: medium
role: developer

### ��?EBrushSettings.antiAlias �t���O����ۂɋ@?E������AE���� stampCircleAA / drawSegmentAA �͌Ă΂�Ă�E??���AantiAlias=false �̂Ƃ�E�n?E�h�G�`E??�`��?E?EtampCircle / drawSegment �����Łj�ւ�?E??�ւ���������E??ESPEC Phase 0-4 �́uAA ON/OFF ����v���?E������AE
### �������
- antiAlias=true: ����Ɠ��� AA �`��?E?�ω��Ȃ��IE- antiAlias=false: �n?E�h�G�`E???E?Eaussian �Ȃ��j�`���?E??�ւ��
- ToolPropertyPanel �� antiAlias �`�F�`E??�{�b�N�X�����ۂɌ��ʂ���

### �֎~����E- Skia �ڍs?Elibmypaint �����i�ʃt�F�[�Y?E?E- stampCircleAA �̍폜��E??�K�̓��t�@�N�^

### ���ؕ���E```powershell
cmake --build build --config Release
# �N����E antiAlias OFF �Ńu���V�X�g���[�N���n�[�h�G�`E??�ɂȂ邱�Ƃ�m?E```

### review_required_when
- ����?E�`�擮���ς��郍�W�`E??�ύX�̂���?E??E
---

## task-004
status: blocked
priority: medium
role: developer

### ��?ENavigator �p�l�� (m_infoDock) �ɁAE00%�v�uFit�v�N�C�`E??�Y�[���{�^����ǉ����AECanvasWidget::resetZoom / fitToScreen �ɐڑ�����AEsystem_status: PARTIAL�unavigator mini-canvas preview + quick zoom buttons�v���?E������AE
### �������
- Navigator �p�l���ɁAE00%�v�uFit�v?E�^�����\�������E- �N���`E??�� CanvasWidget �̃Y�[����?E��ւ��
- ����?E�i�r�Q�[�^�[�v���r���[�\�������Ȃ�E
### �֎~����E- CanvasWidget / AppController �̕ύX?E?EesetZoom / fitToScreen �͊��Ɏ���E??��?E?E- Navigator �ȊO?E�p�l���ւ̕ύX

### ���ؕ���E```powershell
cmake --build build --config Release
.\launch.bat   # Navigator �� 100%/Fit �{�^�����\���E���삷�邱�Ƃ�ڎ��m?E```

### review_required_when
- UI�ύX���܂܂�邽��?E??E
---

## task-005
status: blocked
priority: medium
role: developer

### ��?EColorWheelWidget �܂�?E color dock �`E?E���o?E�� FG/BG swap �{�^����
B/W reset �{�^����ǉ����AMainWindow::onSwapColors / onResetBlackWhiteColors �ɐڑ�����AEsystem_status: PARTIAL�ucolor system commands�v�� Panel �{�^���Ƃ��Ă����ł���悤�ɂ���AE
### �������
- color dock ?E?? swap (?E �{�^���� B/W ���Z�`E??�{�^�����\�������E- �N���`E??�Ŋ�?EAction �Ɠ�����������?E?�F��?E��ւ��?E?E- ����?E�V���[�g�J�`E??/���j���[����Ƌ������Ȃ�E
### �֎~����E- AppController ��Ecore:: ���̕ύX?E?����?E��?EAction �ɈϏ����邾���IE- ColorWheelWidget �� HSV �`�惍�W�`E??�ւ̕ύX

### ���ؕ���E```powershell
cmake --build build --config Release
.\launch.bat   # color dock �� swap/reset �{�^�����\���E���삷�邱�Ƃ�ڎ��m?E```

### review_required_when
- UI�ύX���܂܂�邽��?E??E

## task-7
status: pending
attempt: 0
priority: medium
role: developer
category: feature

### 目的
MainWindow のステータスバーに現在のズーム率（例: 100%）を表示する。
CanvasWidget が持つズーム変更シグナルを MainWindow で受け取り、QLabel に反映する。

### 対象ファイル候補
- src/app/mainwindow/MainWindow.cpp
- src/app/mainwindow/MainWindow.h
- src/app/canvasview/CanvasWidget.h（シグナル確認のみ・変更可）

### 成功条件
- cmake --build でエラー 0 件
- アプリ起動後、ステータスバーに「100%」等のズーム率が表示される
- ズームイン／アウト操作で数値がリアルタイム更新される
- REVIEW_REQUIRED（UI変更のため）

### 時間見積もり
30〜45 分

### 禁止事項
- CanvasWidget のズーム計算ロジック変更
- 新規クラス・新規ファイルの作成

### 検証方法
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-8
status: pending
attempt: 0
priority: high
role: developer
category: bug

### 目的
CanvasWidget のマウスイベントハンドラ（mousePressEvent / mouseMoveEvent / mouseReleaseEvent）でカレントツールポインタが nullptr の場合にクラッシュする問題を防ぐ。
ツール切替中の急速な入力やシャットダウン時の誤操作でツールポインタが未設定のままイベントが来るケースに対処する。

### 対象ファイル候補
- src/app/canvasview/CanvasWidget.cpp

### 成功条件
- cmake --build でエラー 0 件
- 各マウスイベント冒頭に nullptr ガードが追加されている（コードレビューで確認）
- ツール切替直後の連続クリックでクラッシュしない（実行経路をコメントで記録）

### 時間見積もり
30 分

### 禁止事項
- CanvasWidget.h のシグナル・スロット定義変更
- ツール描画ロジックの変更

### 検証方法
```powershell
cmake --build build --config Release
```

---

## task-9
status: pending
attempt: 0
priority: medium
role: developer
category: feature

### 目的
LineTool 使用時に ToolPropertyPanel へ「線の太さ」スライダーを表示する。
ToolDescriptor に LineTool 用プロパティ定義を追加し、ToolPropertyPanel 側で対応するウィジェットを生成・接続する。

### 対象ファイル候補
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h
- src/app/panels/ToolPropertyPanel.cpp
- src/app/panels/ToolPropertyPanel.h

### 成功条件
- cmake --build でエラー 0 件
- LineTool 選択時に ToolPropertyPanel に「線の太さ」スライダーが表示される
- スライダー操作で描画線の太さが変化する
- REVIEW_REQUIRED（UI変更のため）

### 時間見積もり
45〜60 分

### 禁止事項
- LineTool.cpp のアルゴリズム変更
- 新規クラス・新規ファイルの作成

### 検証方法
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-10
status: pending
attempt: 0
priority: low
role: developer
category: refactor

### 目的
ToolDescriptor.cpp に散在するスライダー初期値・最小値・最大値のマジックナンバーを constexpr 名前付き定数に置き換え、将来の値変更を一箇所で管理できるようにする。

### 対象ファイル候補
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h

### 成功条件
- cmake --build でエラー 0 件
- ToolDescriptor.cpp 内のスライダー設定数値が constexpr 定数に置き換わっている（コードレビューで確認）
- 動作変化なし（リファクタのみ）

### 時間見積もり
30〜45 分

### 禁止事項
- スライダーの値域・デフォルト値の変更
- ToolPropertyPanel.cpp の変更

### 検証方法
```powershell
cmake --build build --config Release
```

---

## task-11
status: pending
attempt: 0
priority: high
role: developer
category: bug

### 目的
AppController の保存・エクスポート・Undo・Redo 処理でドキュメントポインタが nullptr の場合に未定義動作が生じるリスクを排除する。
各操作の冒頭に nullptr ガードを追加し、ドキュメント未作成状態での操作を安全に無視する。

### 対象ファイル候補
- src/app/bridge/AppController.cpp

### 成功条件
- cmake --build でエラー 0 件
- 保存・Undo・Redo 各ハンドラの冒頭に nullptr チェックが追加されている（コードレビューで確認）
- 新規起動直後に Ctrl+Z 連打しても クラッシュしない

### 時間見積もり
30 分

### 禁止事項
- AppController.h のシグナル定義変更
- Document.cpp の変更

### 検証方法
```powershell
cmake --build build --config Release
```

---

## task-12
status: pending
attempt: 0
priority: medium
role: developer
category: feature

### 目的
GradientTool 選択時に ToolPropertyPanel へ「グラデーション種類」セレクタ（線形 / 放射状）を追加する。
ToolDescriptor に GradientTool 用 enum プロパティを定義し、ToolPropertyPanel で QComboBox として表示・接続する。

### 対象ファイル候補
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h
- src/app/panels/ToolPropertyPanel.cpp
- src/app/panels/ToolPropertyPanel.h

### 成功条件
- cmake --build でエラー 0 件
- GradientTool 選択時に ToolPropertyPanel に「線形 / 放射状」の切替 ComboBox が表示される
- 切替操作で描画グラデーションの種類が変わる
- REVIEW_REQUIRED（UI変更のため）

### 時間見積もり
45〜60 分

### 禁止事項
- GradientTool.cpp の描画アルゴリズム新規実装
- 新規クラス・新規ファイルの作成

### 検証方法
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-13
status: pending
attempt: 0
priority: low
role: developer
category: refactor

### 目的
LayerPanel.cpp に散在するレイヤーサムネイルサイズ・行高さ等のハードコード数値を名前付き定数（static constexpr int）に置き換え、一箇所で管理できるようにする。

### 対象ファイル候補
- src/app/panels/LayerPanel.cpp
- src/app/panels/LayerPanel.h

### 成功条件
- cmake --build でエラー 0 件
- LayerPanel.cpp 内のサムネイル・行高サイズ数値が constexpr 定数に置き換わっている（コードレビューで確認）
- レイヤーパネルの見た目・動作に変化なし

### 時間見積もり
30 分

### 禁止事項
- レイヤーパネルの実際のサイズ・見た目の変更
- LayerPanel のシグナル定義変更

### 検証方法
```powershell
cmake --build build --config Release
```

---

## task-14
status: pending
attempt: 0
priority: medium
role: developer
category: feature

### 目的
ドキュメントに未保存の変更がある場合、MainWindow のタイトルバーにアスタリスク（例: 「無題 *」）を付加してダーティ状態を示す。
AppController の変更通知シグナルを MainWindow で受け取り setWindowModified() で反映する。

### 対象ファイル候補
- src/app/mainwindow/MainWindow.cpp
- src/app/mainwindow/MainWindow.h
- src/app/bridge/AppController.h（シグナル確認のみ・変更可）

### 成功条件
- cmake --build でエラー 0 件
- 描画操作後にタイトルバーに「*」が付く
- 保存後に「*」が消える
- REVIEW_REQUIRED（UI変更のため）

### 時間見積もり
30〜45 分

### 禁止事項
- AppController のシグナル新規追加（既存シグナルを使うこと）
- Document.cpp の変更

### 検証方法
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-15
status: pending
attempt: 0
priority: medium
role: developer
category: bug

### 目的
SelectionOverlayRenderer.cpp の描画関数で QPainter::begin() 後に早期 return するパスが存在する場合、end() を呼ばずに関数を抜けると Qt の警告・描画アーティファクトが発生する。
early return 前に必ず QPainter::end() を呼ぶか、RAII ラッパー（スタック上の QPainter）に切り替えて安全化する。

### 対象ファイル候補
- src/app/canvasview/SelectionOverlayRenderer.cpp

### 成功条件
- cmake --build でエラー 0 件
- SelectionOverlayRenderer の全描画関数で QPainter が必ず終了される（コードレビューで確認）
- 選択領域オーバーレイ描画時にQtの「painter not ended」警告が出ない

### 時間見積もり
30 分

### 禁止事項
- 選択領域の描画スタイル（色・点線パターン等）の変更
- SelectionOverlayRenderer.h のインターフェイス変更

### 検証方法
```powershell
cmake --build build --config Release
```

---

## task-16
status: pending
attempt: 0
priority: medium
role: developer
category: feature

### 目的
TextTool 選択時に ToolPropertyPanel へフォントサイズ入力用 QSpinBox を追加する。
ToolDescriptor に TextTool 用フォントサイズプロパティを定義し、ToolPropertyPanel 側でスピンボックスとして表示・TextTool に値を渡す接続を行う。

### 対象ファイル候補
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h
- src/app/panels/ToolPropertyPanel.cpp
- src/app/panels/ToolPropertyPanel.h
- src/core/tools/TextTool.cpp
- src/core/tools/TextTool.h

### 成功条件
- cmake --build でエラー 0 件
- TextTool 選択時に ToolPropertyPanel にフォントサイズ SpinBox（例: 8〜144pt）が表示される
- SpinBox の値変更が TextTool に反映される（描画文字サイズが変わる）
- REVIEW_REQUIRED（UI変更のため）

### 時間見積もり
60〜90 分

### 禁止事項
- TextTool のテキスト入力ダイアログ全面刷新
- フォントファミリー選択機能の同時追加（本タスクはサイズのみ）
- 新規クラス・新規ファイルの作成

### 検証方法
```powershell
cmake --build build --config Release
.\launch.bat
```

---
