/****************************************************************************
** Meta object code from reading C++ file 'CursorProvider.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/providers/cursor/CursorProvider.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CursorProvider.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSCursorProviderENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSCursorProviderENDCLASS = QtMocHelpers::stringData(
    "CursorProvider"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSCursorProviderENDCLASS_t {
    uint offsetsAndSizes[2];
    char stringdata0[15];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSCursorProviderENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSCursorProviderENDCLASS_t qt_meta_stringdata_CLASSCursorProviderENDCLASS = {
    {
        QT_MOC_LITERAL(0, 14)   // "CursorProvider"
    },
    "CursorProvider"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSCursorProviderENDCLASS[] = {

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

Q_CONSTINIT const QMetaObject CursorProvider::staticMetaObject = { {
    QMetaObject::SuperData::link<IProvider::staticMetaObject>(),
    qt_meta_stringdata_CLASSCursorProviderENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSCursorProviderENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSCursorProviderENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CursorProvider, std::true_type>
    >,
    nullptr
} };

void CursorProvider::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *CursorProvider::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CursorProvider::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSCursorProviderENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return IProvider::qt_metacast(_clname);
}

int CursorProvider::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IProvider::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSCursorWebStrategyENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSCursorWebStrategyENDCLASS = QtMocHelpers::stringData(
    "CursorWebStrategy"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSCursorWebStrategyENDCLASS_t {
    uint offsetsAndSizes[2];
    char stringdata0[18];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSCursorWebStrategyENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSCursorWebStrategyENDCLASS_t qt_meta_stringdata_CLASSCursorWebStrategyENDCLASS = {
    {
        QT_MOC_LITERAL(0, 17)   // "CursorWebStrategy"
    },
    "CursorWebStrategy"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSCursorWebStrategyENDCLASS[] = {

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

Q_CONSTINIT const QMetaObject CursorWebStrategy::staticMetaObject = { {
    QMetaObject::SuperData::link<IFetchStrategy::staticMetaObject>(),
    qt_meta_stringdata_CLASSCursorWebStrategyENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSCursorWebStrategyENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSCursorWebStrategyENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CursorWebStrategy, std::true_type>
    >,
    nullptr
} };

void CursorWebStrategy::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *CursorWebStrategy::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CursorWebStrategy::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSCursorWebStrategyENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return IFetchStrategy::qt_metacast(_clname);
}

int CursorWebStrategy::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IFetchStrategy::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
