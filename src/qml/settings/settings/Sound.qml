// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

 
import Haptic 1.0
import Config 1.0
import SoundEffects 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: soundPageContent

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

            /** SOUND EFFECTS **/
            RowLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                Text {
                    id: soundEffectsText
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Sound effects")
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: soundEffectsSwitch
                    icon: "uc:check"
                    checked: Config.soundEnabled
                    trigger: function() {
                        Config.soundEnabled = !Config.soundEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.down: soundEffectsVolumeSlider
                    highlight: activeFocus && ui.keyNavigationEnabled

                    Component.onCompleted: {
                        soundEffectsSwitch.forceActiveFocus();
                    }
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** SOUND EFFECTS VOLUME **/
            Item {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20
                height: childrenRect.height

                Text {
                    id: soundEffectsVolumeText
                    width: parent.width - 80
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Sound effects volume")
                    anchors { left: parent.left; top:parent.top }
                    font: fonts.primaryFont(30)
                }

                Components.Slider {
                    id: soundEffectsVolumeSlider
                    height: 60
                    from: 0
                    to: 100
                    stepSize: 1
                    value: Config.soundVolume
                    live: true
                    anchors { top: soundEffectsVolumeText.bottom; topMargin: 10 }

                    onUserInteractionEnded: {
                        Config.soundVolume = value;
                        SoundEffects.play(SoundEffects.Click);
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: soundEffectsSwitch
                    KeyNavigation.down: buttonBacklightSwitch
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
            }

            /** HAPTIC FEEDBACK **/
            RowLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                Text {
                    id: hapticFeedbackText
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.offwhite
                    text: qsTr("Haptic feedback")
                    font: fonts.primaryFont(30)
                }

                Components.Switch {
                    id: buttonBacklightSwitch
                    icon: "uc:check"
                    checked: Config.hapticEnabled
                    trigger: function() {
                        Config.hapticEnabled = !Config.hapticEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    KeyNavigation.up: soundEffectsVolumeSlider
                    highlight: activeFocus && ui.keyNavigationEnabled
                }
            }
        }
    }
}
