// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0
import Entity.Controller 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components

Settings.Page {
    id: voicePageContent
    scrollTarget: flickable
    initialFocusItem: microphoneSwitch

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

            /** MICROPHONE ENABLE **/
            Components.SettingRow {
                title: qsTr("Microphone")
                help: qsTr("Disabling the microphone will completely turn it off.  You won’t be able to use voice assistants.")

                Components.Switch {
                    id: microphoneSwitch
                    checked: Config.micEnabled
                    trigger: function() {
                        Config.micEnabled = !Config.micEnabled;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.down: speechResponseSwitch
                }
            }

            /** VOICE CONTROL ENABLE **/
            Components.SettingRow {
                visible: Config.micEnabled
                title: qsTr("Voice Assistant")
                controlBelow: true

                Column {
                    width: parent.width
                    spacing: 6

                    Component.onCompleted: {
                        if (Config.voiceAssistantId == "") {
                            voiceAssistantName.text = qsTr("None selected");
                            return;
                        }

                        const e = EntityController.get(Config.voiceAssistantId);
                        if (e) {
                            voiceAssistantName.text = e.name;
                            const p = e.getProfile(Config.voiceAssistantProfileId);
                            if (p) {
                                voiceAssistanProfiletName.text = qsTr("Profile: %1").arg(p.name);
                            } else {
                                voiceAssistanProfiletName.text = qsTr("No profile selected");
                            }
                        } else {
                            voiceAssistantName.text = qsTr("None selected");
                            voiceAssistanProfiletName.text = qsTr("No profile selected");
                        }
                    }

                    Text {
                        id: voiceAssistantName
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.textPrimary
                        font: fonts.help()
                    }

                    Text {
                        id: voiceAssistanProfiletName
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: colors.textSecondary
                        font: fonts.help()
                    }

                    Text {
                        width: parent.width
                        topPadding: 14
                        text: qsTr("Use the Web Configurator to edit voice assistants.")
                        wrapMode: Text.WordWrap
                        color: colors.textSecondary
                        font: fonts.help()
                    }
                }
            }

            /** SPEECH RESPONSE ENABLE **/
            Components.SettingRow {
                showDivider: false
                visible: Config.voiceAssistantId != ""
                title: qsTr("Speech response")
                help: qsTr("Play speech response from Voice Assistant when supported.")

                Components.Switch {
                    id: speechResponseSwitch
                    checked: Config.voiceAssistantSpeechResponse
                    trigger: function() {
                        Config.voiceAssistantSpeechResponse = !Config.voiceAssistantSpeechResponse;
                    }

                    /** KEYBOARD NAVIGATION **/
                    highlight: activeFocus && ui.keyNavigationActive
                    KeyNavigation.up: microphoneSwitch
                }
            }
        }
    }
}
