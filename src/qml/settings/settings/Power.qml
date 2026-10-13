// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import HwInfo 1.0
import Haptic 1.0
import Config 1.0
import Wifi 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: powerPageContent
    scrollTarget: flickable
    // the wowlan switch is hidden on some models, then the first slider takes the focus
    initialFocusItem: (HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled) ? wowlanSwitch
                                                                                 : resumeTimeoutValueSlider

    function secondsToTime(e){
        let m = Math.floor(e % 3600 / 60).toString();
        let s = Math.floor(e % 60).toString();

        let mDisplay = m > 0 ? m + "m" : "";
        let sDisplay = s > 0 ? s + "s" : "";

        return mDisplay + sDisplay;
    }

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

            //** WAKE ON WLAN **/
            Components.SettingRow {
                visible: HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled
                //: Title for indication of wifi always on functionality
                title: qsTr("Keep WiFi connected in standby")
                help: qsTr("Keeps WiFi always connected, even when the device is sleeping. Allows for faster reconnect after wakeup. Please note that enabling this feature slightly decreases battery life.")

                Components.Switch {
                    id: wowlanSwitch
                    icon: "uc:check"
                    checked: Config.wowlanEnabled
                    trigger: function() {
                        Config.wowlanEnabled = !Config.wowlanEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: resumeTimeoutValueSlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** RESUME TIMEOUT WINDOW **/
            Components.SettingRow {
                title: qsTr("Retry commands after wakeup")
                help: qsTr("Retry commands within %1 second(s) after wakeup.").arg(Config.resumeTimeoutWindowSec)
                controlBelow: true
                controlBottomSpace: 40

                Components.Slider {
                    id: resumeTimeoutValueSlider
                    height: 60
                    from: 0
                    to: 10
                    stepSize: 1
                    value: Config.resumeTimeoutWindowSec
                    //: The feature is switched off. Not a person with a disability.
                    lowValueText: qsTr("Disabled")
                    highValueText: qsTr("%1 seconds").arg(to)
                    live: true

                    onValueChanged: {
                        Config.resumeTimeoutWindowSec = value;
                    }

                    onUserInteractionEnded: {
                        Config.resumeTimeoutWindowSec = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: (HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled) ? wowlanSwitch : undefined
                    KeyNavigation.down: wakeupSensitivitySlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** WAKEUP SENSITIVITY **/
            Components.SettingRow {
                //: Movement the remote reacts to wake up
                title: qsTr("Wakeup sensitivity")
                help: qsTr("Amount of movement needed to wake up the remote.")
                controlBelow: true
                controlBottomSpace: 40

                Components.Slider {
                    id: wakeupSensitivitySlider
                    height: 60
                    from: 0
                    to: 3
                    stepSize: 1
                    value: Config.wakeupSensitivity
                    showLiveValue: false
                    showTicks: true
                    //: Wakeup is turned off
                    lowValueText: qsTr("Off")
                    //: More sensitive wakeup setting, as in the remote will be more sensitive to movement
                    highValueText: qsTr("Sensitivity")

                    onUserInteractionEnded: {
                        Config.wakeupSensitivity = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: resumeTimeoutValueSlider
                    KeyNavigation.down: displayoffTimeoutSlider
                    highlight: activeFocus && ui.keyNavigationActive

                }
            }

            /** DISPLAY TIMEOUT **/
            Components.SettingRow {
                //: How much time the display will turn off after
                title: qsTr("Display off timeout")
                value: Config.displayTimeout + "s"
                controlBelow: true
                controlBottomSpace: 40

                Components.Slider {
                    id: displayoffTimeoutSlider
                    height: 60
                    from: 10
                    to: 60
                    stepSize: 1
                    live: true
                    value: Config.displayTimeout
                    lowValueText: qsTr("%1 seconds").arg(from)
                    highValueText: qsTr("%1 seconds").arg(to)

                    onValueChanged: {
                        valueDisplayText = value + "s"
                    }

                    onUserInteractionEnded: {
                        Config.displayTimeout = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: wakeupSensitivitySlider
                    KeyNavigation.down: sleepTimeoutSlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }

            /** SLEEP TIMEOUT **/
            Components.SettingRow {
                showDivider: false
                //: How much time the remote will enter sleep mode after
                title: qsTr("Sleep timeout")
                value: secondsToTime(Config.sleepTimeout)
                controlBelow: true
                controlBottomSpace: 40

                Components.Slider {
                    id: sleepTimeoutSlider
                    height: 60
                    from: 10
                    to: 300
                    stepSize: 1
                    live: true
                    value: Config.sleepTimeout
                    lowValueText: qsTr("%1 seconds").arg(from)
                    highValueText: qsTr("%1 minutes").arg(5)

                    onValueChanged: {
                        valueDisplayText = secondsToTime(value);
                    }

                    onUserInteractionEnded: {
                        Config.sleepTimeout = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: displayoffTimeoutSlider
                    highlight: activeFocus && ui.keyNavigationActive
                }
            }
        }
    }
}
