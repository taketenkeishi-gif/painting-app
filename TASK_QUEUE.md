# TASK QUEUE ?EPainting-app ai-night-test
<!-- scheduler ??Estatus ??X?V?????E-->

## task-006
status: completed   
priority: high
role: developer

### ???EToolPropertyPanel.cpp ??u?u???V?T?C?Y?v?X???C?_?[?? ToolTip ????????AE???? BrushSizeSlider ??? setToolTip ?????????E????E??E????E????i?K?I????{???�E�E�E�??:

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
.\launch.bat   # ?N????AToolPropertyPanel ??X???C?_?[???\???E???????�E�E�E�??????m?E```

### review_required_when
- UI??X??????????E??E
---

## task-002
status: blocked
priority: high
role: developer

### ???EFillTool ??ESelectionMask ???A?N?`E???u?????AE?????E???E?E?s?N?Z?????h??????�E�E�E��E�E�E�E???C??????AE????? SelectionMask ????????S??h????????????????E???E?Eystem_status: PARTIAL?E??AE
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
- antiAlias=true: ???????? AA ?`???E??�E�E�E��E�E�E�?????IE- antiAlias=false: ?n?E?h?G?`E???E?Eaussian ????j?`????E??????
- ToolPropertyPanel ?? antiAlias ?`?F?`E???{?b?N?X?????????????

### ??~????E- Skia ??s?Elibmypaint ?????i??t?F?[?Y?E?E- stampCircleAA ?????E???K????t?@?N?^

### ???????E```powershell
cmake --build build --config Release
# ?N????E antiAlias OFF ??u???V?X?g???[?N???n?[?h?G?`E?????�E�E�E�???m?E```

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
.\launch.bat   # Navigator ?? 100%/Fit ?{?^?????\???E?????�E�E�E�??????m?E```

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
.\launch.bat   # color dock ?? swap/reset ?{?^?????\???E?????�E�E�E�??????m?E```

### review_required_when
- UI??X??????????E??E

## task-7
status: completed
attempt: 1
completed_by: worker-b
summary: MainWindow コンストラクタに viewTransformChanged ↁEm_zoomStatusLabel 更新の signal-slot 接続を追加。既存�E updateZoomStatusLabel (CanvasWidget 側 findChild) はそ�Eまま残し、MainWindow 側の正式接続を追加した。ビルドエラー0件確認済み、Epriority: medium
role: developer
category: feature

