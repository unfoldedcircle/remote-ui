// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: uiPageContent
    scrollTarget: flickable
    initialFocusItem: buttonFuncSwitch

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        boundsBehavior: Flickable.StopAtBounds

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 0
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            /** INVERTED BUTTON BEHAVIOUR **/
            Components.SettingRow {
                title: qsTr("Inverted button behaviour")
                help: qsTr("Inverts button functions on the main screen: short press to open the control screen, long press to quick toggle.")

                Components.Switch {
                    id: buttonFuncSwitch
                    icon: "uc:check"
                    checked: Config.entityButtonFuncInverted
                    trigger: function() {
                        Config.entityButtonFuncInverted = !Config.entityButtonFuncInverted;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.down: batteryPercentSwitch
                }
            }

            /** SHOW BATTERY PERCENTAGE **/
            Components.SettingRow {
                title: qsTr("Show battery percentage")
                help: qsTr("Always show the battery percentage next to the icon.")

                Components.Switch {
                    id: batteryPercentSwitch
                    icon: "uc:check"
                    checked: Config.showBatteryPercentage
                    trigger: function() {
                        Config.showBatteryPercentage = !Config.showBatteryPercentage;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.up: buttonFuncSwitch
                    KeyNavigation.down: batteryEveryWhereSwitch
                }
            }

            /** SHOW BATTERY EVERYWHERE **/
            Components.SettingRow {
                title: qsTr("Show battery indicator everywhere")
                help: qsTr("Shows the battery level indicator on all pages and activities.")

                Components.Switch {
                    id: batteryEveryWhereSwitch
                    icon: "uc:check"
                    checked: Config.showBatteryEveryWhere
                    trigger: function() {
                        Config.showBatteryEveryWhere = !Config.showBatteryEveryWhere;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.up: batteryPercentSwitch
                    KeyNavigation.down: activityBarSwitch
                }
            }

            /** ENABLE ACTIVITY BAR **/
            Components.SettingRow {
                title: qsTr("Activities on pages")
                help: qsTr("Show the running activities and playing media players in the page header.")

                Components.Switch {
                    id: activityBarSwitch
                    icon: "uc:check"
                    checked: Config.enableActivityBar
                    trigger: function() {
                        Config.enableActivityBar = !Config.enableActivityBar;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.up: batteryEveryWhereSwitch
                    KeyNavigation.down: apiActivitySwitch
                }
            }

            /** OPEN ACTIVITIES STARTED VIA THE API **/
            Components.SettingRow {
                title: qsTr("Open activities started with the API")
                help: qsTr("Open the activity screen when an activity is started outside of the remote, replacing whatever is on screen.")

                Components.Switch {
                    id: apiActivitySwitch
                    icon: "uc:check"
                    checked: Config.openActivityOnApiStart
                    trigger: function() {
                        Config.openActivityOnApiStart = !Config.openActivityOnApiStart;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.up: activityBarSwitch
                    KeyNavigation.down: mediaComponentSwitch
                }
            }

            /** FILL IMAGE IN MEDIA COMPONENT **/
            Components.SettingRow {
                title: qsTr("Zoom media image")
                help: qsTr("Zoom & crop artwork in media player widgets instead of scaling to fit.")

                Components.Switch {
                    id: mediaComponentSwitch
                    icon: "uc:check"
                    checked: Config.fillMediaArtwork
                    trigger: function() {
                        Config.fillMediaArtwork = !Config.fillMediaArtwork;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.up: apiActivitySwitch
                    KeyNavigation.down: mediaCoverflowSwitch
                }
            }

            /** MEDIA BROWSER COVERFLOW DEFAULT **/
            Components.SettingRow {
                showDivider: false
                title: qsTr("Coverflow in media browser")
                help: qsTr("Use coverflow as the default view when opening the media browser.")

                Components.Switch {
                    id: mediaCoverflowSwitch
                    icon: "uc:check"
                    checked: Config.mediaCoverflowDefault
                    trigger: function() {
                        Config.mediaCoverflowDefault = !Config.mediaCoverflowDefault;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.up: mediaComponentSwitch
                }
            }
        }
    }
}
