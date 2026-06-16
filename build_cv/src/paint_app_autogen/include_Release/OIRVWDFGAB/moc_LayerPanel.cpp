/****************************************************************************
** Meta object code from reading C++ file 'LayerPanel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/app/panels/LayerPanel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LayerPanel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.7.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSappSCOPEpanelsSCOPELayerPanelENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSappSCOPEpanelsSCOPELayerPanelENDCLASS = QtMocHelpers::stringData(
    "app::panels::LayerPanel",
    "refreshLayers",
    "",
    "onAddRasterLayerClicked",
    "onAddVectorLayerClicked",
    "onAddFolderLayerClicked",
    "onDuplicateLayerClicked",
    "onDeleteLayerClicked",
    "onMoveLayerUpClicked",
    "onMoveLayerDownClicked",
    "onToggleClipClicked",
    "onToggleMaskClicked",
    "onRemoveMaskClicked",
    "onToggleLockClicked",
    "onToggleAlphaLockClicked",
    "onTogglePositionLockClicked",
    "onCurrentLayerChanged",
    "row",
    "onLayerItemChanged",
    "QListWidgetItem*",
    "item",
    "onLayerRowsMoved",
    "QModelIndex",
    "parent",
    "start",
    "end",
    "destination",
    "onOpacityChanged",
    "value",
    "onBlendModeChanged",
    "index",
    "onFilterTextChanged",
    "text",
    "onLayerContextMenuRequested",
    "pos"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSappSCOPEpanelsSCOPELayerPanelENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  140,    2, 0x08,    1 /* Private */,
       3,    0,  141,    2, 0x08,    2 /* Private */,
       4,    0,  142,    2, 0x08,    3 /* Private */,
       5,    0,  143,    2, 0x08,    4 /* Private */,
       6,    0,  144,    2, 0x08,    5 /* Private */,
       7,    0,  145,    2, 0x08,    6 /* Private */,
       8,    0,  146,    2, 0x08,    7 /* Private */,
       9,    0,  147,    2, 0x08,    8 /* Private */,
      10,    0,  148,    2, 0x08,    9 /* Private */,
      11,    0,  149,    2, 0x08,   10 /* Private */,
      12,    0,  150,    2, 0x08,   11 /* Private */,
      13,    0,  151,    2, 0x08,   12 /* Private */,
      14,    0,  152,    2, 0x08,   13 /* Private */,
      15,    0,  153,    2, 0x08,   14 /* Private */,
      16,    1,  154,    2, 0x08,   15 /* Private */,
      18,    1,  157,    2, 0x08,   17 /* Private */,
      21,    5,  160,    2, 0x08,   19 /* Private */,
      27,    1,  171,    2, 0x08,   25 /* Private */,
      29,    1,  174,    2, 0x08,   27 /* Private */,
      31,    1,  177,    2, 0x08,   29 /* Private */,
      33,    1,  180,    2, 0x08,   31 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void, 0x80000000 | 19,   20,
    QMetaType::Void, 0x80000000 | 22, QMetaType::Int, QMetaType::Int, 0x80000000 | 22, QMetaType::Int,   23,   24,   25,   26,   17,
    QMetaType::Void, QMetaType::Int,   28,
    QMetaType::Void, QMetaType::Int,   30,
    QMetaType::Void, QMetaType::QString,   32,
    QMetaType::Void, QMetaType::QPoint,   34,

       0        // eod
};

Q_CONSTINIT const QMetaObject app::panels::LayerPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSappSCOPEpanelsSCOPELayerPanelENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSappSCOPEpanelsSCOPELayerPanelENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSappSCOPEpanelsSCOPELayerPanelENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LayerPanel, std::true_type>,
        // method 'refreshLayers'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAddRasterLayerClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAddVectorLayerClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAddFolderLayerClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDuplicateLayerClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDeleteLayerClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onMoveLayerUpClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onMoveLayerDownClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onToggleClipClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onToggleMaskClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRemoveMaskClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onToggleLockClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onToggleAlphaLockClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTogglePositionLockClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCurrentLayerChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onLayerItemChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QListWidgetItem *, std::false_type>,
        // method 'onLayerRowsMoved'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onOpacityChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onBlendModeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onFilterTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onLayerContextMenuRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>
    >,
    nullptr
} };

void app::panels::LayerPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LayerPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->refreshLayers(); break;
        case 1: _t->onAddRasterLayerClicked(); break;
        case 2: _t->onAddVectorLayerClicked(); break;
        case 3: _t->onAddFolderLayerClicked(); break;
        case 4: _t->onDuplicateLayerClicked(); break;
        case 5: _t->onDeleteLayerClicked(); break;
        case 6: _t->onMoveLayerUpClicked(); break;
        case 7: _t->onMoveLayerDownClicked(); break;
        case 8: _t->onToggleClipClicked(); break;
        case 9: _t->onToggleMaskClicked(); break;
        case 10: _t->onRemoveMaskClicked(); break;
        case 11: _t->onToggleLockClicked(); break;
        case 12: _t->onToggleAlphaLockClicked(); break;
        case 13: _t->onTogglePositionLockClicked(); break;
        case 14: _t->onCurrentLayerChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 15: _t->onLayerItemChanged((*reinterpret_cast< std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 16: _t->onLayerRowsMoved((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[5]))); break;
        case 17: _t->onOpacityChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 18: _t->onBlendModeChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 19: _t->onFilterTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->onLayerContextMenuRequested((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *app::panels::LayerPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *app::panels::LayerPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSappSCOPEpanelsSCOPELayerPanelENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int app::panels::LayerPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 21;
    }
    return _id;
}
QT_WARNING_POP
