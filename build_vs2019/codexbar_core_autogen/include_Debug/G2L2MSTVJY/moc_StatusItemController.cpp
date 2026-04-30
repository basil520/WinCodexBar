/****************************************************************************
** Meta object code from reading C++ file 'StatusItemController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/tray/StatusItemController.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StatusItemController.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.5.2. It"
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
struct qt_meta_stringdata_CLASSStatusItemControllerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSStatusItemControllerENDCLASS = QtMocHelpers::stringData(
    "StatusItemController",
    "trayPanelRequested",
    "",
    "settingsRequested",
    "aboutRequested",
    "showTrayPanel",
    "hideTrayPanel",
    "toggleTrayPanel",
    "showBalloon",
    "title",
    "message",
    "onSnapshotChanged",
    "providerId",
    "rebuildProviderMenu",
    "onProviderSelected"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSStatusItemControllerENDCLASS_t {
    uint offsetsAndSizes[30];
    char stringdata0[21];
    char stringdata1[19];
    char stringdata2[1];
    char stringdata3[18];
    char stringdata4[15];
    char stringdata5[14];
    char stringdata6[14];
    char stringdata7[16];
    char stringdata8[12];
    char stringdata9[6];
    char stringdata10[8];
    char stringdata11[18];
    char stringdata12[11];
    char stringdata13[20];
    char stringdata14[19];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSStatusItemControllerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSStatusItemControllerENDCLASS_t qt_meta_stringdata_CLASSStatusItemControllerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 20),  // "StatusItemController"
        QT_MOC_LITERAL(21, 18),  // "trayPanelRequested"
        QT_MOC_LITERAL(40, 0),  // ""
        QT_MOC_LITERAL(41, 17),  // "settingsRequested"
        QT_MOC_LITERAL(59, 14),  // "aboutRequested"
        QT_MOC_LITERAL(74, 13),  // "showTrayPanel"
        QT_MOC_LITERAL(88, 13),  // "hideTrayPanel"
        QT_MOC_LITERAL(102, 15),  // "toggleTrayPanel"
        QT_MOC_LITERAL(118, 11),  // "showBalloon"
        QT_MOC_LITERAL(130, 5),  // "title"
        QT_MOC_LITERAL(136, 7),  // "message"
        QT_MOC_LITERAL(144, 17),  // "onSnapshotChanged"
        QT_MOC_LITERAL(162, 10),  // "providerId"
        QT_MOC_LITERAL(173, 19),  // "rebuildProviderMenu"
        QT_MOC_LITERAL(193, 18)   // "onProviderSelected"
    },
    "StatusItemController",
    "trayPanelRequested",
    "",
    "settingsRequested",
    "aboutRequested",
    "showTrayPanel",
    "hideTrayPanel",
    "toggleTrayPanel",
    "showBalloon",
    "title",
    "message",
    "onSnapshotChanged",
    "providerId",
    "rebuildProviderMenu",
    "onProviderSelected"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSStatusItemControllerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   74,    2, 0x06,    1 /* Public */,
       3,    0,   75,    2, 0x06,    2 /* Public */,
       4,    0,   76,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    0,   77,    2, 0x0a,    4 /* Public */,
       6,    0,   78,    2, 0x0a,    5 /* Public */,
       7,    0,   79,    2, 0x0a,    6 /* Public */,
       8,    2,   80,    2, 0x0a,    7 /* Public */,
      11,    1,   85,    2, 0x08,   10 /* Private */,
      13,    0,   88,    2, 0x08,   12 /* Private */,
      14,    0,   89,    2, 0x08,   13 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    9,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject StatusItemController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSStatusItemControllerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSStatusItemControllerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSStatusItemControllerENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<StatusItemController, std::true_type>,
        // method 'trayPanelRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'settingsRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'aboutRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showTrayPanel'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'hideTrayPanel'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleTrayPanel'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showBalloon'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onSnapshotChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'rebuildProviderMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProviderSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void StatusItemController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StatusItemController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->trayPanelRequested(); break;
        case 1: _t->settingsRequested(); break;
        case 2: _t->aboutRequested(); break;
        case 3: _t->showTrayPanel(); break;
        case 4: _t->hideTrayPanel(); break;
        case 5: _t->toggleTrayPanel(); break;
        case 6: _t->showBalloon((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->onSnapshotChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->rebuildProviderMenu(); break;
        case 9: _t->onProviderSelected(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StatusItemController::*)();
            if (_t _q_method = &StatusItemController::trayPanelRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StatusItemController::*)();
            if (_t _q_method = &StatusItemController::settingsRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (StatusItemController::*)();
            if (_t _q_method = &StatusItemController::aboutRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *StatusItemController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StatusItemController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSStatusItemControllerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int StatusItemController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void StatusItemController::trayPanelRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void StatusItemController::settingsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void StatusItemController::aboutRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
