/****************************************************************************
** Meta object code from reading C++ file 'SettingsStore.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/app/SettingsStore.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SettingsStore.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSSettingsStoreENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSSettingsStoreENDCLASS = QtMocHelpers::stringData(
    "SettingsStore",
    "refreshFrequencyChanged",
    "",
    "launchAtLoginChanged",
    "checkForUpdatesChanged",
    "debugMenuEnabledChanged",
    "mergeIconsChanged",
    "statusChecksEnabledChanged",
    "usageBarsShowUsedChanged",
    "resetTimesShowAbsoluteChanged",
    "showOptionalCreditsAndExtraUsageChanged",
    "sessionQuotaNotificationsEnabledChanged",
    "languageChanged",
    "providerOrderChanged",
    "providerSettingChanged",
    "providerID",
    "key",
    "isProviderEnabled",
    "id",
    "setProviderEnabled",
    "enabled",
    "providerOrder",
    "setProviderOrder",
    "order",
    "providerSetting",
    "defaultValue",
    "setProviderSetting",
    "value",
    "saveConfig",
    "resetToDefaults",
    "refreshFrequency",
    "launchAtLogin",
    "checkForUpdates",
    "debugMenuEnabled",
    "mergeIcons",
    "statusChecksEnabled",
    "usageBarsShowUsed",
    "resetTimesShowAbsolute",
    "showOptionalCreditsAndExtraUsage",
    "sessionQuotaNotificationsEnabled",
    "language"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSSettingsStoreENDCLASS_t {
    uint offsetsAndSizes[82];
    char stringdata0[14];
    char stringdata1[24];
    char stringdata2[1];
    char stringdata3[21];
    char stringdata4[23];
    char stringdata5[24];
    char stringdata6[18];
    char stringdata7[27];
    char stringdata8[25];
    char stringdata9[30];
    char stringdata10[40];
    char stringdata11[40];
    char stringdata12[16];
    char stringdata13[21];
    char stringdata14[23];
    char stringdata15[11];
    char stringdata16[4];
    char stringdata17[18];
    char stringdata18[3];
    char stringdata19[19];
    char stringdata20[8];
    char stringdata21[14];
    char stringdata22[17];
    char stringdata23[6];
    char stringdata24[16];
    char stringdata25[13];
    char stringdata26[19];
    char stringdata27[6];
    char stringdata28[11];
    char stringdata29[16];
    char stringdata30[17];
    char stringdata31[14];
    char stringdata32[16];
    char stringdata33[17];
    char stringdata34[11];
    char stringdata35[20];
    char stringdata36[18];
    char stringdata37[23];
    char stringdata38[33];
    char stringdata39[33];
    char stringdata40[9];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSSettingsStoreENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSSettingsStoreENDCLASS_t qt_meta_stringdata_CLASSSettingsStoreENDCLASS = {
    {
        QT_MOC_LITERAL(0, 13),  // "SettingsStore"
        QT_MOC_LITERAL(14, 23),  // "refreshFrequencyChanged"
        QT_MOC_LITERAL(38, 0),  // ""
        QT_MOC_LITERAL(39, 20),  // "launchAtLoginChanged"
        QT_MOC_LITERAL(60, 22),  // "checkForUpdatesChanged"
        QT_MOC_LITERAL(83, 23),  // "debugMenuEnabledChanged"
        QT_MOC_LITERAL(107, 17),  // "mergeIconsChanged"
        QT_MOC_LITERAL(125, 26),  // "statusChecksEnabledChanged"
        QT_MOC_LITERAL(152, 24),  // "usageBarsShowUsedChanged"
        QT_MOC_LITERAL(177, 29),  // "resetTimesShowAbsoluteChanged"
        QT_MOC_LITERAL(207, 39),  // "showOptionalCreditsAndExtraUs..."
        QT_MOC_LITERAL(247, 39),  // "sessionQuotaNotificationsEnab..."
        QT_MOC_LITERAL(287, 15),  // "languageChanged"
        QT_MOC_LITERAL(303, 20),  // "providerOrderChanged"
        QT_MOC_LITERAL(324, 22),  // "providerSettingChanged"
        QT_MOC_LITERAL(347, 10),  // "providerID"
        QT_MOC_LITERAL(358, 3),  // "key"
        QT_MOC_LITERAL(362, 17),  // "isProviderEnabled"
        QT_MOC_LITERAL(380, 2),  // "id"
        QT_MOC_LITERAL(383, 18),  // "setProviderEnabled"
        QT_MOC_LITERAL(402, 7),  // "enabled"
        QT_MOC_LITERAL(410, 13),  // "providerOrder"
        QT_MOC_LITERAL(424, 16),  // "setProviderOrder"
        QT_MOC_LITERAL(441, 5),  // "order"
        QT_MOC_LITERAL(447, 15),  // "providerSetting"
        QT_MOC_LITERAL(463, 12),  // "defaultValue"
        QT_MOC_LITERAL(476, 18),  // "setProviderSetting"
        QT_MOC_LITERAL(495, 5),  // "value"
        QT_MOC_LITERAL(501, 10),  // "saveConfig"
        QT_MOC_LITERAL(512, 15),  // "resetToDefaults"
        QT_MOC_LITERAL(528, 16),  // "refreshFrequency"
        QT_MOC_LITERAL(545, 13),  // "launchAtLogin"
        QT_MOC_LITERAL(559, 15),  // "checkForUpdates"
        QT_MOC_LITERAL(575, 16),  // "debugMenuEnabled"
        QT_MOC_LITERAL(592, 10),  // "mergeIcons"
        QT_MOC_LITERAL(603, 19),  // "statusChecksEnabled"
        QT_MOC_LITERAL(623, 17),  // "usageBarsShowUsed"
        QT_MOC_LITERAL(641, 22),  // "resetTimesShowAbsolute"
        QT_MOC_LITERAL(664, 32),  // "showOptionalCreditsAndExtraUsage"
        QT_MOC_LITERAL(697, 32),  // "sessionQuotaNotificationsEnabled"
        QT_MOC_LITERAL(730, 8)   // "language"
    },
    "SettingsStore",
    "refreshFrequencyChanged",
    "",
    "launchAtLoginChanged",
    "checkForUpdatesChanged",
    "debugMenuEnabledChanged",
    "mergeIconsChanged",
    "statusChecksEnabledChanged",
    "usageBarsShowUsedChanged",
    "resetTimesShowAbsoluteChanged",
    "showOptionalCreditsAndExtraUsageChanged",
    "sessionQuotaNotificationsEnabledChanged",
    "languageChanged",
    "providerOrderChanged",
    "providerSettingChanged",
    "providerID",
    "key",
    "isProviderEnabled",
    "id",
    "setProviderEnabled",
    "enabled",
    "providerOrder",
    "setProviderOrder",
    "order",
    "providerSetting",
    "defaultValue",
    "setProviderSetting",
    "value",
    "saveConfig",
    "resetToDefaults",
    "refreshFrequency",
    "launchAtLogin",
    "checkForUpdates",
    "debugMenuEnabled",
    "mergeIcons",
    "statusChecksEnabled",
    "usageBarsShowUsed",
    "resetTimesShowAbsolute",
    "showOptionalCreditsAndExtraUsage",
    "sessionQuotaNotificationsEnabled",
    "language"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSSettingsStoreENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      22,   14, // methods
      11,  196, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  146,    2, 0x06,   12 /* Public */,
       3,    0,  147,    2, 0x06,   13 /* Public */,
       4,    0,  148,    2, 0x06,   14 /* Public */,
       5,    0,  149,    2, 0x06,   15 /* Public */,
       6,    0,  150,    2, 0x06,   16 /* Public */,
       7,    0,  151,    2, 0x06,   17 /* Public */,
       8,    0,  152,    2, 0x06,   18 /* Public */,
       9,    0,  153,    2, 0x06,   19 /* Public */,
      10,    0,  154,    2, 0x06,   20 /* Public */,
      11,    0,  155,    2, 0x06,   21 /* Public */,
      12,    0,  156,    2, 0x06,   22 /* Public */,
      13,    0,  157,    2, 0x06,   23 /* Public */,
      14,    2,  158,    2, 0x06,   24 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      17,    1,  163,    2, 0x102,   27 /* Public | MethodIsConst  */,
      19,    2,  166,    2, 0x02,   29 /* Public */,
      21,    0,  171,    2, 0x102,   32 /* Public | MethodIsConst  */,
      22,    1,  172,    2, 0x02,   33 /* Public */,
      24,    3,  175,    2, 0x102,   35 /* Public | MethodIsConst  */,
      24,    2,  182,    2, 0x122,   39 /* Public | MethodCloned | MethodIsConst  */,
      26,    3,  187,    2, 0x02,   42 /* Public */,
      28,    0,  194,    2, 0x02,   46 /* Public */,
      29,    0,  195,    2, 0x02,   47 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   15,   16,

 // methods: parameters
    QMetaType::Bool, QMetaType::QString,   18,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   18,   20,
    QMetaType::QStringList,
    QMetaType::Void, QMetaType::QStringList,   23,
    QMetaType::QVariant, QMetaType::QString, QMetaType::QString, QMetaType::QVariant,   15,   16,   25,
    QMetaType::QVariant, QMetaType::QString, QMetaType::QString,   15,   16,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QVariant,   15,   16,   27,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags
      30, QMetaType::Int, 0x00015103, uint(0), 0,
      31, QMetaType::Bool, 0x00015103, uint(1), 0,
      32, QMetaType::Bool, 0x00015103, uint(2), 0,
      33, QMetaType::Bool, 0x00015103, uint(3), 0,
      34, QMetaType::Bool, 0x00015103, uint(4), 0,
      35, QMetaType::Bool, 0x00015103, uint(5), 0,
      36, QMetaType::Bool, 0x00015103, uint(6), 0,
      37, QMetaType::Bool, 0x00015103, uint(7), 0,
      38, QMetaType::Bool, 0x00015103, uint(8), 0,
      39, QMetaType::Bool, 0x00015103, uint(9), 0,
      40, QMetaType::QString, 0x00015103, uint(10), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject SettingsStore::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSSettingsStoreENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSSettingsStoreENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSSettingsStoreENDCLASS_t,
        // property 'refreshFrequency'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'launchAtLogin'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'checkForUpdates'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'debugMenuEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'mergeIcons'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'statusChecksEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'usageBarsShowUsed'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'resetTimesShowAbsolute'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'showOptionalCreditsAndExtraUsage'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'sessionQuotaNotificationsEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'language'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SettingsStore, std::true_type>,
        // method 'refreshFrequencyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'launchAtLoginChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'checkForUpdatesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'debugMenuEnabledChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'mergeIconsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusChecksEnabledChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'usageBarsShowUsedChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetTimesShowAbsoluteChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showOptionalCreditsAndExtraUsageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sessionQuotaNotificationsEnabledChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'languageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'providerOrderChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'providerSettingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'isProviderEnabled'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setProviderEnabled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'providerOrder'
        QtPrivate::TypeAndForceComplete<QStringList, std::false_type>,
        // method 'setProviderOrder'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QStringList &, std::false_type>,
        // method 'providerSetting'
        QtPrivate::TypeAndForceComplete<QVariant, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariant &, std::false_type>,
        // method 'providerSetting'
        QtPrivate::TypeAndForceComplete<QVariant, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setProviderSetting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariant &, std::false_type>,
        // method 'saveConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetToDefaults'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void SettingsStore::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SettingsStore *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->refreshFrequencyChanged(); break;
        case 1: _t->launchAtLoginChanged(); break;
        case 2: _t->checkForUpdatesChanged(); break;
        case 3: _t->debugMenuEnabledChanged(); break;
        case 4: _t->mergeIconsChanged(); break;
        case 5: _t->statusChecksEnabledChanged(); break;
        case 6: _t->usageBarsShowUsedChanged(); break;
        case 7: _t->resetTimesShowAbsoluteChanged(); break;
        case 8: _t->showOptionalCreditsAndExtraUsageChanged(); break;
        case 9: _t->sessionQuotaNotificationsEnabledChanged(); break;
        case 10: _t->languageChanged(); break;
        case 11: _t->providerOrderChanged(); break;
        case 12: _t->providerSettingChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: { bool _r = _t->isProviderEnabled((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 14: _t->setProviderEnabled((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 15: { QStringList _r = _t->providerOrder();
            if (_a[0]) *reinterpret_cast< QStringList*>(_a[0]) = std::move(_r); }  break;
        case 16: _t->setProviderOrder((*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[1]))); break;
        case 17: { QVariant _r = _t->providerSetting((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QVariant>>(_a[3])));
            if (_a[0]) *reinterpret_cast< QVariant*>(_a[0]) = std::move(_r); }  break;
        case 18: { QVariant _r = _t->providerSetting((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QVariant*>(_a[0]) = std::move(_r); }  break;
        case 19: _t->setProviderSetting((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QVariant>>(_a[3]))); break;
        case 20: _t->saveConfig(); break;
        case 21: _t->resetToDefaults(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::refreshFrequencyChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::launchAtLoginChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::checkForUpdatesChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::debugMenuEnabledChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::mergeIconsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::statusChecksEnabledChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::usageBarsShowUsedChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::resetTimesShowAbsoluteChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::showOptionalCreditsAndExtraUsageChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::sessionQuotaNotificationsEnabledChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::languageChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)();
            if (_t _q_method = &SettingsStore::providerOrderChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (SettingsStore::*)(const QString & , const QString & );
            if (_t _q_method = &SettingsStore::providerSettingChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<SettingsStore *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = _t->refreshFrequency(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->launchAtLogin(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->checkForUpdates(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->debugMenuEnabled(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->mergeIcons(); break;
        case 5: *reinterpret_cast< bool*>(_v) = _t->statusChecksEnabled(); break;
        case 6: *reinterpret_cast< bool*>(_v) = _t->usageBarsShowUsed(); break;
        case 7: *reinterpret_cast< bool*>(_v) = _t->resetTimesShowAbsolute(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->showOptionalCreditsAndExtraUsage(); break;
        case 9: *reinterpret_cast< bool*>(_v) = _t->sessionQuotaNotificationsEnabled(); break;
        case 10: *reinterpret_cast< QString*>(_v) = _t->language(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<SettingsStore *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setRefreshFrequency(*reinterpret_cast< int*>(_v)); break;
        case 1: _t->setLaunchAtLogin(*reinterpret_cast< bool*>(_v)); break;
        case 2: _t->setCheckForUpdates(*reinterpret_cast< bool*>(_v)); break;
        case 3: _t->setDebugMenuEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 4: _t->setMergeIcons(*reinterpret_cast< bool*>(_v)); break;
        case 5: _t->setStatusChecksEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 6: _t->setUsageBarsShowUsed(*reinterpret_cast< bool*>(_v)); break;
        case 7: _t->setResetTimesShowAbsolute(*reinterpret_cast< bool*>(_v)); break;
        case 8: _t->setShowOptionalCreditsAndExtraUsage(*reinterpret_cast< bool*>(_v)); break;
        case 9: _t->setSessionQuotaNotificationsEnabled(*reinterpret_cast< bool*>(_v)); break;
        case 10: _t->setLanguage(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *SettingsStore::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsStore::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSSettingsStoreENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SettingsStore::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 22)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 22;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 22)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 22;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void SettingsStore::refreshFrequencyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SettingsStore::launchAtLoginChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SettingsStore::checkForUpdatesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SettingsStore::debugMenuEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SettingsStore::mergeIconsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SettingsStore::statusChecksEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SettingsStore::usageBarsShowUsedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SettingsStore::resetTimesShowAbsoluteChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void SettingsStore::showOptionalCreditsAndExtraUsageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void SettingsStore::sessionQuotaNotificationsEnabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void SettingsStore::languageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void SettingsStore::providerOrderChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void SettingsStore::providerSettingChanged(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}
QT_WARNING_POP
