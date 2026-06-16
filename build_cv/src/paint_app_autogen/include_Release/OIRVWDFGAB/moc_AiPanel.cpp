/****************************************************************************
** Meta object code from reading C++ file 'AiPanel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/app/panels/AiPanel.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AiPanel.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSappSCOPEpanelsSCOPEAiPanelENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSappSCOPEpanelsSCOPEAiPanelENDCLASS = QtMocHelpers::stringData(
    "app::panels::AiPanel",
    "onConnectClicked",
    "",
    "onGenerateClicked",
    "onInpaintClicked",
    "onCancelClicked",
    "onBrowseWorkflow",
    "onWorkflowSelected",
    "index",
    "onApplyCandidate",
    "onComfyStateChanged",
    "connected",
    "onModelsLoaded",
    "models",
    "onProgressUpdate",
    "step",
    "total",
    "nodeId",
    "onPreviewReceived",
    "px",
    "onGenerationComplete",
    "opType",
    "onBatchCandidatesReady",
    "QList<QPixmap>",
    "candidates",
    "onGenerationError",
    "message",
    "onSelectionMissing",
    "onElapsedTick"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSappSCOPEpanelsSCOPEAiPanelENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  110,    2, 0x08,    1 /* Private */,
       3,    0,  111,    2, 0x08,    2 /* Private */,
       4,    0,  112,    2, 0x08,    3 /* Private */,
       5,    0,  113,    2, 0x08,    4 /* Private */,
       6,    0,  114,    2, 0x08,    5 /* Private */,
       7,    1,  115,    2, 0x08,    6 /* Private */,
       9,    0,  118,    2, 0x08,    8 /* Private */,
      10,    1,  119,    2, 0x08,    9 /* Private */,
      12,    1,  122,    2, 0x08,   11 /* Private */,
      14,    3,  125,    2, 0x08,   13 /* Private */,
      18,    1,  132,    2, 0x08,   17 /* Private */,
      20,    1,  135,    2, 0x08,   19 /* Private */,
      22,    1,  138,    2, 0x08,   21 /* Private */,
      25,    1,  141,    2, 0x08,   23 /* Private */,
      27,    0,  144,    2, 0x08,   25 /* Private */,
      28,    0,  145,    2, 0x08,   26 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   11,
    QMetaType::Void, QMetaType::QStringList,   13,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,   15,   16,   17,
    QMetaType::Void, QMetaType::QPixmap,   19,
    QMetaType::Void, QMetaType::QString,   21,
    QMetaType::Void, 0x80000000 | 23,   24,
    QMetaType::Void, QMetaType::QString,   26,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject app::panels::AiPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSappSCOPEpanelsSCOPEAiPanelENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSappSCOPEpanelsSCOPEAiPanelENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSappSCOPEpanelsSCOPEAiPanelENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AiPanel, std::true_type>,
        // method 'onConnectClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGenerateClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onInpaintClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCancelClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onBrowseWorkflow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onWorkflowSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onApplyCandidate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onComfyStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onModelsLoaded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QStringList &, std::false_type>,
        // method 'onProgressUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onPreviewReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPixmap &, std::false_type>,
        // method 'onGenerationComplete'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onBatchCandidatesReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<QPixmap> &, std::false_type>,
        // method 'onGenerationError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onSelectionMissing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onElapsedTick'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void app::panels::AiPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AiPanel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onConnectClicked(); break;
        case 1: _t->onGenerateClicked(); break;
        case 2: _t->onInpaintClicked(); break;
        case 3: _t->onCancelClicked(); break;
        case 4: _t->onBrowseWorkflow(); break;
        case 5: _t->onWorkflowSelected((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 6: _t->onApplyCandidate(); break;
        case 7: _t->onComfyStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 8: _t->onModelsLoaded((*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 9: _t->onProgressUpdate((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 10: _t->onPreviewReceived((*reinterpret_cast< std::add_pointer_t<QPixmap>>(_a[1]))); break;
        case 11: _t->onGenerationComplete((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->onBatchCandidatesReady((*reinterpret_cast< std::add_pointer_t<QList<QPixmap>>>(_a[1]))); break;
        case 13: _t->onGenerationError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->onSelectionMissing(); break;
        case 15: _t->onElapsedTick(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 12:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<QPixmap> >(); break;
            }
            break;
        }
    }
}

const QMetaObject *app::panels::AiPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *app::panels::AiPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSappSCOPEpanelsSCOPEAiPanelENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int app::panels::AiPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}
QT_WARNING_POP
