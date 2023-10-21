// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components

Item {
    Item {
        id: title
        width: parent.width
        height: 60

        Text {
            text: qsTr("You're all set")
            width: parent.width
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(24)
        }
    }

    Text {
        id: description
        width: parent.width - 40
        wrapMode: Text.WordWrap
        color: colors.light
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("You can add integrations or change configuration via the Web configurator.")
        anchors { top: title.bottom; topMargin: 20; horizontalCenter: parent.horizontalCenter }
        font: fonts.secondaryFont(24)
    }


    Rectangle {
        anchors { top: description.bottom; topMargin: 40; left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20 }
        height: childrenRect.height

        color: colors.transparent
        border { color: colors.medium; width: 2 }
        radius: ui.cornerRadiusSmall

        ColumnLayout {
            width: parent.width
            spacing: 0

            ColumnLayout {
                spacing: 0

                Layout.alignment: Qt.AlignBottom
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.margins: 20

                visible: !ui.profile.restricted

                // web configurator enable
                ColumnLayout {
                    Layout.alignment: Qt.AlignBottom
                    Layout.fillHeight: false
                    Layout.bottomMargin: Config.webConfiguratorEnabled ? 30 : 0

                    RowLayout {
                        Text {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignVCenter
                            color: colors.light
                            text: Config.webConfiguratorEnabled ? qsTr("Web configurator enabled") : qsTr("Web configurator disabled")
                            font: fonts.secondaryFont(22)
                        }

                        Components.Switch {
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                            icon: "uc:check"
                            checked: Config.webConfiguratorEnabled
                            trigger: function() {
                                Config.webConfiguratorEnabled = !Config.webConfiguratorEnabled
                            }
                        }
                    }

                    RowLayout {
                        visible: Config.webConfiguratorEnabled && Config.webConfiguratorAddress != ""

                        Text {
                            id: webConfiguratorAddress

                            property bool showIp: false

                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignVCenter
                            color: colors.light
                            text: ("http://%1/configurator").arg(webConfiguratorAddress.showIp ? Wifi.ipAddress : Config.webConfiguratorAddress)
                            font: fonts.secondaryFont(22)

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    webConfiguratorAddress.showIp = !webConfiguratorAddress.showIp;
                                }
                            }
                        }
                    }
                }

                // pin & qr code
                RowLayout {
                    id: pinQrContainer
                    spacing: 10
                    clip: true

                    Layout.alignment: Qt.AlignBottom
                    Layout.fillWidth: true
                    Layout.preferredHeight: Config.webConfiguratorEnabled ? 60 : 0

                    Behavior on Layout.preferredHeight {
                        NumberAnimation {
                            duration: 300
                            easing.type: Easing.OutExpo
                        }
                    }

                    RowLayout {
                        id: pinContainer
                        spacing: 10
                        width: childrenRect.width
                        height: childrenRect.height

                        property string pin: Config.webConfiguratorPin
                        property int containerWidth: 45
                        property int containerHeight: 60

                        Rectangle {
                            width: pinContainer.containerWidth
                            height: pinContainer.containerHeight
                            color: colors.black
                            border { color: colors.medium; width: 2 }
                            radius: ui.cornerRadiusSmall

                            Text {
                                text: pinContainer.pin[0]
                                color: colors.offwhite
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                font: fonts.primaryFont(36, "Light")
                                anchors.centerIn: parent
                            }
                        }

                        Rectangle {
                            width: pinContainer.containerWidth
                            height: pinContainer.containerHeight
                            color: colors.black
                            border { color: colors.medium; width: 2 }
                            radius: ui.cornerRadiusSmall

                            Text {
                                text: pinContainer.pin[1]
                                color: colors.offwhite
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                font: fonts.primaryFont(36, "Light")
                                anchors.centerIn: parent
                            }
                        }

                        Rectangle {
                            width: pinContainer.containerWidth
                            height: pinContainer.containerHeight
                            color: colors.black
                            border { color: colors.medium; width: 2 }
                            radius: ui.cornerRadiusSmall

                            Text {
                                text: pinContainer.pin[2]
                                color: colors.offwhite
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                font: fonts.primaryFont(36, "Light")
                                anchors.centerIn: parent
                            }
                        }

                        Rectangle {
                            width: pinContainer.containerWidth
                            height: pinContainer.containerHeight
                            color: colors.black
                            border { color: colors.medium; width: 2 }
                            radius: ui.cornerRadiusSmall

                            Text {
                                text: pinContainer.pin[3]
                                color: colors.offwhite
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                                font: fonts.primaryFont(36, "Light")
                                anchors.centerIn: parent
                            }
                        }
                    }

                    Components.HapticMouseArea {
                        Layout.preferredWidth: pinContainer.height
                        Layout.preferredHeight: pinContainer.height
                        Layout.alignment: Qt.AlignVCenter

                        onClicked: {
                            Config.generateNewWebConfigPin();
                        }

                        onPressed: generateQrCodeIcon.color = colors.highlight
                        onReleased: generateQrCodeIcon.color = colors.light

                        Components.Icon {
                            id: generateQrCodeIcon
                            icon: "uc:reboot"
                            color: colors.light
                            size: 60
                            anchors.centerIn: parent

                            Behavior on color {
                                ColorAnimation { duration: 200 }
                            }
                        }
                    }
                }

                Image {
                    id: qrCode

                    Layout.preferredWidth: 220
                    Layout.preferredHeight: 220
                    Layout.topMargin: 40

                    fillMode: Image.PreserveAspectFit
                    antialiasing: false
                    source: "data:image/png;base64," + ui.createQrCode(("http://%1/configurator").arg(Config.webConfiguratorAddress))
                    visible: Config.webConfiguratorAddress != "" && Config.webConfiguratorEnabled
                }
            }
        }
    }

    Components.Button {
        id: skipButton
        text: qsTr("Done")
        width: parent.width - 40
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
        trigger: function() {
            ui.setOnboarding(false);
            ui.inputController.activeController = containerMain;
            ui.showHelp = true;
        }
    }
}
