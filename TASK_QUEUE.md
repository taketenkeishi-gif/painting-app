# TASK QUEUE ?EPainting-app ai-night-test
<!-- scheduler ??Estatus ??X?V?????E-->

## task-006
status: completed   
priority: high
role: developer

### ???EToolPropertyPanel.cpp ??u?u???V?T?C?Y?v?X???C?_?[?? ToolTip ????????AE???? BrushSizeSlider ??? setToolTip ?????????E????E??E????E????i?K?I????{???�E�??:

1. ToolPropertyPanel.cpp / ToolPropertyPanel.h ??ERead ?`E?E??????????c??
2. BrushSizeSlider ????E???????EQSlider ????E3. setToolTip("?u???V?T?C?Y (1-500)") ????
4. AppController.h ??EBrushSettings.h ??T?C?Y?E????m?F???? ToolTip ?E???E???m??L?q
5. cmake --build build --config Release ??r???h?m?E6. HANDOFF_STATE.json ??X?V?E?Ebjective / completed / current_files / next_steps ???E???????IE
### ???????
- ToolPropertyPanel ??u???V?T?C?Y?X???C?_?[?? ToolTip ???\???????E- cmake --build ??G???[0??

### ??~????E- ToolPropertyPanel ??O?E?t?@?C?????X
- ?V?K?N???X?E?V?K?t?@?C??????E
- setToolTip ??O?E UI ??X

### ???????E`powershell
cmake --build build --config Release
`

---

## task-001
status: completed
priority: high
role: developer

### ???EToolPropertyPanel ?? angle / roundness / taperStart / taperEnd ?X???C?_?[???????AEAppController ????Esetter (setBrushAngle ?E ????????AEBrushSettings ??l??????????E?????p?l?? UI ????????E????APARTIAL ??????????AE
### ???????
- ToolPropertyPanel ?? angle/roundness/taperStart/taperEnd ???E?????C?_?[???\???????E- ?X???C?_?[?????? AppController::setBrushAngle ????????EToolStateViewModel ????f?????E- ?u???V?`E?E???I??????\???A???c?[??????\??????E????

### ??~????E- BrushSettings / AppController / core:: ?????X?E?Eetter ????????E?????E?E- ?V?K?O??????E???

### ???????E```powershell
cmake --build build --config Release
.\launch.bat   # ?N????AToolPropertyPanel ??X???C?_?[???\???E???????�E�??????m?E```

### review_required_when
- UI??X??????????E??E
---

## task-002
status: blocked
priority: high
role: developer

### ???EFillTool ??ESelectionMask ???A?N?`E???u?????AE?????E???E?E?s?N?Z?????h??????�E��E�E???C??????AE????? SelectionMask ????????S??h????????????????E???E?Eystem_status: PARTIAL?E??AE
### ???????
- ?I???E??????????????A?h????????I???E???E?E???K?p?????E- ?I???E???????E?????E?]???????S?????E contiguous fill ????????E- undo/redo ???????@?E????

### ??~????E- SelectionMask ??EFillTool ???v??X?E??????C???^?[?t?F?[?X???E???E???C???E?E- fillGapClose / referAllLayers ???E???@?E???e??

### ???????E```powershell
cmake --build build --config Release
# ?N????E ??`?I?E?EFill ?E?I???E???O???h?????????E?????m?E```

