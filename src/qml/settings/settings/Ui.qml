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

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom }
        contentWidth: content.width; contentHeight: content.height
        clip: true

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        onContentYChanged: {
            if (contentY < 0) {
                contentY = 0;
            }
            if (contentY > 1100) {
                contentY = 1100;
            }
        }

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 20
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            /** INVERTED BUTTON BEHAVIOUR **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        id: buttonFuncText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Inverted button behaviour")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: buttonFuncSwitch
                        icon: "uc:check"
                        checked: Config.entityButtonFuncInverted
                        trigger: function() {
                            Config.entityButtonFuncInverted = !Config.entityButtonFuncInverted;
                        }

                        /** KEYBOARD NAVIGATION **/
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.down: batteryPercentSwitch

                        Component.onCompleted: {
                            buttonFuncSwitch.forceActiveFocus();
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Inverts button functions on the main screen: short press to open the control screen, long press to quick toggle.")
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** SHOW BATTERY PERCENTAGE **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        id: batteryPercentText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Show battery percentage")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: batteryPercentSwitch
                        icon: "uc:check"
                        checked: Config.showBatteryPercentage
                        trigger: function() {
                            Config.showBatteryPercentage = !Config.showBatteryPercentage;
                        }

                        /** KEYBOARD NAVIGATION **/
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.up: buttonFuncSwitch
                        KeyNavigation.down: activityBarSwitch
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Always show the battery percentage next to the icon.")
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** ENABLE ACTIVITY BAR **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        id: activityBarText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Activities on pages")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: activityBarSwitch
                        icon: "uc:check"
                        checked: Config.enableActivityBar
                        trigger: function() {
                            Config.enableActivityBar = !Config.enableActivityBar;
                        }

                        /** KEYBOARD NAVIGATION **/
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.up: batteryPercentSwitch
                        KeyNavigation.down: mediaComponentSwitch
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Show the running activities and playing media players in the page header.")
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** FILL IMAGE IN MEDIA COMPONENT **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        id: mediaComponentText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Zoom media image")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: mediaComponentSwitch
                        icon: "uc:check"
                        checked: Config.fillMediaArtwork
                        trigger: function() {
                            Config.fillMediaArtwork = !Config.fillMediaArtwork;
                        }

                        /** KEYBOARD NAVIGATION **/
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.up: activityBarSwitch
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Zoom & crop artwork in media player widgets instead of scaling to fit.")
                    font: fonts.secondaryFont(24)
                }
            }   
        }
    }
}
