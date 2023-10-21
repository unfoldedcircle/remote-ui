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
    id: displayPageContent

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


            /** AUTO BRIGHTNESS **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        id: autoBrightnessText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        //: Title for indication of auto brightness functionality
                        text: qsTr("Auto brightness")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: displayAutoBrightnessSwitch
                        icon: "uc:check"
                        checked: Config.displayAutoBrightness
                        trigger: function() {
                            Config.displayAutoBrightness = !Config.displayAutoBrightness;
                        }

                        /** KEYBOARD NAVIGATION **/
                        KeyNavigation.down: displayBrightnessSlider
                        highlight: activeFocus && ui.keyNavigationEnabled

                        Component.onCompleted: {
                            displayAutoBrightnessSwitch.forceActiveFocus();
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Automatically adjust the display brightness based on ambient lighting conditions.")
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** DISPLAY BRIGHTNESS **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height

                Text {
                    id: displayBrightnessText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Display brightness")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Components.Slider {
                    id: displayBrightnessSlider
                    height: 60
                    from: 20
                    to: 100
                    stepSize: 1
                    value: Config.displayBrightness
                    live: true
                    anchors { top: displayBrightnessText.bottom; topMargin: 10 }

                    onValueChanged: {
                        Config.displayBrightness = value;
                    }

                    onUserInteractionEnded: {
                        Config.displayBrightness = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: displayAutoBrightnessSwitch
                    KeyNavigation.down: buttonBacklightSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** BUTTON BACKLIGHT **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        id: buttonBacklightText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        //: Title for button backlight functionality
                        text: qsTr("Button backlight")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: buttonBacklightSwitch
                        icon: "uc:check"
                        checked: Config.buttonAutoBirghtness
                        trigger: function() {
                            Config.buttonAutoBirghtness = !Config.buttonAutoBirghtness;
                        }

                        /** KEYBOARD NAVIGATION **/
                        KeyNavigation.up: displayBrightnessSlider
                        KeyNavigation.down: buttonBrightnessSlider
                        highlight: activeFocus && ui.keyNavigationEnabled
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("When on, button backlight will automatically turn on in a dark room.")
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** BUTTON BRIGHTNESS **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height

                Text {
                    id: buttonBrightnessText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Button backlight brightness")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Components.Slider {
                    id: buttonBrightnessSlider
                    height: 60
                    from: 0
                    to: 100
                    stepSize: 1
                    value: Config.buttonBrightness
                    live: true
                    anchors { top: buttonBrightnessText.bottom; topMargin: 10 }

                    onValueChanged: {
                        Config.buttonBrightness = value;
                    }

                    onUserInteractionEnded: {
                        Config.buttonBrightness = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: buttonBacklightSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }
        }
    }
}
