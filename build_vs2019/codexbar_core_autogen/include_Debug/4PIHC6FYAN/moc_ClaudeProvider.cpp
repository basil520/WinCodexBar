/****************************************************************************
** Meta object code from reading C++ file 'ClaudeProvider.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/providers/claude/ClaudeProvider.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClaudeProvider.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSClaudeProviderENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSClaudeProviderENDCLASS = QtMocHelpers::stringData(
    "ClaudeProvider"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSClaudeProviderENDCLASS_t {
    uint offsetsAndSizes[2];
    char stringdata0[15];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSClaudeProviderENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSClaudeProviderENDCLASS_t qt_meta_stringdata_CLASSClaudeProviderENDCLASS = {
    {
        QT_MOC_LITERAL(0, 14)   // "ClaudeProvider"
    },
    "ClaudeProvider"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSClaudeProviderENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject ClaudeProvider::staticMetaObject = { {
    QMetaObject::SuperData::link<IProvider::staticMetaObject>(),
    qt_meta_stringdata_CLASSClaudeProviderENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSClaudeProviderENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSClaudeProviderENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ClaudeProvider, std::true_type>
    >,
    nullptr
} };

void ClaudeProvider::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *ClaudeProvider::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ClaudeProvider::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSClaudeProviderENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return IProvider::qt_metacast(_clname);
}

int ClaudeProvider::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IProvider::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSClaudeOAuthStrategyENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSClaudeOAuthStrategyENDCLASS = QtMocHelpers::stringData(
    "ClaudeOAuthStrategy"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSClaudeOAuthStrategyENDCLASS_t {
    uint offsetsAndSizes[2];
    char stringdata0[20];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSClaudeOAuthStrategyENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSClaudeOAuthStrategyENDCLASS_t qt_meta_stringdata_CLASSClaudeOAuthStrategyENDCLASS = {
    {
        QT_MOC_LITERAL(0, 19)   // "ClaudeOAuthStrategy"
    },
    "ClaudeOAuthStrategy"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSClaudeOAuthStrategyENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject ClaudeOAuthStrategy::staticMetaObject = { {
    QMetaObject::SuperData::link<IFetchStrategy::staticMetaObject>(),
    qt_meta_stringdata_CLASSClaudeOAuthStrategyENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSClaudeOAuthStrategyENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSClaudeOAuthStrategyENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ClaudeOAuthStrategy, std::true_type>
    >,
    nullptr
} };

void ClaudeOAuthStrategy::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *ClaudeOAuthStrategy::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ClaudeOAuthStrategy::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSClaudeOAuthStrategyENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return IFetchStrategy::qt_metacast(_clname);
}

int ClaudeOAuthStrategy::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IFetchStrategy::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSClaudeWebStrategyENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSClaudeWebStrategyENDCLASS = QtMocHelpers::stringData(
    "ClaudeWebStrategy"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSClaudeWebStrategyENDCLASS_t {
    uint offsetsAndSizes[2];
    char stringdata0[18];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSClaudeWebStrategyENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSClaudeWebStrategyENDCLASS_t qt_meta_stringdata_CLASSClaudeWebStrategyENDCLASS = {
    {
        QT_MOC_LITERAL(0, 17)   // "ClaudeWebStrategy"
    },
    "ClaudeWebStrategy"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSClaudeWebStrategyENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject ClaudeWebStrategy::staticMetaObject = { {
    QMetaObject::SuperData::link<IFetchStrategy::staticMetaObject>(),
    qt_meta_stringdata_CLASSClaudeWebStrategyENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSClaudeWebStrategyENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSClaudeWebStrategyENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ClaudeWebStrategy, std::true_type>
    >,
    nullptr
} };

void ClaudeWebStrategy::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *ClaudeWebStrategy::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ClaudeWebStrategy::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSClaudeWebStrategyENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return IFetchStrategy::qt_metacast(_clname);
}

int ClaudeWebStrategy::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IFetchStrategy::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
