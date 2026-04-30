/****************************************************************************
** Meta object code from reading C++ file 'MistralProvider.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/providers/mistral/MistralProvider.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MistralProvider.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSMistralProviderENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSMistralProviderENDCLASS = QtMocHelpers::stringData(
    "MistralProvider"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSMistralProviderENDCLASS_t {
    uint offsetsAndSizes[2];
    char stringdata0[16];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSMistralProviderENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSMistralProviderENDCLASS_t qt_meta_stringdata_CLASSMistralProviderENDCLASS = {
    {
        QT_MOC_LITERAL(0, 15)   // "MistralProvider"
    },
    "MistralProvider"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSMistralProviderENDCLASS[] = {

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

Q_CONSTINIT const QMetaObject MistralProvider::staticMetaObject = { {
    QMetaObject::SuperData::link<IProvider::staticMetaObject>(),
    qt_meta_stringdata_CLASSMistralProviderENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSMistralProviderENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSMistralProviderENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MistralProvider, std::true_type>
    >,
    nullptr
} };

void MistralProvider::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *MistralProvider::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MistralProvider::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSMistralProviderENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return IProvider::qt_metacast(_clname);
}

int MistralProvider::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IProvider::qt_metacall(_c, _id, _a);
    return _id;
}
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSMistralWebStrategyENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSMistralWebStrategyENDCLASS = QtMocHelpers::stringData(
    "MistralWebStrategy"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSMistralWebStrategyENDCLASS_t {
    uint offsetsAndSizes[2];
    char stringdata0[19];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSMistralWebStrategyENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSMistralWebStrategyENDCLASS_t qt_meta_stringdata_CLASSMistralWebStrategyENDCLASS = {
    {
        QT_MOC_LITERAL(0, 18)   // "MistralWebStrategy"
    },
    "MistralWebStrategy"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSMistralWebStrategyENDCLASS[] = {

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

Q_CONSTINIT const QMetaObject MistralWebStrategy::staticMetaObject = { {
    QMetaObject::SuperData::link<IFetchStrategy::staticMetaObject>(),
    qt_meta_stringdata_CLASSMistralWebStrategyENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSMistralWebStrategyENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSMistralWebStrategyENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MistralWebStrategy, std::true_type>
    >,
    nullptr
} };

void MistralWebStrategy::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *MistralWebStrategy::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MistralWebStrategy::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSMistralWebStrategyENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return IFetchStrategy::qt_metacast(_clname);
}

int MistralWebStrategy::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IFetchStrategy::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