### �E�E�E�ړI
MainWindow �E�E�E�̃X�E�E�E�e�E�E�E�[�E�E�E�^�E�E�E�X�E�E�E�o�E�E�E�[�E�E�E�Ɍ��E�E�E�݂̃Y�E�E�E�[�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�i�E�E�E��E�E�E�: 100%�E�E�E�j�E�E�E��E�E�E�\�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�B
CanvasWidget �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�Y�E�E�E�[�E�E�E��E�E�E��E�E�E�ύX�E�E�E�V�E�E�E�O�E�E�E�i�E�E�E��E�E�E��E�E�E��E�E�E� MainWindow �E�E�E�Ŏ󂯎��E�E�E�AQLabel �E�E�E�ɔ��E�E�E�f�E�E�E��E�E�E��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/mainwindow/MainWindow.cpp
- src/app/mainwindow/MainWindow.h
- src/app/canvasview/CanvasWidget.h�E�E�E�i�E�E�E�V�E�E�E�O�E�E�E�i�E�E�E��E�E�E��E�E�E�m�E�E�E�F�E�E�E�̂݁E�E�E�E�ύX�E�E�E�j

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- �E�E�E�A�E�E�E�v�E�E�E��E�E�E��E�E�E�N�E�E�E��E�E�E��E�E�E��E�E�E�A�E�E�E�X�E�E�E�e�E�E�E�[�E�E�E�^�E�E�E�X�E�E�E�o�E�E�E�[�E�E�E�Ɂu100%�E�E�E�v�E�E�E��E�E�E��E�E�E�̃Y�E�E�E�[�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�\�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- �E�E�E�Y�E�E�E�[�E�E�E��E�E�E��E�E�E�C�E�E�E��E�E�E��E�E�E�^�E�E�E�A�E�E�E�E�E�E�E�g�E�E�E��E�E�E��E�E�E��E�E�E�Ő��E�E�E�l�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�A�E�E�E��E�E�E��E�E�E�^�E�E�E�C�E�E�E��E�E�E��E�E�E�X�E�E�E�V�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- REVIEW_REQUIRED�E�E�E�iUI�E�E�E�ύX�E�E�E�̂��E�E�E�߁j

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
30?45 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- CanvasWidget �E�E�E�̃Y�E�E�E�[�E�E�E��E�E�E��E�E�E�v�E�E�E�Z�E�E�E��E�E�E��E�E�E�W�E�E�E�b�E�E�E�N�E�E�E�ύX
- �E�E�E�V�E�E�E�K�E�E�E�N�E�E�E��E�E�E��E�E�E�X�E�E�E�E�E�E�E�V�E�E�E�K�E�E�E�t�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E�̍쐬

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-8
status: completed
attempt: 1
priority: high
role: developer
category: bug

### 完亁E��要EmousePressEvent / mouseMoveEvent / mouseReleaseEvent の nullptr ガードに
`m_mouseDrawing = false` と `hasLastStrokeDispatchPos = false` のリセチE��を追加、Eコントローラ刁E��時にストローク状態が残るバグを修正。ビルド確誁E エラー0件、E
### �E�E�E�ړI
CanvasWidget �E�E�E�̃}�E�E�E�E�E�E�E�X�E�E�E�C�E�E�E�x�E�E�E��E�E�E��E�E�E�g�E�E�E�n�E�E�E��E�E�E��E�E�E�h�E�E�E��E�E�E��E�E�E�imousePressEvent / mouseMoveEvent / mouseReleaseEvent�E�E�E�j�E�E�E�ŃJ�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�g�E�E�E�c�E�E�E�[�E�E�E��E�E�E��E�E�E�|�E�E�E�C�E�E�E��E�E�E��E�E�E�^�E�E�E��E�E�E� nullptr �E�E�E�̏ꍇ�E�E�E�ɃN�E�E�E��E�E�E��E�E�E�b�E�E�E�V�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�h�E�E�E��E�E�E��E�E�E�B
�E�E�E�c�E�E�E�[�E�E�E��E�E�E��E�E�E�ؑ֒��E�E�E�̋}�E�E�E��E�E�E��E�E�E�ȓ��E�E�E�͂�V�E�E�E��E�E�E��E�E�E�b�E�E�E�g�E�E�E�_�E�E�E�E�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�̌둀�E�E�E��E�E�E�Ńc�E�E�E�[�E�E�E��E�E�E��E�E�E�|�E�E�E�C�E�E�E��E�E�E��E�E�E�^�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�ݒ�̂܂܃C�E�E�E�x�E�E�E��E�E�E��E�E�E�g�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�P�E�E�E�[�E�E�E�X�E�E�E�ɑΏ��E�E�E��E�E�E��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/canvasview/CanvasWidget.cpp

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- �E�E�E�e�E�E�E�}�E�E�E�E�E�E�E�X�E�E�E�C�E�E�E�x�E�E�E��E�E�E��E�E�E�g�E�E�E�`�E�E�E��E�E�E��E�E�E��E�E�E� nullptr �E�E�E�K�E�E�E�[�E�E�E�h�E�E�E��E�E�E��E�E�E�ǉ��E�E�E��E�E�E��E�E�E��E�E�E�Ă��E�E�E��E�E�E�i�E�E�E�R�E�E�E�[�E�E�E�h�E�E�E��E�E�E��E�E�E�r�E�E�E��E�E�E��E�E�E�[�E�E�E�Ŋm�E�E�E�F�E�E�E�j
- �E�E�E�c�E�E�E�[�E�E�E��E�E�E��E�E�E�ؑ֒��E�E�E��E�E�E�̘A�E�E�E��E�E�E��E�E�E�N�E�E�E��E�E�E��E�E�E�b�E�E�E�N�E�E�E�ŃN�E�E�E��E�E�E��E�E�E�b�E�E�E�V�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�Ȃ��E�E�E�i�E�E�E��E�E�E��E�E�E�s�E�E�E�o�E�E�E�H�E�E�E��E�E�E��E�E�E�R�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�g�E�E�E�ŋL�E�E�E�^�E�E�E�j

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
30 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- CanvasWidget.h �E�E�E�̃V�E�E�E�O�E�E�E�i�E�E�E��E�E�E��E�E�E�E�E�E�E�X�E�E�E��E�E�E��E�E�E�b�E�E�E�g�E�E�E��E�E�E�`�E�E�E�ύX
- �E�E�E�c�E�E�E�[�E�E�E��E�E�E��E�E�E�`�E�E�E�惍�W�E�E�E�b�E�E�E�N�E�E�E�̕ύX

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
```

---

## task-9
status: completed
attempt: 0
priority: medium
role: developer
category: feature

### �E�E�E�ړI
LineTool �E�E�E�g�E�E�E�p�E�E�E��E�E�E��E�E�E��E�E�E� ToolPropertyPanel �E�E�E�ցu�E�E�E��E�E�E��E�E�E�̑��E�E�E��E�E�E��E�E�E�v�E�E�E�X�E�E�E��E�E�E��E�E�E�C�E�E�E�_�E�E�E�[�E�E�E��E�E�E�\�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�B
ToolDescriptor �E�E�E��E�E�E� LineTool �E�E�E�p�E�E�E�v�E�E�E��E�E�E��E�E�E�p�E�E�E�e�E�E�E�B�E�E�E��E�E�E�`�E�E�E��E�E�E�ǉ��E�E�E��E�E�E��E�E�E�AToolPropertyPanel �E�E�E��E�E�E��E�E�E�őΉ��E�E�E��E�E�E��E�E�E��E�E�E�E�E�E�E�B�E�E�E�W�E�E�E�F�E�E�E�b�E�E�E�g�E�E�E�𐶐��E�E�E�E�E�E�E�ڑ��E�E�E��E�E�E��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h
- src/app/panels/ToolPropertyPanel.cpp
- src/app/panels/ToolPropertyPanel.h

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- LineTool �E�E�E�I�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E� ToolPropertyPanel �E�E�E�Ɂu�E�E�E��E�E�E��E�E�E�̑��E�E�E��E�E�E��E�E�E�v�E�E�E�X�E�E�E��E�E�E��E�E�E�C�E�E�E�_�E�E�E�[�E�E�E��E�E�E��E�E�E�\�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- �E�E�E�X�E�E�E��E�E�E��E�E�E�C�E�E�E�_�E�E�E�[�E�E�E��E�E�E��E�E�E��E�E�E�ŕ`�E�E�E��E�E�E��E�E�E��E�E�E�̑��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�ω��E�E�E��E�E�E��E�E�E��E�E�E�
- REVIEW_REQUIRED�E�E�E�iUI�E�E�E�ύX�E�E�E�̂��E�E�E�߁j

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
45?60 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- LineTool.cpp �E�E�E�̃A�E�E�E��E�E�E��E�E�E�S�E�E�E��E�E�E��E�E�E�Y�E�E�E��E�E�E��E�E�E�ύX
- �E�E�E�V�E�E�E�K�E�E�E�N�E�E�E��E�E�E��E�E�E�X�E�E�E�E�E�E�E�V�E�E�E�K�E�E�E�t�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E�̍쐬

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-10
status: completed
attempt: 0
priority: low
role: developer
category: refactor

### �E�E�E�ړI
ToolDescriptor.cpp �E�E�E�ɎU�E�E�E�݂��E�E�E��E�E�E�X�E�E�E��E�E�E��E�E�E�C�E�E�E�_�E�E�E�[�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�l�E�E�E�E�E�E�E�ŏ��E�E�E�l�E�E�E�E�E�E�E�ő�l�E�E�E�̃}�E�E�E�W�E�E�E�b�E�E�E�N�E�E�E�i�E�E�E��E�E�E��E�E�E�o�E�E�E�[�E�E�E��E�E�E� constexpr �E�E�E��E�E�E��E�E�E�O�E�E�E�t�E�E�E��E�E�E��E�E�E�萔�ɒu�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�A�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�̒l�E�E�E�ύX�E�E�E��E�E�E��E�E�E��E�E�E�ӏ��E�E�E�ŊǗ��E�E�E�ł��E�E�E��E�E�E�悤�E�E�E�ɂ��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- ToolDescriptor.cpp �E�E�E��E�E�E��E�E�E�̃X�E�E�E��E�E�E��E�E�E�C�E�E�E�_�E�E�E�[�E�E�E�ݒ萔�E�E�E�l�E�E�E��E�E�E� constexpr �E�E�E�萔�ɒu�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�Ă��E�E�E��E�E�E�i�E�E�E�R�E�E�E�[�E�E�E�h�E�E�E��E�E�E��E�E�E�r�E�E�E��E�E�E��E�E�E�[�E�E�E�Ŋm�E�E�E�F�E�E�E�j
- �E�E�E��E�E�E��E�E�E��E�E�E�ω��E�E�E�Ȃ��E�E�E�i�E�E�E��E�E�E��E�E�E�t�E�E�E�@�E�E�E�N�E�E�E�^�E�E�E�̂݁j

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
30?45 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- �E�E�E�X�E�E�E��E�E�E��E�E�E�C�E�E�E�_�E�E�E�[�E�E�E�̒l�E�E�E��E�E�E�E�E�E�E�f�E�E�E�t�E�E�E�H�E�E�E��E�E�E��E�E�E�g�E�E�E�l�E�E�E�̕ύX
- ToolPropertyPanel.cpp �E�E�E�̕ύX

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
```

---

## task-11
status: completed
attempt: 0
priority: high
role: developer
category: bug

### �E�E�E�ړI
AppController �E�E�E�̕ۑ��E�E�E�E�E�E�E�G�E�E�E�N�E�E�E�X�E�E�E�|�E�E�E�[�E�E�E�g�E�E�E�EUndo�E�E�E�ERedo �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�Ńh�E�E�E�L�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�g�E�E�E�|�E�E�E�C�E�E�E��E�E�E��E�E�E�^�E�E�E��E�E�E� nullptr �E�E�E�̏ꍇ�E�E�E�ɖ��E�E�E��E�E�E�`�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�郊�X�E�E�E�N�E�E�E��E�E�E�r�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�B
�E�E�E�e�E�E�E��E�E�E��E�E�E��E�E�E�̖`�E�E�E��E�E�E��E�E�E��E�E�E� nullptr �E�E�E�K�E�E�E�[�E�E�E�h�E�E�E��E�E�E�ǉ��E�E�E��E�E�E��E�E�E�A�E�E�E�h�E�E�E�L�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�g�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�Ԃł̑��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�S�E�E�E�ɖ��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/bridge/AppController.cpp

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- �E�E�E�ۑ��E�E�E�EUndo�E�E�E�ERedo �E�E�E�e�E�E�E�n�E�E�E��E�E�E��E�E�E�h�E�E�E��E�E�E��E�E�E�̖`�E�E�E��E�E�E��E�E�E��E�E�E� nullptr �E�E�E�`�E�E�E�F�E�E�E�b�E�E�E�N�E�E�E��E�E�E��E�E�E�ǉ��E�E�E��E�E�E��E�E�E��E�E�E�Ă��E�E�E��E�E�E�i�E�E�E�R�E�E�E�[�E�E�E�h�E�E�E��E�E�E��E�E�E�r�E�E�E��E�E�E��E�E�E�[�E�E�E�Ŋm�E�E�E�F�E�E�E�j
- �E�E�E�V�E�E�E�K�E�E�E�N�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E� Ctrl+Z �E�E�E�A�E�E�E�ł��E�E�E�Ă� �E�E�E�N�E�E�E��E�E�E��E�E�E�b�E�E�E�V�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�Ȃ�

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
30 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- AppController.h �E�E�E�̃V�E�E�E�O�E�E�E�i�E�E�E��E�E�E��E�E�E��E�E�E�`�E�E�E�ύX
- Document.cpp �E�E�E�̕ύX

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
```

---

## task-12
status: completed
attempt: 0
priority: medium
role: developer
category: feature

### �E�E�E�ړI
GradientTool �E�E�E�I�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E� ToolPropertyPanel �E�E�E�ցu�E�E�E�O�E�E�E��E�E�E��E�E�E�f�E�E�E�[�E�E�E�V�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�ށv�E�E�E�Z�E�E�E��E�E�E��E�E�E�N�E�E�E�^�E�E�E�i�E�E�E��E�E�E��E�E�E�` / �E�E�E��E�E�E��E�E�E�ˏ�j�E�E�E��E�E�E�ǉ��E�E�E��E�E�E��E�E�E��E�E�E�B
ToolDescriptor �E�E�E��E�E�E� GradientTool �E�E�E�p enum �E�E�E�v�E�E�E��E�E�E��E�E�E�p�E�E�E�e�E�E�E�B�E�E�E��E�E�E��E�E�E�`�E�E�E��E�E�E��E�E�E�AToolPropertyPanel �E�E�E��E�E�E� QComboBox �E�E�E�Ƃ��E�E�E�ĕ\�E�E�E��E�E�E��E�E�E�E�E�E�E�ڑ��E�E�E��E�E�E��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h
- src/app/panels/ToolPropertyPanel.cpp
- src/app/panels/ToolPropertyPanel.h

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- GradientTool �E�E�E�I�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E� ToolPropertyPanel �E�E�E�Ɂu�E�E�E��E�E�E��E�E�E�` / �E�E�E��E�E�E��E�E�E�ˏ�v�E�E�E�̐ؑ� ComboBox �E�E�E��E�E�E��E�E�E�\�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- �E�E�E�֑ؑ��E�E�E��E�E�E�ŕ`�E�E�E��E�E�E�O�E�E�E��E�E�E��E�E�E�f�E�E�E�[�E�E�E�V�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�̎�ނ��E�E�E�ς��E�E�E�
- REVIEW_REQUIRED�E�E�E�iUI�E�E�E�ύX�E�E�E�̂��E�E�E�߁j

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
45?60 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- GradientTool.cpp �E�E�E�̕`�E�E�E��E�E�E�A�E�E�E��E�E�E��E�E�E�S�E�E�E��E�E�E��E�E�E�Y�E�E�E��E�E�E��E�E�E�V�E�E�E�K�E�E�E��E�E�E��E�E�E��E�E�E�
- �E�E�E�V�E�E�E�K�E�E�E�N�E�E�E��E�E�E��E�E�E�X�E�E�E�E�E�E�E�V�E�E�E�K�E�E�E�t�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E�̍쐬

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-13
status: completed
attempt: 0
priority: low
role: developer
category: refactor

### �E�E�E�ړI
LayerPanel.cpp �E�E�E�ɎU�E�E�E�݂��E�E�E�郌�C�E�E�E��E�E�E��E�E�E�[�E�E�E�T�E�E�E��E�E�E��E�E�E�l�E�E�E�C�E�E�E��E�E�E��E�E�E�T�E�E�E�C�E�E�E�Y�E�E�E�E�E�E�E�s�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�̃n�E�E�E�[�E�E�E�h�E�E�E�R�E�E�E�[�E�E�E�h�E�E�E��E�E�E��E�E�E�l�E�E�E�𖼑O�E�E�E�t�E�E�E��E�E�E��E�E�E�萔�istatic constexpr int�E�E�E�j�E�E�E�ɒu�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�A�E�E�E��E�E�E�ӏ��E�E�E�ŊǗ��E�E�E�ł��E�E�E��E�E�E�悤�E�E�E�ɂ��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/panels/LayerPanel.cpp
- src/app/panels/LayerPanel.h

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- LayerPanel.cpp �E�E�E��E�E�E��E�E�E�̃T�E�E�E��E�E�E��E�E�E�l�E�E�E�C�E�E�E��E�E�E��E�E�E�E�E�E�E�s�E�E�E��E�E�E��E�E�E�T�E�E�E�C�E�E�E�Y�E�E�E��E�E�E��E�E�E�l�E�E�E��E�E�E� constexpr �E�E�E�萔�ɒu�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�Ă��E�E�E��E�E�E�i�E�E�E�R�E�E�E�[�E�E�E�h�E�E�E��E�E�E��E�E�E�r�E�E�E��E�E�E��E�E�E�[�E�E�E�Ŋm�E�E�E�F�E�E�E�j
- �E�E�E��E�E�E��E�E�E�C�E�E�E��E�E�E��E�E�E�[�E�E�E�p�E�E�E�l�E�E�E��E�E�E��E�E�E�̌��E�E�E��E�E�E��E�E�E�ځE�E�E�E��E�E�E��E�E�E��E�E�E�ɕω��E�E�E�Ȃ�

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
30 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- �E�E�E��E�E�E��E�E�E�C�E�E�E��E�E�E��E�E�E�[�E�E�E�p�E�E�E�l�E�E�E��E�E�E��E�E�E�̎��E�E�E�ۂ̃T�E�E�E�C�E�E�E�Y�E�E�E�E�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�ڂ̕ύX
- LayerPanel �E�E�E�̃V�E�E�E�O�E�E�E�i�E�E�E��E�E�E��E�E�E��E�E�E�`�E�E�E�ύX

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
```

---

## task-14
status: completed
attempt: 0
priority: medium
role: developer
category: feature

### �E�E�E�ړI
�E�E�E�h�E�E�E�L�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�g�E�E�E�ɖ��E�E�E�ۑ��E�E�E�̕ύX�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�ꍁE�E��E�AMainWindow �E�E�E�̃^�E�E�E�C�E�E�E�g�E�E�E��E�E�E��E�E�E�o�E�E�E�[�E�E�E�ɃA�E�E�E�X�E�E�E�^�E�E�E��E�E�E��E�E�E�X�E�E�E�N�E�E�E�i�E�E�E��E�E�E�: �E�E�E�u�E�E�E��E�E�E��E�E�E��E�E�E� *�E�E�E�v�E�E�E�j�E�E�E��E�E�E�t�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�ă_�E�E�E�[�E�E�E�e�E�E�E�B�E�E�E��E�E�E�Ԃ��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�B
AppController �E�E�E�̕ύX�E�E�E�ʒm�E�E�E�V�E�E�E�O�E�E�E�i�E�E�E��E�E�E��E�E�E��E�E�E� MainWindow �E�E�E�Ŏ󂯎��E�E�E� setWindowModified() �E�E�E�Ŕ��E�E�E�f�E�E�E��E�E�E��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/mainwindow/MainWindow.cpp
- src/app/mainwindow/MainWindow.h
- src/app/bridge/AppController.h�E�E�E�i�E�E�E�V�E�E�E�O�E�E�E�i�E�E�E��E�E�E��E�E�E�m�E�E�E�F�E�E�E�̂݁E�E�E�E�ύX�E�E�E�j

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- �E�E�E�`�E�E�E�摀�E�E�E��E�E�E��E�E�E�Ƀ^�E�E�E�C�E�E�E�g�E�E�E��E�E�E��E�E�E�o�E�E�E�[�E�E�E�Ɂu*�E�E�E�v�E�E�E��E�E�E��E�E�E�t�E�E�E��E�E�E�
- �E�E�E�ۑ��E�E�E��E�E�E�Ɂu*�E�E�E�v�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- REVIEW_REQUIRED�E�E�E�iUI�E�E�E�ύX�E�E�E�̂��E�E�E�߁j

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
30?45 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- AppController �E�E�E�̃V�E�E�E�O�E�E�E�i�E�E�E��E�E�E��E�E�E�V�E�E�E�K�E�E�E�ǉ��E�E�E�i�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�V�E�E�E�O�E�E�E�i�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�g�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�Ɓj
- Document.cpp �E�E�E�̕ύX

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-15
status: completed
attempt: 0
priority: medium
role: developer
category: bug

### �E�E�E�ړI
SelectionOverlayRenderer.cpp �E�E�E�̕`�E�E�E��E�E�E�֐��E�E�E��E�E�E� QPainter::begin() �E�E�E��E�E�E�ɑ��E�E�E��E�E�E� return �E�E�E��E�E�E��E�E�E��E�E�E�p�E�E�E�X�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�݂��E�E�E��E�E�E�ꍁE�E��E�Aend() �E�E�E��E�E�E��E�E�E�Ă΂��E�E�E�Ɋ֐��E�E�E�𔲂��E�E�E��E�E�E��E�E�E� Qt �E�E�E�̌x�E�E�E��E�E�E��E�E�E�E�E�E�E�`�E�E�E��E�E�E�A�E�E�E�[�E�E�E�e�E�E�E�B�E�E�E�t�E�E�E�@�E�E�E�N�E�E�E�g�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�B
early return �E�E�E�O�E�E�E�ɕK�E�E�E��E�E�E� QPainter::end() �E�E�E��E�E�E��E�E�E�ĂԂ��E�E�E�ARAII �E�E�E��E�E�E��E�E�E�b�E�E�E�p�E�E�E�[�E�E�E�i�E�E�E�X�E�E�E�^�E�E�E�b�E�E�E�N�E�E�E��E�E�E��E�E�E� QPainter�E�E�E�j�E�E�E�ɐ؂�ւ��E�E�E�Ĉ��E�E�E�S�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/canvasview/SelectionOverlayRenderer.cpp

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- SelectionOverlayRenderer �E�E�E�̑S�E�E�E�`�E�E�E��E�E�E�֐��E�E�E��E�E�E� QPainter �E�E�E��E�E�E��E�E�E�K�E�E�E��E�E�E��E�E�E�I�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�i�E�E�E�R�E�E�E�[�E�E�E�h�E�E�E��E�E�E��E�E�E�r�E�E�E��E�E�E��E�E�E�[�E�E�E�Ŋm�E�E�E�F�E�E�E�j
- �E�E�E�I�E�E�E��E�E�E�̈�I�E�E�E�[�E�E�E�o�E�E�E�[�E�E�E��E�E�E��E�E�E�C�E�E�E�`�E�E�E�掞��E�E�E�Qt�E�E�E�́upainter not ended�E�E�E�v�E�E�E�x�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�o�E�E�E�Ȃ�

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
30 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- �E�E�E�I�E�E�E��E�E�E�̈�̕`�E�E�E��E�E�E�X�E�E�E�^�E�E�E�C�E�E�E��E�E�E��E�E�E�i�E�E�E�F�E�E�E�E�E�E�E�_�E�E�E��E�E�E��E�E�E�p�E�E�E�^�E�E�E�[�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�j�E�E�E�̕ύX
- SelectionOverlayRenderer.h �E�E�E�̃C�E�E�E��E�E�E��E�E�E�^�E�E�E�[�E�E�E�t�E�E�E�F�E�E�E�C�E�E�E�X�E�E�E�ύX

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
```

---

## task-16
status: completed
attempt: 0
priority: medium
role: developer
category: feature

### �E�E�E�ړI
TextTool �E�E�E�I�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E� ToolPropertyPanel �E�E�E�փt�E�E�E�H�E�E�E��E�E�E��E�E�E�g�E�E�E�T�E�E�E�C�E�E�E�Y�E�E�E��E�E�E��E�E�E�͗p QSpinBox �E�E�E��E�E�E�ǉ��E�E�E��E�E�E��E�E�E��E�E�E�B
ToolDescriptor �E�E�E��E�E�E� TextTool �E�E�E�p�E�E�E�t�E�E�E�H�E�E�E��E�E�E��E�E�E�g�E�E�E�T�E�E�E�C�E�E�E�Y�E�E�E�v�E�E�E��E�E�E��E�E�E�p�E�E�E�e�E�E�E�B�E�E�E��E�E�E��E�E�E�`�E�E�E��E�E�E��E�E�E�AToolPropertyPanel �E�E�E��E�E�E��E�E�E�ŃX�E�E�E�s�E�E�E��E�E�E��E�E�E�{�E�E�E�b�E�E�E�N�E�E�E�X�E�E�E�Ƃ��E�E�E�ĕ\�E�E�E��E�E�E��E�E�E�ETextTool �E�E�E�ɒl�E�E�E��E�E�E�n�E�E�E��E�E�E��E�E�E�ڑ��E�E�E��E�E�E��E�E�E�s�E�E�E��E�E�E��E�E�E�B

### �E�E�E�Ώۃt�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- src/app/ui/ToolDescriptor.cpp
- src/app/ui/ToolDescriptor.h
- src/app/panels/ToolPropertyPanel.cpp
- src/app/panels/ToolPropertyPanel.h
- src/core/tools/TextTool.cpp
- src/core/tools/TextTool.h

### �E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- cmake --build �E�E�E�ŃG�E�E�E��E�E�E��E�E�E�[ 0 �E�E�E��E�E�E�
- TextTool �E�E�E�I�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E� ToolPropertyPanel �E�E�E�Ƀt�E�E�E�H�E�E�E��E�E�E��E�E�E�g�E�E�E�T�E�E�E�C�E�E�E�Y SpinBox�E�E�E�i�E�E�E��E�E�E�: 8?144pt�E�E�E�j�E�E�E��E�E�E��E�E�E�\�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�
- SpinBox �E�E�E�̒l�E�E�E�ύX�E�E�E��E�E�E� TextTool �E�E�E�ɔ��E�E�E�f�E�E�E��E�E�E��E�E�E��E�E�E��E�E�E�i�E�E�E�`�E�E�E�敶�E�E�E��E�E�E��E�E�E�T�E�E�E�C�E�E�E�Y�E�E�E��E�E�E��E�E�E�ς��E�E�E�j
- REVIEW_REQUIRED�E�E�E�iUI�E�E�E�ύX�E�E�E�̂��E�E�E�߁j

### �E�E�E��E�E�E��E�E�E�Ԍ��E�E�E�ς��E�E�E��E�E�E�
60?90 �E�E�E��E�E�E�

### �E�E�E�֎~�E�E�E��E�E�E��E�E�E��E�E�E�
- TextTool �E�E�E�̃e�E�E�E�L�E�E�E�X�E�E�E�g�E�E�E��E�E�E��E�E�E�̓_�E�E�E�C�E�E�E�A�E�E�E��E�E�E��E�E�E�O�E�E�E�S�E�E�E�ʍ��E�E�E�V
- �E�E�E�t�E�E�E�H�E�E�E��E�E�E��E�E�E�g�E�E�E�t�E�E�E�@�E�E�E�~�E�E�E��E�E�E��E�E�E�[�E�E�E�I�E�E�E��E�E�E��E�E�E�@�E�E�E�\�E�E�E�̓��E�E�E��E�E�E��E�E�E�ǉ��E�E�E�i�E�E�E�{�E�E�E�^�E�E�E�X�E�E�E�N�E�E�E�̓T�E�E�E�C�E�E�E�Y�E�E�E�̂݁j
- �E�E�E�V�E�E�E�K�E�E�E�N�E�E�E��E�E�E��E�E�E�X�E�E�E�E�E�E�E�V�E�E�E�K�E�E�E�t�E�E�E�@�E�E�E�C�E�E�E��E�E�E��E�E�E�̍쐬

### �E�E�E��E�E�E��E�E�E�ؕ��E�E�E�@
```powershell
cmake --build build --config Release
.\launch.bat
```

---

## task-17
status: pending
attempt: 0
priority: high
role: developer
category: feature

### 目的
`ToolPropertyPanel.cpp` にブラシの Opacity（不透明度）スライダーを追加する。既存の angle/roundness/taperStart/taperEnd スライダー実装（task-001 で確認済み）と同じパターンで、0〜100% の QSlider と QLabel を追加し、ToolDescriptor 経由でブラシツールの opacity パラメータと接続する。

### 対象ファイル候補
- `src/app/panels/ToolPropertyPanel.cpp`
- `src/app/panels/ToolPropertyPanel.h`
- `src/app/ui/ToolDescriptor.cpp`（opacity パラメータが未定義なら追記）

### 成功条件
- `cmake --build` でエラー 0 件
- ブラシツール選択時に Opacity スライダーが表示される
- スライダーを動かしても UI がクラッシュしない

### 時間見積もり
30〜60 分

### 禁止事項
- 新規クラス・新規ファイルの作成
- 他ツール（消しゴム・塗りつぶし等）への無断適用

### 検証方法
```powershell
cmake --build build --config Release
```

---
## task-18
status: pending
attempt: 0
priority: high
role: developer
category: bug

### 目的
`CanvasWidget.cpp` でツールを切り替えた際にキャンバスのカーソル形状が更新されない不具合を修正する。ブラシ選択中はクロスヘア、選択ツール選択中は矢印など、ツールに応じた `setCursor()` を `onToolChanged` ハンドラ（または同等のスロット）内で呼ぶ。

### 対象ファイル候補
- `src/app/canvasview/CanvasWidget.cpp`
- `src/app/canvasview/CanvasWidget.h`

### 成功条件
- `cmake --build` でエラー 0 件
- ツールパネルでブラシ→選択ツール→ハンドと切り替えると、キャンバス上のカーソル形状が変化する

### 時間見積もり
30〜45 分

### 禁止事項
- CanvasWidget 以外のファイルへの描画ロジック追加
- 新規シグナル・スロットの大量追加（既存接続を活用する）

### 検証方法
```powershell
cmake --build build --config Release
```

---
## task-19
status: pending
attempt: 0
priority: medium
role: developer
category: refactor

### 目的
`ToolDescriptor.cpp` に存在する QSlider 生成コード（min/max/value 設定、ラベル配置）が angle/roundness/taperStart/taperEnd で繰り返されている。共通ヘルパー関数 `createLabeledSlider(const QString& label, int min, int max, int value)` を同ファイル内に追加し、重複コードを削除する。

### 対象ファイル候補
- `src/app/ui/ToolDescriptor.cpp`
- `src/app/ui/ToolDescriptor.h`（必要に応じ private メソッド宣言）

### 成功条件
- `cmake --build` でエラー 0 件
- リファクタリング前後でスライダーの見た目・動作が変わらない（コード行数が削減されている）

### 時間見積もり
30〜45 分

### 禁止事項
- 機能変更・挙動変更
- 新規クラスの追加

### 検証方法
```powershell
cmake --build build --config Release
```

---
## task-20
status: pending
attempt: 0
priority: high
role: developer
category: feature

### 目的
`MainWindow.cpp` のステータスバーにキャンバス座標（マウス位置の X, Y）を表示する。task-7 で追加したズーム表示と同じ `QStatusBar` に `QLabel` を追加し、`CanvasWidget` の `mouseMoveEvent` から座標シグナルを受け取って更新する。

### 対象ファイル候補
- `src/app/mainwindow/MainWindow.cpp`
- `src/app/mainwindow/MainWindow.h`
- `src/app/canvasview/CanvasWidget.cpp`（座標シグナルの emit 追加）
- `src/app/canvasview/CanvasWidget.h`（シグナル宣言）

### 成功条件
- `cmake --build` でエラー 0 件
- キャンバス上でマウスを動かすとステータスバーに `X: 123  Y: 456` 形式で座標が表示される

### 時間見積もり
45〜60 分

### 禁止事項
- ズーム表示（task-7）の既存実装を壊さない
- 新規ウィジェットクラスの作成

### 検証方法
```powershell
cmake --build build --config Release
```

---
## task-21
status: pending
attempt: 0
priority: high
role: developer
category: bug

### 目的
`AppController.cpp` の「新規作成」「ファイルを開く」「アプリ終了」処理で、ドキュメントに未保存の変更がある場合に警告ダイアログが表示されない問題を修正する。`Document` の dirty フラグを確認し、`QMessageBox::question` で保存・破棄・キャンセルの選択肢を表示する。

### 対象ファイル候補
- `src/app/bridge/AppController.cpp`
- `src/app/bridge/AppController.h`
- `src/core/document/Document.h`（isDirty() が未実装なら追加）

### 成功条件
- `cmake --build` でエラー 0 件
- 未保存ドキュメントを編集後、新規作成またはアプリ終了を行うとダイアログが表示され、「キャンセル」で操作を中断できる

### 時間見積もり
45〜60 分

### 禁止事項
- ファイル保存の実装変更（保存ロジック自体はそのまま）

### 検証方法
```powershell
cmake --build build --config Release
```

---
## task-22
status: pending
attempt: 0
priority: medium
role: developer
category: refactor

### 目的
`MainWindow.cpp` 内でウィンドウタイトル（ファイル名・dirty 状態）を更新するコードが複数箇所に散在している。`updateWindowTitle()` プライベートスロットに集約し、ドキュメント変更シグナルから接続する形に整理する。

### 対象ファイル候補
- `src/app/mainwindow/MainWindow.cpp`
- `src/app/mainwindow/MainWindow.h`

### 成功条件
- `cmake --build` でエラー 0 件
- タイトルバーの表示挙動がリファクタリング前と同一である
- `updateWindowTitle` という名前のプライベートスロットが存在する

### 時間見積もり
30〜45 分

### 禁止事項
- タイトル文字列フォーマットの変更
- 他ファイルへの影響

### 検証方法
```powershell
cmake --build build --config Release
```

---
## task-23
status: pending
attempt: 0
priority: high
role: developer
category: feature

### 目的
`LayerPanel.cpp` にレイヤー不透明度スライダー（0〜100%）を追加する。選択中レイヤーの opacity 値を表示・変更でき、`LayerManager` または `Document` 経由でレイヤーに反映される。スライダー変更時に `update()` でキャンバスを再描画する。

### 対象ファイル候補
- `src/app/panels/LayerPanel.cpp`
- `src/app/panels/LayerPanel.h`

### 成功条件
- `cmake --build` でエラー 0 件
- レイヤーを選択すると、そのレイヤーの不透明度がスライダーに反映される
- スライダーを動かしてもクラッシュしない

### 時間見積もり
45〜75 分

### 禁止事項
- 新規クラス作成
- blend mode セレクターの同時追加（別タスクで対応）

### 検証方法
```powershell
cmake --build build --config Release
```

---
## task-24
status: pending
attempt: 0
priority: medium
role: developer
category: feature

### 目的
`ToolPanel.cpp` の各ツールボタンのツールチップにショートカットキーを付記する。例: `"ブラシ (B)"` `"消しゴム (E)"` のように `ToolDescriptor` から shortcut 情報を取得し `setToolTip()` で設定する。

### 対象ファイル候補
- `src/app/panels/ToolPanel.cpp`
- `src/app/ui/ToolDescriptor.cpp`（shortcut フィールドが未定義なら追加）
- `src/app/ui/ToolDescriptor.h`

### 成功条件
- `cmake --build` でエラー 0 件
- ツールパネルのボタンにマウスオーバーすると、ツール名とショートカットキーを含むツールチップが表示される

### 時間見積もり
30〜45 分

### 禁止事項
- キーボードショートカット実装の変更（QShortcut 追加等）
- 既存のボタンレイアウト変更

### 検証方法
```powershell
cmake --build build --config Release
```

---
## task-25
status: pending
attempt: 0
priority: high
role: developer
category: bug

### 目的
`SelectionOverlayRenderer.cpp` が空またはヌルの SelectionMask を受け取った際に未定義動作（クラッシュまたはゴミ描画）する可能性がある。`render()` または同等メソッドの冒頭で `SelectionMask` の有効性チェック（null ポインタ・空マスク確認）を追加し、早期リターンする。

### 対象ファイル候補
- `src/app/canvasview/SelectionOverlayRenderer.cpp`
- `src/app/canvasview/SelectionOverlayRenderer.h`

### 成功条件
- `cmake --build` でエラー 0 件
- 選択なし状態でのオーバーレイ描画呼び出しがクラッシュしない
- 選択ありの場合は従来通りのマーチングアンツが描画される

### 時間見積もり
30〜45 分

### 禁止事項
- SelectionMask・SelectionEngine の内部ロジック変更

### 検証方法
```powershell
cmake --build build --config Release
```

---
## task-26
status: pending
attempt: 0
priority: medium
role: developer
category: feature

### 目的
`ColorWheelWidget.cpp` に 16 進カラーコード入力 QLineEdit（`#RRGGBB` 形式）を追加する。カラーホイールの色変更で QLineEdit を更新し、QLineEdit 編集確定（returnPressed）でカラーホイールの色を更新する双方向バインディングを実装する。

### 対象ファイル候補
- `src/app/panels/ColorWheelWidget.cpp`
- `src/app/panels/ColorWheelWidget.h`

### 成功条件
- `cmake --build` でエラー 0 件
- カラーホイールで色を選ぶと QLineEdit に `#RRGGBB` 形式で反映される
- QLineEdit に有効な hex 値を入力して Enter を押すとカラーホイールが更新される
- 無効な hex 入力は無視される（クラッシュしない）

### 時間見積もり
45〜60 分

### 禁止事項
- カラーホイールの描画ロジック変更
- 新規カラーモデルクラスの追加

### 検証方法
```powershell
cmake --build build --config Release
```

---
