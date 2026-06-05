/****************************************************************************
** Meta object code from reading C++ file 'AppController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/app/bridge/AppController.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AppController.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSappSCOPEbridgeSCOPEAppControllerENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSappSCOPEbridgeSCOPEAppControllerENDCLASS = QtMocHelpers::stringData(
    "app::bridge::AppController",
    "canvasChanged",
    "",
    "documentChanged",
    "layersChanged",
    "toolStateChanged",
    "foregroundColorUsed",
    "overlayChanged",
    "comfyUiStateChanged",
    "connected",
    "aiSelectionRefined",
    "aiModelsLoaded",
    "models",
    "aiProgressUpdate",
    "step",
    "totalSteps",
    "nodeId",
    "aiPreviewReceived",
    "preview",
    "aiGenerationComplete",
    "operationType",
    "aiBatchCandidatesReady",
    "QList<QPixmap>",
    "candidates",
    "aiGenerationError",
    "message",
    "selectionMissing",
    "freeTransformStateChanged",
    "active"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSappSCOPEbridgeSCOPEAppControllerENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      16,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  110,    2, 0x06,    1 /* Public */,
       3,    0,  111,    2, 0x06,    2 /* Public */,
       4,    0,  112,    2, 0x06,    3 /* Public */,
       5,    0,  113,    2, 0x06,    4 /* Public */,
       6,    0,  114,    2, 0x06,    5 /* Public */,
       7,    0,  115,    2, 0x06,    6 /* Public */,
       8,    1,  116,    2, 0x06,    7 /* Public */,
      10,    0,  119,    2, 0x06,    9 /* Public */,
      11,    1,  120,    2, 0x06,   10 /* Public */,
      13,    3,  123,    2, 0x06,   12 /* Public */,
      17,    1,  130,    2, 0x06,   16 /* Public */,
      19,    1,  133,    2, 0x06,   18 /* Public */,
      21,    1,  136,    2, 0x06,   20 /* Public */,
      24,    1,  139,    2, 0x06,   22 /* Public */,
      26,    0,  142,    2, 0x06,   24 /* Public */,
      27,    1,  143,    2, 0x06,   25 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QStringList,   12,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,   14,   15,   16,
    QMetaType::Void, QMetaType::QPixmap,   18,
    QMetaType::Void, QMetaType::QString,   20,
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   28,

       0        // eod
};

Q_CONSTINIT const QMetaObject app::bridge::AppController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSappSCOPEbridgeSCOPEAppControllerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSappSCOPEbridgeSCOPEAppControllerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSappSCOPEbridgeSCOPEAppControllerENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AppController, std::true_type>,
        // method 'canvasChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'documentChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'layersChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toolStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'foregroundColorUsed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'overlayChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'comfyUiStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'aiSelectionRefined'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'aiModelsLoaded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QStringList, std::false_type>,
        // method 'aiProgressUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'aiPreviewReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QPixmap, std::false_type>,
        // method 'aiGenerationComplete'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'aiBatchCandidatesReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QList<QPixmap>, std::false_type>,
        // method 'aiGenerationError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'selectionMissing'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'freeTransformStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void app::bridge::AppController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AppController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->canvasChanged(); break;
        case 1: _t->documentChanged(); break;
        case 2: _t->layersChanged(); break;
        case 3: _t->toolStateChanged(); break;
        case 4: _t->foregroundColorUsed(); break;
        case 5: _t->overlayChanged(); break;
        case 6: _t->comfyUiStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 7: _t->aiSelectionRefined(); break;
        case 8: _t->aiModelsLoaded((*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 9: _t->aiProgressUpdate((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 10: _t->aiPreviewReceived((*reinterpret_cast< std::add_pointer_t<QPixmap>>(_a[1]))); break;
        case 11: _t->aiGenerationComplete((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->aiBatchCandidatesReady((*reinterpret_cast< std::add_pointer_t<QList<QPixmap>>>(_a[1]))); break;
        case 13: _t->aiGenerationError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->selectionMissing(); break;
        case 15: _t->freeTransformStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
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
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AppController::*)();
            if (_t _q_method = &AppController::canvasChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AppController::*)();
            if (_t _q_method = &AppController::documentChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AppController::*)();
            if (_t _q_method = &AppController::layersChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AppController::*)();
            if (_t _q_method = &AppController::toolStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (AppController::*)();
            if (_t _q_method = &AppController::foregroundColorUsed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (AppController::*)();
            if (_t _q_method = &AppController::overlayChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (AppController::*)(bool );
            if (_t _q_method = &AppController::comfyUiStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (AppController::*)();
            if (_t _q_method = &AppController::aiSelectionRefined; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (AppController::*)(QStringList );
            if (_t _q_method = &AppController::aiModelsLoaded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (AppController::*)(int , int , QString );
            if (_t _q_method = &AppController::aiProgressUpdate; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (AppController::*)(QPixmap );
            if (_t _q_method = &AppController::aiPreviewReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (AppController::*)(QString );
            if (_t _q_method = &AppController::aiGenerationComplete; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (AppController::*)(QList<QPixmap> );
            if (_t _q_method = &AppController::aiBatchCandidatesReady; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (AppController::*)(QString );
            if (_t _q_method = &AppController::aiGenerationError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (AppController::*)();
            if (_t _q_method = &AppController::selectionMissing; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (AppController::*)(bool );
            if (_t _q_method = &AppController::freeTransformStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
    }
}

const QMetaObject *app::bridge::AppController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *app::bridge::AppController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSappSCOPEbridgeSCOPEAppControllerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int app::bridge::AppController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void app::bridge::AppController::canvasChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void app::bridge::AppController::documentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void app::bridge::AppController::layersChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void app::bridge::AppController::toolStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void app::bridge::AppController::foregroundColorUsed()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void app::bridge::AppController::overlayChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void app::bridge::AppController::comfyUiStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void app::bridge::AppController::aiSelectionRefined()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void app::bridge::AppController::aiModelsLoaded(QStringList _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void app::bridge::AppController::aiProgressUpdate(int _t1, int _t2, QString _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void app::bridge::AppController::aiPreviewReceived(QPixmap _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void app::bridge::AppController::aiGenerationComplete(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void app::bridge::AppController::aiBatchCandidatesReady(QList<QPixmap> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void app::bridge::AppController::aiGenerationError(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void app::bridge::AppController::selectionMissing()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void app::bridge::AppController::freeTransformStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}
QT_WARNING_POP
