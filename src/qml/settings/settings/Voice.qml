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
    id: voicePageContent

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

            /** VOICE CONTROL ENABLE **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height

                Text {
                    id: voiceText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Voice control")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: voiceSwitch
                    checked: false
                    anchors { top: voiceText.top; right: parent.right }
                    trigger: function() {
                        Config.voiceEnabled = !Config.voiceEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: microphoneSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled

                    Component.onCompleted: {
                        voiceSwitch.forceActiveFocus();
                    }
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Disabling voice control will still let you use voice dictation with integrations.\n\nPress and hold the voice button and say the command.")
                    anchors { left: parent.left; top: voiceText.bottom; topMargin: 5 }
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** MICROPHONE ENABLE **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height

                Text {
                    id: microphoneText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Microphone")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: microphoneSwitch
                    checked: Config.micEnabled
                    anchors { top: microphoneText.top; right: parent.right }
                    trigger: function() {
                        Config.micEnabled = !Config.micEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: voiceSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Disabling the microphone will completely turn it off. You won’t be able to use voice control or dictation with integrations")
                    anchors { left: parent.left; top: microphoneText.bottom; topMargin: 5 }
                    font: fonts.secondaryFont(24)
                }
            }
        }
    }
}
