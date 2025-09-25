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

            //** WAKE ON WLAN **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10
                visible: HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled

                RowLayout {
                    spacing: 10

                    Text {
                        id: wowlanText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        //: Title for indication of wifi always on functionality
                        text: qsTr("Keep WiFi connected in standby")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: wowlanSwitch
                        icon: "uc:check"
                        checked: Config.wowlanEnabled
                        trigger: function() {
                            Config.wowlanEnabled = !Config.wowlanEnabled;
                        }

                        /** KEYBOARD NAVIGATION **/
                        KeyNavigation.down: wakeupSensitivitySlider
                        highlight: activeFocus && ui.keyNavigationEnabled

                        Component.onCompleted: {
                            if (Wifi.wowlanEnabled) {
                                wowlanSwitch.forceActiveFocus();
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Keeps WiFi always connected, even when the device is sleeping. Allows for faster reconnect after wakeup. Please note that enabling this feature slightly decreases battery life.")
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
                visible: HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled
            }

            /** WAKEUP SENSITIVITY **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height + 40

                Text {
                    id: wakeupSensitivityText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    //: Movement the remote reacts to wake up
                    text: qsTr("Wakeup sensitivity")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    id: wakeupSensitivitySmallText
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Amount of movement needed to wake up the remote.")
                    anchors { left: parent.left; top:wakeupSensitivityText.bottom; topMargin: 5 }
                    font: fonts.secondaryFont(24)
                }

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
                    anchors { top: wakeupSensitivitySmallText.bottom; topMargin: 10 }

                    onUserInteractionEnded: {
                        Config.wakeupSensitivity = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: (HwInfo.modelNumber == "UCR2" ? true : Wifi.wowlanEnabled) ? wowlanSwitch : undefined
                    KeyNavigation.down: displayoffTimeoutSlider
                    highlight: activeFocus && ui.keyNavigationEnabled

                    Component.onCompleted: {
                        if (HwInfo.modelNumber != "UCR3") {
                            wakeupSensitivitySlider.forceActiveFocus();
                        }
                    }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** DISPLAY TIMEOUT **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height + 40

                Text {
                    id: displayTimeoutText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    //: How much time the display will turn off after
                    text: qsTr("Display off timeout")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    color: colors.light
                    text: Config.displayTimeout + "s"
                    anchors { right: parent.right; baseline: displayTimeoutText.baseline }
                    font: fonts.secondaryFont(24)
                }

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
                    anchors { top: displayTimeoutText.bottom; topMargin: 10 }

                    onValueChanged: {
                        valueDisplayText = value + "s"
                    }

                    onUserInteractionEnded: {
                        Config.displayTimeout = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: wakeupSensitivitySlider
                    KeyNavigation.down: sleepTimeoutSlider
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** SLEEP TIMEOUT **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height

                Text {
                    id: sleepTimeoutText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    //: How much time the remote will enter sleep mode after
                    text: qsTr("Sleep timeout")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Text {
                    color: colors.light
                    text:  secondsToTime(Config.sleepTimeout)
                    anchors { right: parent.right; baseline: sleepTimeoutText.baseline }
                    font: fonts.secondaryFont(24)
                }

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
                    anchors { top: sleepTimeoutText.bottom; topMargin: 10 }

                    onValueChanged: {
                        valueDisplayText = secondsToTime(value);
                    }

                    onUserInteractionEnded: {
                        Config.sleepTimeout = value;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: displayoffTimeoutSlider
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }
        }
    }
}
