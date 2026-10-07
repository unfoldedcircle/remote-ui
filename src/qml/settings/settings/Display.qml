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
    scrollTarget: flickable
    initialFocusItem: displayAutoBrightnessSwitch

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

            Components.SettingRow {
                //: Title for indication of auto brightness functionality
                title: qsTr("Auto brightness")
                help: qsTr("Automatically adjust the display brightness based on ambient lighting conditions.")

                Components.Switch {
                    id: displayAutoBrightnessSwitch
                    icon: "uc:check"
                    checked: Config.displayAutoBrightness
                    trigger: function() {
                        Config.displayAutoBrightness = !Config.displayAutoBrightness;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: displayBrightnessSlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                title: qsTr("Display brightness")
                value: Math.round(displayBrightnessSlider.value) + "%"
                controlBelow: true

                Components.Slider {
                    id: displayBrightnessSlider
                    height: 60
                    from: 5
                    to: 100
                    stepSize: 1
                    value: Config.displayBrightness
                    live: true

                    onValueChanged: {
                        Config.displayBrightness = value;
                    }

                    onUserInteractionEnded: {
                        Config.displayBrightness = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: displayAutoBrightnessSwitch
                    KeyNavigation.down: buttonBacklightSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                //: Title for button backlight functionality
                title: qsTr("Button backlight")
                help: qsTr("When on, button backlight will automatically turn on in a dark room.")

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
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            Components.SettingRow {
                title: qsTr("Button backlight brightness")
                value: Math.round(buttonBrightnessSlider.value) + "%"
                controlBelow: true
                showDivider: false

                Components.Slider {
                    id: buttonBrightnessSlider
                    height: 60
                    from: 0
                    to: 100
                    stepSize: 1
                    value: Config.buttonBrightness
                    live: true

                    onValueChanged: {
                        Config.buttonBrightness = value;
                    }

                    onUserInteractionEnded: {
                        Config.buttonBrightness = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: buttonBacklightSwitch
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }
        }
    }
}