### review_required_when
- ?????E??????????\?????????W?`E????X??????E??E
---

## task-003
status: blocked
priority: medium
role: developer

### ???EBrushSettings.antiAlias ?t???O??????@?E??????AE???? stampCircleAA / drawSegmentAA ???????E?????AantiAlias=false ????E?n?E?h?G?`E???`???E?EtampCircle / drawSegment ??????j????E?????????????E??ESPEC Phase 0-4 ??uAA ON/OFF ????v????E??????AE
### ???????
- antiAlias=true: ???????? AA ?`???E??�E��E�?????IE- antiAlias=false: ?n?E?h?G?`E???E?Eaussian ????j?`????E??????
- ToolPropertyPanel ?? antiAlias ?`?F?`E???{?b?N?X?????????????

### ??~????E- Skia ??s?Elibmypaint ?????i??t?F?[?Y?E?E- stampCircleAA ?????E???K????t?@?N?^

### ???????E```powershell
cmake --build build --config Release
# ?N????E antiAlias OFF ??u???V?X?g???[?N???n?[?h?G?`E?????�E�???m?E```

### review_required_when
- ?????E?`??????????W?`E????X??????E??E
---

## task-004
status: blocked
priority: medium
role: developer

### ???ENavigator ?p?l?? (m_infoDock) ??AE00%?v?uFit?v?N?C?`E???Y?[???{?^?????????AECanvasWidget::resetZoom / fitToScreen ????????AEsystem_status: PARTIAL?unavigator mini-canvas preview + quick zoom buttons?v????E??????AE
### ???????
- Navigator ?p?l????AE00%?v?uFit?v?E?^?????\???????E- ?N???`E???? CanvasWidget ??Y?[?????E?????
- ?????E?i?r?Q?[?^?[?v???r???[?\?????????E
### ??~????E- CanvasWidget / AppController ???X?E?EesetZoom / fitToScreen ????????E?????E?E- Navigator ??O?E?p?l??????X

### ???????E```powershell
cmake --build build --config Release
.\launch.bat   # Navigator ?? 100%/Fit ?{?^?????\???E?????�E�??????m?E```

### review_required_when
- UI??X??????????E??E
---

## task-005
status: blocked
priority: medium
role: developer

### ???EColorWheelWidget ????E color dock ?`E?E???o?E?? FG/BG swap ?{?^????
B/W reset ?{?^?????????AMainWindow::onSwapColors / onResetBlackWhiteColors ????????AEsystem_status: PARTIAL?ucolor system commands?v?? Panel ?{?^?????????????????????AE
### ???????
- color dock ?E?? swap (?E ?{?^???? B/W ???Z?`E???{?^?????\???????E- ?N???`E??????EAction ??????????????E??F???E??????E?E- ?????E?V???[?g?J?`E??/???j???[?????????????E
### ??~????E- AppController ??Ecore:: ?????X?E??????E???EAction ???????????IE- ColorWheelWidget ?? HSV ?`???W?`E??????X

### ???????E```powershell
cmake --build build --config Release
.\launch.bat   # color dock ?? swap/reset ?{?^?????\???E?????�E�??????m?E```

### review_required_when
- UI??X??????????E??E

## task-7
status: review_required
attempt: 1
completed_by: worker-b
summary: MainWindow コンストラクタに viewTransformChanged ↁEm_zoomStatusLabel 更新の signal-slot 接続を追加。既存�E updateZoomStatusLabel (CanvasWidget 側 findChild) はそ�Eまま残し、MainWindow 側の正式接続を追加した。ビルドエラー0件確認済み、Epriority: medium
role: developer
category: feature

### �E�ړI
MainWindow �E�̃X�E�e�E�[�E�^�E�X�E�o�E�[�E�Ɍ��E�݂̃Y�E�[�E��E��E��E��E�i�E��E�: 100%�E�j�E��E�\�E��E��E��E��E��E�B
CanvasWidget �E��E��E��E��E�Y�E�[�E��E��E�ύX�E�V�E�O�E�i�E��E��E��E� MainWindow �E�Ŏ󂯎��E�AQLabel �E�ɔ��E�f�E��E��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/mainwindow/MainWindow.cpp
- src/app/mainwindow/MainWindow.h
- src/app/canvasview/CanvasWidget.h�E�i�E�V�E�O�E�i�E��E��E�m�E�F�E�̂݁E�E�ύX�E�j

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- �E�A�E�v�E��E��E�N�E��E��E��E�A�E�X�E�e�E�[�E�^�E�X�E�o�E�[�E�Ɂu100%�E�v�E��E��E�̃Y�E�[�E��E��E��E��E��E��E�\�E��E��E��E��E��E��E�
- �E�Y�E�[�E��E��E�C�E��E��E�^�E�A�E�E�E�g�E��E��E��E�Ő��E�l�E��E��E��E��E�A�E��E��E�^�E�C�E��E��E�X�E�V�E��E��E��E��E�
- REVIEW_REQUIRED�E�iUI�E�ύX�E�̂��E�߁j

### �E��E��E�Ԍ��E�ς��E��E�
30?45 �E��E�

### �E�֎~�E��E��E��E�
- CanvasWidget �E�̃Y�E�[�E��E��E�v�E�Z�E��E��E�W�E�b�E�N�E�ύX
- �E�V�E�K�E�N�E��E��E�X�E�E�E�V�E�K�E�t�E�@�E�C�E��E��E�̍쐬

### �E��E��E�ؕ��E�@
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-8
status: review_required
attempt: 1
priority: high
role: developer
category: bug

### 完了概要
mousePressEvent / mouseMoveEvent / mouseReleaseEvent の nullptr ガードに
`m_mouseDrawing = false` と `hasLastStrokeDispatchPos = false` のリセットを追加。
コントローラ切断時にストローク状態が残るバグを修正。ビルド確認: エラー0件。

### �E�ړI
CanvasWidget �E�̃}�E�E�E�X�E�C�E�x�E��E��E�g�E�n�E��E��E�h�E��E��E�imousePressEvent / mouseMoveEvent / mouseReleaseEvent�E�j�E�ŃJ�E��E��E��E��E�g�E�c�E�[�E��E��E�|�E�C�E��E��E�^�E��E� nullptr �E�̏ꍇ�E�ɃN�E��E��E�b�E�V�E��E��E��E��E��E��E��E��E�h�E��E��E�B
�E�c�E�[�E��E��E�ؑ֒��E�̋}�E��E��E�ȓ��E�͂�V�E��E��E�b�E�g�E�_�E�E�E��E��E��E��E�̌둀�E��E�Ńc�E�[�E��E��E�|�E�C�E��E��E�^�E��E��E��E��E�ݒ�̂܂܃C�E�x�E��E��E�g�E��E��E��E��E��E�P�E�[�E�X�E�ɑΏ��E��E��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/canvasview/CanvasWidget.cpp

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- �E�e�E�}�E�E�E�X�E�C�E�x�E��E��E�g�E�`�E��E��E��E� nullptr �E�K�E�[�E�h�E��E��E�ǉ��E��E��E��E�Ă��E��E�i�E�R�E�[�E�h�E��E��E�r�E��E��E�[�E�Ŋm�E�F�E�j
- �E�c�E�[�E��E��E�ؑ֒��E��E�̘A�E��E��E�N�E��E��E�b�E�N�E�ŃN�E��E��E�b�E�V�E��E��E��E��E�Ȃ��E�i�E��E��E�s�E�o�E�H�E��E��E�R�E��E��E��E��E�g�E�ŋL�E�^�E�j

### �E��E��E�Ԍ��E�ς��E��E�
30 �E��E�

### �E�֎~�E��E��E��E�
- CanvasWidget.h �E�̃V�E�O�E�i�E��E��E�E�E�X�E��E��E�b�E�g�E��E�`�E�ύX
- �E�c�E�[�E��E��E�`�E�惍�W�E�b�E�N�E�̕ύX

### �E��E��E�ؕ��E�@
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

### �E�ړI
LineTool �E�g�E�p�E��E��E��E� ToolPropertyPanel �E�ցu�E��E��E�̑��E��E��E�v�E�X�E��E��E�C�E�_�E�[�E��E�\�E��E��E��E��E��E�B
ToolDescriptor �E��E� LineTool �E�p�E�v�E��E��E�p�E�e�E�B�E��E�`�E��E�ǉ��E��E��E�AToolPropertyPanel �E��E��E�őΉ��E��E��E��E�E�E�B�E�W�E�F�E�b�E�g�E�𐶐��E�E�E�ڑ��E��E��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h
- src/app/panels/ToolPropertyPanel.cpp
- src/app/panels/ToolPropertyPanel.h

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- LineTool �E�I�E��E��E��E��E��E� ToolPropertyPanel �E�Ɂu�E��E��E�̑��E��E��E�v�E�X�E��E��E�C�E�_�E�[�E��E��E�\�E��E��E��E��E��E��E�
- �E�X�E��E��E�C�E�_�E�[�E��E��E��E�ŕ`�E��E��E��E�̑��E��E��E��E��E�ω��E��E��E��E�
- REVIEW_REQUIRED�E�iUI�E�ύX�E�̂��E�߁j

### �E��E��E�Ԍ��E�ς��E��E�
45?60 �E��E�

### �E�֎~�E��E��E��E�
- LineTool.cpp �E�̃A�E��E��E�S�E��E��E�Y�E��E��E�ύX
- �E�V�E�K�E�N�E��E��E�X�E�E�E�V�E�K�E�t�E�@�E�C�E��E��E�̍쐬

### �E��E��E�ؕ��E�@
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

### �E�ړI
ToolDescriptor.cpp �E�ɎU�E�݂��E��E�X�E��E��E�C�E�_�E�[�E��E��E��E��E�l�E�E�E�ŏ��E�l�E�E�E�ő�l�E�̃}�E�W�E�b�E�N�E�i�E��E��E�o�E�[�E��E� constexpr �E��E��E�O�E�t�E��E��E�萔�ɒu�E��E��E��E��E��E��E�A�E��E��E��E��E�̒l�E�ύX�E��E��E��E�ӏ��E�ŊǗ��E�ł��E��E�悤�E�ɂ��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- ToolDescriptor.cpp �E��E��E�̃X�E��E��E�C�E�_�E�[�E�ݒ萔�E�l�E��E� constexpr �E�萔�ɒu�E��E��E��E��E��E��E��E�Ă��E��E�i�E�R�E�[�E�h�E��E��E�r�E��E��E�[�E�Ŋm�E�F�E�j
- �E��E��E��E�ω��E�Ȃ��E�i�E��E��E�t�E�@�E�N�E�^�E�̂݁j

### �E��E��E�Ԍ��E�ς��E��E�
30?45 �E��E�

### �E�֎~�E��E��E��E�
- �E�X�E��E��E�C�E�_�E�[�E�̒l�E��E�E�E�f�E�t�E�H�E��E��E�g�E�l�E�̕ύX
- ToolPropertyPanel.cpp �E�̕ύX

### �E��E��E�ؕ��E�@
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

### �E�ړI
AppController �E�̕ۑ��E�E�E�G�E�N�E�X�E�|�E�[�E�g�E�EUndo�E�ERedo �E��E��E��E��E�Ńh�E�L�E��E��E��E��E��E��E�g�E�|�E�C�E��E��E�^�E��E� nullptr �E�̏ꍇ�E�ɖ��E��E�`�E��E��E��E��E��E��E��E��E�郊�X�E�N�E��E�r�E��E��E��E��E��E�B
�E�e�E��E��E��E�̖`�E��E��E��E� nullptr �E�K�E�[�E�h�E��E�ǉ��E��E��E�A�E�h�E�L�E��E��E��E��E��E��E�g�E��E��E��E��E��E�Ԃł̑��E��E��E��E��E��E�S�E�ɖ��E��E��E��E��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/bridge/AppController.cpp

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- �E�ۑ��E�EUndo�E�ERedo �E�e�E�n�E��E��E�h�E��E��E�̖`�E��E��E��E� nullptr �E�`�E�F�E�b�E�N�E��E��E�ǉ��E��E��E��E�Ă��E��E�i�E�R�E�[�E�h�E��E��E�r�E��E��E�[�E�Ŋm�E�F�E�j
- �E�V�E�K�E�N�E��E��E��E��E��E��E� Ctrl+Z �E�A�E�ł��E�Ă� �E�N�E��E��E�b�E�V�E��E��E��E��E�Ȃ�

### �E��E��E�Ԍ��E�ς��E��E�
30 �E��E�

### �E�֎~�E��E��E��E�
- AppController.h �E�̃V�E�O�E�i�E��E��E��E�`�E�ύX
- Document.cpp �E�̕ύX

### �E��E��E�ؕ��E�@
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

### �E�ړI
GradientTool �E�I�E��E��E��E��E��E� ToolPropertyPanel �E�ցu�E�O�E��E��E�f�E�[�E�V�E��E��E��E��E��E�ށv�E�Z�E��E��E�N�E�^�E�i�E��E��E�` / �E��E��E�ˏ�j�E��E�ǉ��E��E��E��E�B
ToolDescriptor �E��E� GradientTool �E�p enum �E�v�E��E��E�p�E�e�E�B�E��E��E�`�E��E��E�AToolPropertyPanel �E��E� QComboBox �E�Ƃ��E�ĕ\�E��E��E�E�E�ڑ��E��E��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h
- src/app/panels/ToolPropertyPanel.cpp
- src/app/panels/ToolPropertyPanel.h

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- GradientTool �E�I�E��E��E��E��E��E� ToolPropertyPanel �E�Ɂu�E��E��E�` / �E��E��E�ˏ�v�E�̐ؑ� ComboBox �E��E��E�\�E��E��E��E��E��E��E�
- �E�֑ؑ��E��E�ŕ`�E��E�O�E��E��E�f�E�[�E�V�E��E��E��E��E�̎�ނ��E�ς��E�
- REVIEW_REQUIRED�E�iUI�E�ύX�E�̂��E�߁j

### �E��E��E�Ԍ��E�ς��E��E�
45?60 �E��E�

### �E�֎~�E��E��E��E�
- GradientTool.cpp �E�̕`�E��E�A�E��E��E�S�E��E��E�Y�E��E��E�V�E�K�E��E��E��E�
- �E�V�E�K�E�N�E��E��E�X�E�E�E�V�E�K�E�t�E�@�E�C�E��E��E�̍쐬

### �E��E��E�ؕ��E�@
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

### �E�ړI
LayerPanel.cpp �E�ɎU�E�݂��E�郌�C�E��E��E�[�E�T�E��E��E�l�E�C�E��E��E�T�E�C�E�Y�E�E�E�s�E��E��E��E��E��E��E�̃n�E�[�E�h�E�R�E�[�E�h�E��E��E�l�E�𖼑O�E�t�E��E��E�萔�istatic constexpr int�E�j�E�ɒu�E��E��E��E��E��E��E�A�E��E�ӏ��E�ŊǗ��E�ł��E��E�悤�E�ɂ��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/panels/LayerPanel.cpp
- src/app/panels/LayerPanel.h

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- LayerPanel.cpp �E��E��E�̃T�E��E��E�l�E�C�E��E��E�E�E�s�E��E��E�T�E�C�E�Y�E��E��E�l�E��E� constexpr �E�萔�ɒu�E��E��E��E��E��E��E��E�Ă��E��E�i�E�R�E�[�E�h�E��E��E�r�E��E��E�[�E�Ŋm�E�F�E�j
- �E��E��E�C�E��E��E�[�E�p�E�l�E��E��E�̌��E��E��E�ځE�E��E��E��E�ɕω��E�Ȃ�

### �E��E��E�Ԍ��E�ς��E��E�
30 �E��E�

### �E�֎~�E��E��E��E�
- �E��E��E�C�E��E��E�[�E�p�E�l�E��E��E�̎��E�ۂ̃T�E�C�E�Y�E�E�E��E��E��E��E�ڂ̕ύX
- LayerPanel �E�̃V�E�O�E�i�E��E��E��E�`�E�ύX

### �E��E��E�ؕ��E�@
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

### �E�ړI
�E�h�E�L�E��E��E��E��E��E��E�g�E�ɖ��E�ۑ��E�̕ύX�E��E��E��E��E��E�ꍁE��AMainWindow �E�̃^�E�C�E�g�E��E��E�o�E�[�E�ɃA�E�X�E�^�E��E��E�X�E�N�E�i�E��E�: �E�u�E��E��E��E� *�E�v�E�j�E��E�t�E��E��E��E��E�ă_�E�[�E�e�E�B�E��E�Ԃ��E��E��E��E��E�B
AppController �E�̕ύX�E�ʒm�E�V�E�O�E�i�E��E��E��E� MainWindow �E�Ŏ󂯎��E� setWindowModified() �E�Ŕ��E�f�E��E��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/mainwindow/MainWindow.cpp
- src/app/mainwindow/MainWindow.h
- src/app/bridge/AppController.h�E�i�E�V�E�O�E�i�E��E��E�m�E�F�E�̂݁E�E�ύX�E�j

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- �E�`�E�摀�E��E��E�Ƀ^�E�C�E�g�E��E��E�o�E�[�E�Ɂu*�E�v�E��E��E�t�E��E�
- �E�ۑ��E��E�Ɂu*�E�v�E��E��E��E��E��E��E��E�
- REVIEW_REQUIRED�E�iUI�E�ύX�E�̂��E�߁j

### �E��E��E�Ԍ��E�ς��E��E�
30?45 �E��E�

### �E�֎~�E��E��E��E�
- AppController �E�̃V�E�O�E�i�E��E��E�V�E�K�E�ǉ��E�i�E��E��E��E��E�V�E�O�E�i�E��E��E��E��E�g�E��E��E��E��E�Ɓj
- Document.cpp �E�̕ύX

### �E��E��E�ؕ��E�@
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

### �E�ړI
SelectionOverlayRenderer.cpp �E�̕`�E��E�֐��E��E� QPainter::begin() �E��E�ɑ��E��E� return �E��E��E��E�p�E�X�E��E��E��E��E�݂��E��E�ꍁE��Aend() �E��E��E�Ă΂��E�Ɋ֐��E�𔲂��E��E��E� Qt �E�̌x�E��E��E�E�E�`�E��E�A�E�[�E�e�E�B�E�t�E�@�E�N�E�g�E��E��E��E��E��E��E��E��E��E�B
early return �E�O�E�ɕK�E��E� QPainter::end() �E��E��E�ĂԂ��E�ARAII �E��E��E�b�E�p�E�[�E�i�E�X�E�^�E�b�E�N�E��E��E� QPainter�E�j�E�ɐ؂�ւ��E�Ĉ��E�S�E��E��E��E��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/canvasview/SelectionOverlayRenderer.cpp

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- SelectionOverlayRenderer �E�̑S�E�`�E��E�֐��E��E� QPainter �E��E��E�K�E��E��E�I�E��E��E��E��E��E��E�i�E�R�E�[�E�h�E��E��E�r�E��E��E�[�E�Ŋm�E�F�E�j
- �E�I�E��E�̈�I�E�[�E�o�E�[�E��E��E�C�E�`�E�掞��E�Qt�E�́upainter not ended�E�v�E�x�E��E��E��E��E�o�E�Ȃ�

### �E��E��E�Ԍ��E�ς��E��E�
30 �E��E�

### �E�֎~�E��E��E��E�
- �E�I�E��E�̈�̕`�E��E�X�E�^�E�C�E��E��E�i�E�F�E�E�E�_�E��E��E�p�E�^�E�[�E��E��E��E��E�j�E�̕ύX
- SelectionOverlayRenderer.h �E�̃C�E��E��E�^�E�[�E�t�E�F�E�C�E�X�E�ύX

### �E��E��E�ؕ��E�@
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

### �E�ړI
TextTool �E�I�E��E��E��E��E��E� ToolPropertyPanel �E�փt�E�H�E��E��E�g�E�T�E�C�E�Y�E��E��E�͗p QSpinBox �E��E�ǉ��E��E��E��E�B
ToolDescriptor �E��E� TextTool �E�p�E�t�E�H�E��E��E�g�E�T�E�C�E�Y�E�v�E��E��E�p�E�e�E�B�E��E��E�`�E��E��E�AToolPropertyPanel �E��E��E�ŃX�E�s�E��E��E�{�E�b�E�N�E�X�E�Ƃ��E�ĕ\�E��E��E�ETextTool �E�ɒl�E��E�n�E��E��E�ڑ��E��E��E�s�E��E��E�B

### �E�Ώۃt�E�@�E�C�E��E��E��E��E�
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h
- src/app/panels/ToolPropertyPanel.cpp
- src/app/panels/ToolPropertyPanel.h
- src/core/tools/TextTool.cpp
- src/core/tools/TextTool.h

### �E��E��E��E��E��E��E��E�
- cmake --build �E�ŃG�E��E��E�[ 0 �E��E�
- TextTool �E�I�E��E��E��E��E��E� ToolPropertyPanel �E�Ƀt�E�H�E��E��E�g�E�T�E�C�E�Y SpinBox�E�i�E��E�: 8?144pt�E�j�E��E��E�\�E��E��E��E��E��E��E�
- SpinBox �E�̒l�E�ύX�E��E� TextTool �E�ɔ��E�f�E��E��E��E��E�i�E�`�E�敶�E��E��E�T�E�C�E�Y�E��E��E�ς��E�j
- REVIEW_REQUIRED�E�iUI�E�ύX�E�̂��E�߁j

### �E��E��E�Ԍ��E�ς��E��E�
60?90 �E��E�

### �E�֎~�E��E��E��E�
- TextTool �E�̃e�E�L�E�X�E�g�E��E��E�̓_�E�C�E�A�E��E��E�O�E�S�E�ʍ��E�V
- �E�t�E�H�E��E��E�g�E�t�E�@�E�~�E��E��E�[�E�I�E��E��E�@�E�\�E�̓��E��E��E�ǉ��E�i�E�{�E�^�E�X�E�N�E�̓T�E�C�E�Y�E�̂݁j
- �E�V�E�K�E�N�E��E��E�X�E�E�E�V�E�K�E�t�E�@�E�C�E��E��E�̍쐬

### �E��E��E�ؕ��E�@
```powershell
cmake --build build --config Release
.\launch.bat
```

---
