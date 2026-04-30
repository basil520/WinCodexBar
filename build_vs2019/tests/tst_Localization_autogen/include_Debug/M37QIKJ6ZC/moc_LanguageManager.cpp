/****************************************************************************
** Meta object code from reading C++ file 'LanguageManager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/app/LanguageManager.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LanguageManager.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSLanguageManagerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSLanguageManagerENDCLASS = QtMocHelpers::stringData(
    "LanguageManager",
    "languageChanged",
    "",
    "translationRevisionChanged",
    "retranslate",
    "setLanguage",
    "code",
    "language",
    "translationRevision"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSLanguageManagerENDCLASS_t {
    uint offsetsAndSizes[18];
    char stringdata0[16];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[27];
    char stringdata4[12];
    char stringdata5[12];
    char stringdata6[5];
    char stringdata7[9];
    char stringdata8[20];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSLanguageManagerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSLanguageManagerENDCLASS_t qt_meta_stringdata_CLASSLanguageManagerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 15),  // "LanguageManager"
        QT_MOC_LITERAL(16, 15),  // "languageChanged"
        QT_MOC_LITERAL(32, 0),  // ""
        QT_MOC_LITERAL(33, 26),  // "translationRevisionChanged"
        QT_MOC_LITERAL(60, 11),  // "retranslate"
        QT_MOC_LITERAL(72, 11),  // "setLanguage"
        QT_MOC_LITERAL(84, 4),  // "code"
        QT_MOC_LITERAL(89, 8),  // "language"
        QT_MOC_LITERAL(98, 19)   // "translationRevision"
    },
    "LanguageManager",
    "languageChanged",
    "",
    "translationRevisionChanged",
    "retranslate",
    "setLanguage",
    "code",
    "language",
    "translationRevision"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSLanguageManagerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       2,   44, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    3 /* Public */,
       3,    0,   39,    2, 0x06,    4 /* Public */,
       4,    0,   40,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    1,   41,    2, 0x0a,    6 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    6,

 // properties: name, type, flags
       7, QMetaType::QString, 0x00015103, uint(0), 0,
       8, QMetaType::Int, 0x00015001, uint(1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject LanguageManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSLanguageManagerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSLanguageManagerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSLanguageManagerENDCLASS_t,
        // property 'language'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'translationRevision'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LanguageManager, std::true_type>,
        // method 'languageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'translationRevisionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'retranslate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setLanguage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void LanguageManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LanguageManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->languageChanged(); break;
        case 1: _t->translationRevisionChanged(); break;
        case 2: _t->retranslate(); break;
        case 3: _t->setLanguage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (LanguageManager::*)();
            if (_t _q_method = &LanguageManager::languageChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (LanguageManager::*)();
            if (_t _q_method = &LanguageManager::translationRevisionChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (LanguageManager::*)();
            if (_t _q_method = &LanguageManager::retranslate; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<LanguageManager *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->language(); break;
        case 1: *reinterpret_cast< int*>(_v) = _t->translationRevision(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<LanguageManager *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setLanguage(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *LanguageManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LanguageManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSLanguageManagerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int LanguageManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void LanguageManager::languageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void LanguageManager::translationRevisionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void LanguageManager::retranslate()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
