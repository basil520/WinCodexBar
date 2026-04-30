import QtQuick 2.15
import QtQuick.Layouts 1.15
import CodexBar 1.0
import ".."
import "../components"

SettingsPage {
    title: qsTr("Display")
    subtitle: qsTr("Tune how usage and tray state are presented.")

    SettingsGroupBox {
        SettingsToggleRow {
            title: qsTr("Merge Icons")
            subtitle: qsTr("Show a single combined tray icon for enabled providers.")
            checked: SettingsStore.mergeIcons
            onToggled: function(checked) {
                SettingsStore.mergeIcons = checked
            }
        }

        SettingsToggleRow {
            title: qsTr("Show Usage Amount Used")
            subtitle: qsTr("Use consumed percentage instead of remaining percentage.")
            checked: SettingsStore.usageBarsShowUsed
            onToggled: function(checked) {
                SettingsStore.usageBarsShowUsed = checked
            }
        }
    }

    SettingsGroupBox {
        SettingsToggleRow {
            title: qsTr("Show Absolute Reset Times")
            subtitle: qsTr("Display exact reset times instead of relative wording.")
            checked: SettingsStore.resetTimesShowAbsolute
            onToggled: function(checked) {
                SettingsStore.resetTimesShowAbsolute = checked
            }
        }

        SettingsToggleRow {
            title: qsTr("Optional Credits and Extra Usage")
            subtitle: qsTr("Show additional provider-specific credit and usage fields.")
            checked: SettingsStore.showOptionalCreditsAndExtraUsage
            onToggled: function(checked) {
                SettingsStore.showOptionalCreditsAndExtraUsage = checked
            }
        }
    }
}
