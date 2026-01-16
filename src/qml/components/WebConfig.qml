// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import SoftwareUpdate 1.0
import Haptic 1.0
import Wifi 1.0
import Config 1.0
import Integration.Controller 1.0
import Dock.Controller 1.0

import "qrc:/components" as Components

Rectangle {
    id: profileRoot
    width: parent.width; height: parent.height
    anchors.centerIn: parent
    color: colors.black

    signal closed

    property alias profileRoot: profileRoot
    property alias buttonNavigation: buttonNavigation

    function open() {
        buttonNavigation.takeControl();
    }

    function close() {
        buttonNavigation.releaseControl();
        closed();
    }

    function goHome() {
        buttonNavigation.releaseControl();
        closed();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    if (profileRoot.state == "showLargeQr") {
                        profileRoot.state = "";
                    } else {
                        goHome();
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    if (profileRoot.state == "showLargeQr") {
                        profileRoot.state = "";
                    } else {
                        goHome();
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height

            Text {
                text: qsTr("Web Configurator")
                color: colors.offwhite
                verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
                font: fonts.primaryFont(26)
            }

            Components.Icon {
                id: closeIcon

                color: colors.offwhite
                icon: "uc:arrow-left"
                size: 80

                anchors.left: parent.left

                Components.HapticMouseArea {
                    width: 120; height: 120
                    anchors.centerIn: parent
                    onClicked: {
                        close();
                    }
                }
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            Layout.bottomMargin: 20

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

                                property bool showIp: true

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
                                        if (Wifi.ipAddress) {
                                            webConfiguratorAddress.showIp = !webConfiguratorAddress.showIp;
                                        }
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
                                icon: "uc:arrow-rotate-right"
                                color: colors.light
                                size: 60
                                anchors.centerIn: parent

                                Behavior on color {
                                    ColorAnimation { duration: 200 }
                                }
                            }
                        }
                    }
                }
                // restricted
                Item {
                    Layout.alignment: Qt.AlignBottom
                    Layout.preferredHeight: 30
                    Layout.margins: 20

                    visible: ui.profile.restricted

                    Components.Icon {
                        id: lockIcon
                        icon: "uc:lock"
                        color: colors.offwhite
                        opacity: 0.6
                        anchors { left: parent.left }
                        size: 30
                    }

                    Text {
                        color: colors.offwhite
                        opacity: 0.6
                        //: Text explaining that the profile has restricted access
                        text: qsTr("Restricted")
                        anchors { left: lockIcon.right; leftMargin: 10; verticalCenter: lockIcon.verticalCenter }
                        font: fonts.secondaryFont(24)
                    }
                }
            }
        }

        Item {
            id: qrCode
            visible: Config.webConfiguratorEnabled && Config.webConfiguratorAddress != ""

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 20

            Rectangle {
                width: parent.height < parent.width ? parent.height : parent.width
                height: width
                anchors.centerIn: parent

                color: colors.transparent
                border { width: 10; color: colors.offwhite }

                Image {
                    anchors.fill: parent
                    anchors.margins: 10
                    fillMode: Image.PreserveAspectFit
                    antialiasing: false
                    source: "data:image/png;base64," + ui.createQrCode(("http://%1/configurator").arg(Config.webConfiguratorAddress))
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
