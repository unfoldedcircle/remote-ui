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

            /** MICROPHONE ENABLE **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10

                RowLayout {
                    spacing: 10

                    Text {
                        id: microphoneText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Microphone")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: microphoneSwitch
                        checked: Config.micEnabled
                        trigger: function() {
                            Config.micEnabled = !Config.micEnabled;
                        }

                        /** KEYBOARD NAVIGATION **/
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.down: speechResponseSwitch
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Disabling the microphone will completely turn it off.  You won’t be able to use voice assistants.")
                    font: fonts.secondaryFont(24)
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignCenter
                width: parent.width - 20; height: 2
                color: colors.medium
                visible: Config.micEnabled
            }

            /** VOICE CONTROL ENABLE **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10
                visible: Config.micEnabled

                RowLayout {
                    spacing: 10

                    Text {
                        id: voiceText
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Voice Assistant")
                        font: fonts.primaryFont(30)
                    }
                }

                Component.onCompleted: {
                    if (Config.voiceAssistantId == "") {
                        voiceAssistantName.text = qsTr("None selected");
                        return;
                    }

                    let e = EntityController.get(Config.voiceAssistantId);
                    if (!e) {
                        EntityController.load(Config.voiceAssistantId);
                        connectSignalSlot(EntityController.entityLoaded, function(success, entityId) {
                            if (success && entityId == Config.voiceAssistantId) {
                                e = EntityController.get(Config.voiceAssistantId);

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
                        });
                    } else {
                        voiceAssistantName.text = e.name;
                    }
                }

                RowLayout {
                    spacing: 10

                    Text {
                        id: voiceAssistantName
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        font: fonts.primaryFont(26)
                    }
                }

                RowLayout {
                    spacing: 10

                    Text {
                        id: voiceAssistanProfiletName
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.light
                        font: fonts.primaryFont(22)
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                }

                RowLayout {
                    spacing: 10

                    Text {
                        text: qsTr("Use the Web Configurator to edit voice assistants.")
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.light
                        font: fonts.primaryFont(20)
                    }
                }
            }


            /** SPEECH RESPONSE ENABLE **/
            ColumnLayout {
                Layout.alignment: Qt.AlignCenter
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 10
                visible: Config.voiceAssistantId != ""

                RowLayout {
                    spacing: 10

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        text: qsTr("Speech response")
                        font: fonts.primaryFont(30)
                    }

                    Components.Switch {
                        id: speechResponseSwitch
                        checked: Config.voiceAssistantSpeechResponse
                        trigger: function() {
                            Config.voiceAssistantSpeechResponse = !Config.voiceAssistantSpeechResponse;
                        }

                        /** KEYBOARD NAVIGATION **/
                        highlight: activeFocus && ui.keyNavigationEnabled
                        KeyNavigation.up: microphoneSwitch
                    }
                }

                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Play speech response from Voice Assistant when supported.")
                    font: fonts.secondaryFont(24)
                }
            }
        }
    }
}
