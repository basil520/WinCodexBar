/****************************************************************************
** Meta object code from reading C++ file 'ProviderPipeline.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/providers/ProviderPipeline.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ProviderPipeline.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSProviderPipelineENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSProviderPipelineENDCLASS = QtMocHelpers::stringData(
    "ProviderPipeline",
    "strategyStarted",
    "",
    "strategyID",
    "strategyFailed",
    "error",
    "pipelineComplete",
    "ProviderFetchResult",
    "result"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSProviderPipelineENDCLASS_t {
    uint offsetsAndSizes[18];
    char stringdata0[17];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[15];
    char stringdata5[6];
    char stringdata6[17];
    char stringdata7[20];
    char stringdata8[7];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSProviderPipelineENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSProviderPipelineENDCLASS_t qt_meta_stringdata_CLASSProviderPipelineENDCLASS = {
    {
        QT_MOC_LITERAL(0, 16),  // "ProviderPipeline"
        QT_MOC_LITERAL(17, 15),  // "strategyStarted"
        QT_MOC_LITERAL(33, 0),  // ""
        QT_MOC_LITERAL(34, 10),  // "strategyID"
        QT_MOC_LITERAL(45, 14),  // "strategyFailed"
        QT_MOC_LITERAL(60, 5),  // "error"
        QT_MOC_LITERAL(66, 16),  // "pipelineComplete"
        QT_MOC_LITERAL(83, 19),  // "ProviderFetchResult"
        QT_MOC_LITERAL(103, 6)   // "result"
    },
    "ProviderPipeline",
    "strategyStarted",
    "",
    "strategyID",
    "strategyFailed",
    "error",
    "pipelineComplete",
    "ProviderFetchResult",
    "result"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSProviderPipelineENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   32,    2, 0x06,    1 /* Public */,
       4,    2,   35,    2, 0x06,    3 /* Public */,
       6,    1,   40,    2, 0x06,    6 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    5,
    QMetaType::Void, 0x80000000 | 7,    8,

       0        // eod
};

Q_CONSTINIT const QMetaObject ProviderPipeline::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSProviderPipelineENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSProviderPipelineENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSProviderPipelineENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ProviderPipeline, std::true_type>,
        // method 'strategyStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'strategyFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'pipelineComplete'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const ProviderFetchResult &, std::false_type>
    >,
    nullptr
} };

void ProviderPipeline::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ProviderPipeline *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->strategyStarted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->strategyFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->pipelineComplete((*reinterpret_cast< std::add_pointer_t<ProviderFetchResult>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ProviderPipeline::*)(const QString & );
            if (_t _q_method = &ProviderPipeline::strategyStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ProviderPipeline::*)(const QString & , const QString & );
            if (_t _q_method = &ProviderPipeline::strategyFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ProviderPipeline::*)(const ProviderFetchResult & );
            if (_t _q_method = &ProviderPipeline::pipelineComplete; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *ProviderPipeline::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ProviderPipeline::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSProviderPipelineENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ProviderPipeline::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void ProviderPipeline::strategyStarted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ProviderPipeline::strategyFailed(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ProviderPipeline::pipelineComplete(const ProviderFetchResult & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
