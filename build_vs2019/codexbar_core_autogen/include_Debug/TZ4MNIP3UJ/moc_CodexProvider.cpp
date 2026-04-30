/****************************************************************************
** Meta object code from reading C++ file 'CodexProvider.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/providers/codex/CodexProvider.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CodexProvider.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSCodexProviderENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSCodexProviderENDCLASS = QtMocHelpers::stringData(
    "CodexProvider"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSCodexProviderENDCLASS_t {
    uint offsetsAndSizes[2];
    char stringdata0[14];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSCodexProviderENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSCodexProviderENDCLASS_t qt_meta_stringdata_CLASSCodexProviderENDCLASS = {
    {
        QT_MOC_LITERAL(0, 13)   // "CodexProvider"
    },
    "CodexProvider"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSCodexProviderENDCLASS[] = {

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

Q_CONSTINIT const QMetaObject CodexProvider::staticMetaObject = { {
    QMetaObject::SuperData::link<IProvider::staticMetaObject>(),
    qt_meta_stringdata_CLASSCodexProviderENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSCodexProviderENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSCodexProviderENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CodexProvider, std::true_type>
    >,
    nullptr
} };

void CodexProvider::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *CodexProvider::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CodexProvider::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSCodexProviderENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return IProvider::qt_metacast(_clname);
}

int CodexProvider::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IProvider::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSCodexOAuthStrategyENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSCodexOAuthStrategyENDCLASS = QtMocHelpers::stringData(
    "CodexOAuthStrategy"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSCodexOAuthStrategyENDCLASS_t {
    uint offsetsAndSizes[2];
    char stringdata0[19];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSCodexOAuthStrategyENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSCodexOAuthStrategyENDCLASS_t qt_meta_stringdata_CLASSCodexOAuthStrategyENDCLASS = {
    {
        QT_MOC_LITERAL(0, 18)   // "CodexOAuthStrategy"
    },
    "CodexOAuthStrategy"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSCodexOAuthStrategyENDCLASS[] = {

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

Q_CONSTINIT const QMetaObject CodexOAuthStrategy::staticMetaObject = { {
    QMetaObject::SuperData::link<IFetchStrategy::staticMetaObject>(),
    qt_meta_stringdata_CLASSCodexOAuthStrategyENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSCodexOAuthStrategyENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSCodexOAuthStrategyENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CodexOAuthStrategy, std::true_type>
    >,
    nullptr
} };

void CodexOAuthStrategy::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *CodexOAuthStrategy::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CodexOAuthStrategy::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSCodexOAuthStrategyENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return IFetchStrategy::qt_metacast(_clname);
}

int CodexOAuthStrategy::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IFetchStrategy::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
