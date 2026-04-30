/****************************************************************************
** Meta object code from reading C++ file 'UsageStore.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/app/UsageStore.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'UsageStore.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSUsageStoreENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSUsageStoreENDCLASS = QtMocHelpers::stringData(
    "UsageStore",
    "snapshotChanged",
    "",
    "providerId",
    "refreshingChanged",
    "providerIDsChanged",
    "errorOccurred",
    "message",
    "costUsageEnabledChanged",
    "costUsageRefreshingChanged",
    "costUsageChanged",
    "snapshotRevisionChanged",
    "providerConnectionTestChanged",
    "providerLoginStateChanged",
    "providerStatusChanged",
    "statusRevisionChanged",
    "snapshot",
    "UsageSnapshot",
    "refresh",
    "refreshAll",
    "clearCache",
    "refreshProvider",
    "setProviderEnabled",
    "id",
    "enabled",
    "isProviderEnabled",
    "providerDisplayName",
    "snapshotData",
    "providerError",
    "providerList",
    "moveProvider",
    "fromIndex",
    "toIndex",
    "providerSettingsFields",
    "providerDescriptorData",
    "setProviderSetting",
    "key",
    "value",
    "providerSecretStatus",
    "setProviderSecret",
    "clearProviderSecret",
    "testProviderConnection",
    "providerConnectionTest",
    "startProviderLogin",
    "cancelProviderLogin",
    "providerLoginState",
    "refreshProviderStatuses",
    "providerStatus",
    "providerUsageSnapshot",
    "allProviderIDs",
    "refreshCostUsage",
    "costUsageData",
    "providerCostUsageList",
    "providerCostUsageData",
    "utilizationChartData",
    "seriesName",
    "isRefreshing",
    "providerIDs",
    "costUsageEnabled",
    "costUsageRefreshing",
    "snapshotRevision",
    "statusRevision"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSUsageStoreENDCLASS_t {
    uint offsetsAndSizes[124];
    char stringdata0[11];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[18];
    char stringdata5[19];
    char stringdata6[14];
    char stringdata7[8];
    char stringdata8[24];
    char stringdata9[27];
    char stringdata10[17];
    char stringdata11[24];
    char stringdata12[30];
    char stringdata13[26];
    char stringdata14[22];
    char stringdata15[22];
    char stringdata16[9];
    char stringdata17[14];
    char stringdata18[8];
    char stringdata19[11];
    char stringdata20[11];
    char stringdata21[16];
    char stringdata22[19];
    char stringdata23[3];
    char stringdata24[8];
    char stringdata25[18];
    char stringdata26[20];
    char stringdata27[13];
    char stringdata28[14];
    char stringdata29[13];
    char stringdata30[13];
    char stringdata31[10];
    char stringdata32[8];
    char stringdata33[23];
    char stringdata34[23];
    char stringdata35[19];
    char stringdata36[4];
    char stringdata37[6];
    char stringdata38[21];
    char stringdata39[18];
    char stringdata40[20];
    char stringdata41[23];
    char stringdata42[23];
    char stringdata43[19];
    char stringdata44[20];
    char stringdata45[19];
    char stringdata46[24];
    char stringdata47[15];
    char stringdata48[22];
    char stringdata49[15];
    char stringdata50[17];
    char stringdata51[14];
    char stringdata52[22];
    char stringdata53[22];
    char stringdata54[21];
    char stringdata55[11];
    char stringdata56[13];
    char stringdata57[12];
    char stringdata58[17];
    char stringdata59[20];
    char stringdata60[17];
    char stringdata61[15];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSUsageStoreENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSUsageStoreENDCLASS_t qt_meta_stringdata_CLASSUsageStoreENDCLASS = {
    {
        QT_MOC_LITERAL(0, 10),  // "UsageStore"
        QT_MOC_LITERAL(11, 15),  // "snapshotChanged"
        QT_MOC_LITERAL(27, 0),  // ""
        QT_MOC_LITERAL(28, 10),  // "providerId"
        QT_MOC_LITERAL(39, 17),  // "refreshingChanged"
        QT_MOC_LITERAL(57, 18),  // "providerIDsChanged"
        QT_MOC_LITERAL(76, 13),  // "errorOccurred"
        QT_MOC_LITERAL(90, 7),  // "message"
        QT_MOC_LITERAL(98, 23),  // "costUsageEnabledChanged"
        QT_MOC_LITERAL(122, 26),  // "costUsageRefreshingChanged"
        QT_MOC_LITERAL(149, 16),  // "costUsageChanged"
        QT_MOC_LITERAL(166, 23),  // "snapshotRevisionChanged"
        QT_MOC_LITERAL(190, 29),  // "providerConnectionTestChanged"
        QT_MOC_LITERAL(220, 25),  // "providerLoginStateChanged"
        QT_MOC_LITERAL(246, 21),  // "providerStatusChanged"
        QT_MOC_LITERAL(268, 21),  // "statusRevisionChanged"
        QT_MOC_LITERAL(290, 8),  // "snapshot"
        QT_MOC_LITERAL(299, 13),  // "UsageSnapshot"
        QT_MOC_LITERAL(313, 7),  // "refresh"
        QT_MOC_LITERAL(321, 10),  // "refreshAll"
        QT_MOC_LITERAL(332, 10),  // "clearCache"
        QT_MOC_LITERAL(343, 15),  // "refreshProvider"
        QT_MOC_LITERAL(359, 18),  // "setProviderEnabled"
        QT_MOC_LITERAL(378, 2),  // "id"
        QT_MOC_LITERAL(381, 7),  // "enabled"
        QT_MOC_LITERAL(389, 17),  // "isProviderEnabled"
        QT_MOC_LITERAL(407, 19),  // "providerDisplayName"
        QT_MOC_LITERAL(427, 12),  // "snapshotData"
        QT_MOC_LITERAL(440, 13),  // "providerError"
        QT_MOC_LITERAL(454, 12),  // "providerList"
        QT_MOC_LITERAL(467, 12),  // "moveProvider"
        QT_MOC_LITERAL(480, 9),  // "fromIndex"
        QT_MOC_LITERAL(490, 7),  // "toIndex"
        QT_MOC_LITERAL(498, 22),  // "providerSettingsFields"
        QT_MOC_LITERAL(521, 22),  // "providerDescriptorData"
        QT_MOC_LITERAL(544, 18),  // "setProviderSetting"
        QT_MOC_LITERAL(563, 3),  // "key"
        QT_MOC_LITERAL(567, 5),  // "value"
        QT_MOC_LITERAL(573, 20),  // "providerSecretStatus"
        QT_MOC_LITERAL(594, 17),  // "setProviderSecret"
        QT_MOC_LITERAL(612, 19),  // "clearProviderSecret"
        QT_MOC_LITERAL(632, 22),  // "testProviderConnection"
        QT_MOC_LITERAL(655, 22),  // "providerConnectionTest"
        QT_MOC_LITERAL(678, 18),  // "startProviderLogin"
        QT_MOC_LITERAL(697, 19),  // "cancelProviderLogin"
        QT_MOC_LITERAL(717, 18),  // "providerLoginState"
        QT_MOC_LITERAL(736, 23),  // "refreshProviderStatuses"
        QT_MOC_LITERAL(760, 14),  // "providerStatus"
        QT_MOC_LITERAL(775, 21),  // "providerUsageSnapshot"
        QT_MOC_LITERAL(797, 14),  // "allProviderIDs"
        QT_MOC_LITERAL(812, 16),  // "refreshCostUsage"
        QT_MOC_LITERAL(829, 13),  // "costUsageData"
        QT_MOC_LITERAL(843, 21),  // "providerCostUsageList"
        QT_MOC_LITERAL(865, 21),  // "providerCostUsageData"
        QT_MOC_LITERAL(887, 20),  // "utilizationChartData"
        QT_MOC_LITERAL(908, 10),  // "seriesName"
        QT_MOC_LITERAL(919, 12),  // "isRefreshing"
        QT_MOC_LITERAL(932, 11),  // "providerIDs"
        QT_MOC_LITERAL(944, 16),  // "costUsageEnabled"
        QT_MOC_LITERAL(961, 19),  // "costUsageRefreshing"
        QT_MOC_LITERAL(981, 16),  // "snapshotRevision"
        QT_MOC_LITERAL(998, 14)   // "statusRevision"
    },
    "UsageStore",
    "snapshotChanged",
    "",
    "providerId",
    "refreshingChanged",
    "providerIDsChanged",
    "errorOccurred",
    "message",
    "costUsageEnabledChanged",
    "costUsageRefreshingChanged",
    "costUsageChanged",
    "snapshotRevisionChanged",
    "providerConnectionTestChanged",
    "providerLoginStateChanged",
    "providerStatusChanged",
    "statusRevisionChanged",
    "snapshot",
    "UsageSnapshot",
    "refresh",
    "refreshAll",
    "clearCache",
    "refreshProvider",
    "setProviderEnabled",
    "id",
    "enabled",
    "isProviderEnabled",
    "providerDisplayName",
    "snapshotData",
    "providerError",
    "providerList",
    "moveProvider",
    "fromIndex",
    "toIndex",
    "providerSettingsFields",
    "providerDescriptorData",
    "setProviderSetting",
    "key",
    "value",
    "providerSecretStatus",
    "setProviderSecret",
    "clearProviderSecret",
    "testProviderConnection",
    "providerConnectionTest",
    "startProviderLogin",
    "cancelProviderLogin",
    "providerLoginState",
    "refreshProviderStatuses",
    "providerStatus",
    "providerUsageSnapshot",
    "allProviderIDs",
    "refreshCostUsage",
    "costUsageData",
    "providerCostUsageList",
    "providerCostUsageData",
    "utilizationChartData",
    "seriesName",
    "isRefreshing",
    "providerIDs",
    "costUsageEnabled",
    "costUsageRefreshing",
    "snapshotRevision",
    "statusRevision"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSUsageStoreENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      44,   14, // methods
       6,  398, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  278,    2, 0x06,    7 /* Public */,
       4,    0,  281,    2, 0x06,    9 /* Public */,
       5,    0,  282,    2, 0x06,   10 /* Public */,
       6,    2,  283,    2, 0x06,   11 /* Public */,
       8,    0,  288,    2, 0x06,   14 /* Public */,
       9,    0,  289,    2, 0x06,   15 /* Public */,
      10,    0,  290,    2, 0x06,   16 /* Public */,
      11,    0,  291,    2, 0x06,   17 /* Public */,
      12,    1,  292,    2, 0x06,   18 /* Public */,
      13,    1,  295,    2, 0x06,   20 /* Public */,
      14,    1,  298,    2, 0x06,   22 /* Public */,
      15,    0,  301,    2, 0x06,   24 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      16,    1,  302,    2, 0x102,   25 /* Public | MethodIsConst  */,
      18,    0,  305,    2, 0x02,   27 /* Public */,
      19,    0,  306,    2, 0x02,   28 /* Public */,
      20,    0,  307,    2, 0x02,   29 /* Public */,
      21,    1,  308,    2, 0x02,   30 /* Public */,
      22,    2,  311,    2, 0x02,   32 /* Public */,
      25,    1,  316,    2, 0x102,   35 /* Public | MethodIsConst  */,
      26,    1,  319,    2, 0x102,   37 /* Public | MethodIsConst  */,
      27,    1,  322,    2, 0x102,   39 /* Public | MethodIsConst  */,
      28,    1,  325,    2, 0x102,   41 /* Public | MethodIsConst  */,
      29,    0,  328,    2, 0x102,   43 /* Public | MethodIsConst  */,
      30,    2,  329,    2, 0x02,   44 /* Public */,
      33,    1,  334,    2, 0x102,   47 /* Public | MethodIsConst  */,
      34,    1,  337,    2, 0x102,   49 /* Public | MethodIsConst  */,
      35,    3,  340,    2, 0x02,   51 /* Public */,
      38,    2,  347,    2, 0x102,   55 /* Public | MethodIsConst  */,
      39,    3,  352,    2, 0x02,   58 /* Public */,
      40,    2,  359,    2, 0x02,   62 /* Public */,
      41,    1,  364,    2, 0x02,   65 /* Public */,
      42,    1,  367,    2, 0x102,   67 /* Public | MethodIsConst  */,
      43,    1,  370,    2, 0x02,   69 /* Public */,
      44,    1,  373,    2, 0x02,   71 /* Public */,
      45,    1,  376,    2, 0x102,   73 /* Public | MethodIsConst  */,
      46,    0,  379,    2, 0x02,   75 /* Public */,
      47,    1,  380,    2, 0x102,   76 /* Public | MethodIsConst  */,
      48,    1,  383,    2, 0x102,   78 /* Public | MethodIsConst  */,
      49,    0,  386,    2, 0x102,   80 /* Public | MethodIsConst  */,
      50,    0,  387,    2, 0x02,   81 /* Public */,
      51,    0,  388,    2, 0x102,   82 /* Public | MethodIsConst  */,
      52,    0,  389,    2, 0x102,   83 /* Public | MethodIsConst  */,
      53,    1,  390,    2, 0x102,   84 /* Public | MethodIsConst  */,
      54,    2,  393,    2, 0x102,   86 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,

 // methods: parameters
    0x80000000 | 17, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   23,   24,
    QMetaType::Bool, QMetaType::QString,   23,
    QMetaType::QString, QMetaType::QString,   23,
    QMetaType::QVariantMap, QMetaType::QString,   23,
    QMetaType::QString, QMetaType::QString,   23,
    QMetaType::QVariantList,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   31,   32,
    QMetaType::QVariantList, QMetaType::QString,   23,
    QMetaType::QVariantMap, QMetaType::QString,   23,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QVariant,    3,   36,   37,
    QMetaType::QVariantMap, QMetaType::QString, QMetaType::QString,    3,   36,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString, QMetaType::QString,    3,   36,   37,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString,    3,   36,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::QVariantMap, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::QVariantMap, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::QVariantMap, QMetaType::QString,    3,
    QMetaType::QVariantMap, QMetaType::QString,    3,
    QMetaType::QStringList,
    QMetaType::Void,
    QMetaType::QVariantMap,
    QMetaType::QVariantList,
    QMetaType::QVariantMap, QMetaType::QString,    3,
    QMetaType::QVariantList, QMetaType::QString, QMetaType::QString,    3,   55,

 // properties: name, type, flags
      56, QMetaType::Bool, 0x00015001, uint(1), 0,
      57, QMetaType::QStringList, 0x00015001, uint(2), 0,
      58, QMetaType::Bool, 0x00015103, uint(4), 0,
      59, QMetaType::Bool, 0x00015001, uint(5), 0,
      60, QMetaType::Int, 0x00015001, uint(7), 0,
      61, QMetaType::Int, 0x00015001, uint(11), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject UsageStore::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSUsageStoreENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSUsageStoreENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSUsageStoreENDCLASS_t,
        // property 'isRefreshing'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'providerIDs'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'costUsageEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'costUsageRefreshing'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'snapshotRevision'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'statusRevision'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UsageStore, std::true_type>,
        // method 'snapshotChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refreshingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'providerIDsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'costUsageEnabledChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'costUsageRefreshingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'costUsageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'snapshotRevisionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'providerConnectionTestChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'providerLoginStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'providerStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'statusRevisionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'snapshot'
        QtPrivate::TypeAndForceComplete<UsageSnapshot, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refresh'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearCache'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshProvider'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setProviderEnabled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'isProviderEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'providerDisplayName'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'snapshotData'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'providerError'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'providerList'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        // method 'moveProvider'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'providerSettingsFields'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'providerDescriptorData'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setProviderSetting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariant &, std::false_type>,
        // method 'providerSecretStatus'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setProviderSecret'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clearProviderSecret'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'testProviderConnection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'providerConnectionTest'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'startProviderLogin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'cancelProviderLogin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'providerLoginState'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'refreshProviderStatuses'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'providerStatus'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'providerUsageSnapshot'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'allProviderIDs'
        QtPrivate::TypeAndForceComplete<QStringList, std::false_type>,
        // method 'refreshCostUsage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'costUsageData'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        // method 'providerCostUsageList'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        // method 'providerCostUsageData'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'utilizationChartData'
        QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void UsageStore::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UsageStore *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->snapshotChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->refreshingChanged(); break;
        case 2: _t->providerIDsChanged(); break;
        case 3: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->costUsageEnabledChanged(); break;
        case 5: _t->costUsageRefreshingChanged(); break;
        case 6: _t->costUsageChanged(); break;
        case 7: _t->snapshotRevisionChanged(); break;
        case 8: _t->providerConnectionTestChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->providerLoginStateChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->providerStatusChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->statusRevisionChanged(); break;
        case 12: { UsageSnapshot _r = _t->snapshot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< UsageSnapshot*>(_a[0]) = std::move(_r); }  break;
        case 13: _t->refresh(); break;
        case 14: _t->refreshAll(); break;
        case 15: _t->clearCache(); break;
        case 16: _t->refreshProvider((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 17: _t->setProviderEnabled((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 18: { bool _r = _t->isProviderEnabled((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 19: { QString _r = _t->providerDisplayName((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 20: { QVariantMap _r = _t->snapshotData((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 21: { QString _r = _t->providerError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 22: { QVariantList _r = _t->providerList();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 23: _t->moveProvider((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 24: { QVariantList _r = _t->providerSettingsFields((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 25: { QVariantMap _r = _t->providerDescriptorData((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 26: _t->setProviderSetting((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QVariant>>(_a[3]))); break;
        case 27: { QVariantMap _r = _t->providerSecretStatus((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 28: { bool _r = _t->setProviderSecret((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 29: { bool _r = _t->clearProviderSecret((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 30: _t->testProviderConnection((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 31: { QVariantMap _r = _t->providerConnectionTest((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 32: _t->startProviderLogin((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 33: _t->cancelProviderLogin((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 34: { QVariantMap _r = _t->providerLoginState((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 35: _t->refreshProviderStatuses(); break;
        case 36: { QVariantMap _r = _t->providerStatus((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 37: { QVariantMap _r = _t->providerUsageSnapshot((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 38: { QStringList _r = _t->allProviderIDs();
            if (_a[0]) *reinterpret_cast< QStringList*>(_a[0]) = std::move(_r); }  break;
        case 39: _t->refreshCostUsage(); break;
        case 40: { QVariantMap _r = _t->costUsageData();
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 41: { QVariantList _r = _t->providerCostUsageList();
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 42: { QVariantMap _r = _t->providerCostUsageData((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 43: { QVariantList _r = _t->utilizationChartData((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UsageStore::*)(const QString & );
            if (_t _q_method = &UsageStore::snapshotChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)();
            if (_t _q_method = &UsageStore::refreshingChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)();
            if (_t _q_method = &UsageStore::providerIDsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)(const QString & , const QString & );
            if (_t _q_method = &UsageStore::errorOccurred; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)();
            if (_t _q_method = &UsageStore::costUsageEnabledChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)();
            if (_t _q_method = &UsageStore::costUsageRefreshingChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)();
            if (_t _q_method = &UsageStore::costUsageChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)();
            if (_t _q_method = &UsageStore::snapshotRevisionChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)(const QString & );
            if (_t _q_method = &UsageStore::providerConnectionTestChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)(const QString & );
            if (_t _q_method = &UsageStore::providerLoginStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)(const QString & );
            if (_t _q_method = &UsageStore::providerStatusChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (UsageStore::*)();
            if (_t _q_method = &UsageStore::statusRevisionChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<UsageStore *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isRefreshing(); break;
        case 1: *reinterpret_cast< QStringList*>(_v) = _t->providerIDs(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->costUsageEnabled(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->costUsageRefreshing(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->snapshotRevision(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->statusRevision(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<UsageStore *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 2: _t->setCostUsageEnabled(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *UsageStore::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UsageStore::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSUsageStoreENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UsageStore::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 44)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 44;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 44)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 44;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void UsageStore::snapshotChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void UsageStore::refreshingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void UsageStore::providerIDsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void UsageStore::errorOccurred(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void UsageStore::costUsageEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void UsageStore::costUsageRefreshingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void UsageStore::costUsageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void UsageStore::snapshotRevisionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void UsageStore::providerConnectionTestChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void UsageStore::providerLoginStateChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void UsageStore::providerStatusChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void UsageStore::statusRevisionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}
QT_WARNING_POP
