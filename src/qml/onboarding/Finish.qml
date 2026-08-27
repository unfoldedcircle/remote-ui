// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Onboarding 1.0
import Config 1.0

import "qrc:/components" as Components
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    id: finishStep

    // the web configurator switch is only there for an unrestricted profile
    initialFocusItem: webConfiguratorSwitch.visible ? webConfiguratorSwitch : doneButton
    scrollTarget: flickable

    Component.onCompleted: {
        buttonNavigation.extendDefaultConfig({
                                                 // the setup can not be left at the last step
                                                 "BACK": {
                                                     "pressed": function() {}
                                                 }
                                             });
    }

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

    // translations of the description run well past the English text: the page scrolls, so the
    // "Done" button stays reachable in every language
    Flickable {
        id: flickable
        anchors { top: title.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
        contentWidth: width
        contentHeight: content.height + 20
        clip: true
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds

        // the content changes height while the pin row and the QR code come and go; a contentY
        // beyond the new end would leave an empty screen behind
        onContentHeightChanged: {
            const maxContentY = Math.max(0, contentHeight - height);
            if (contentY > maxContentY) {
                contentY = maxContentY;
            }
        }

        // Plain positioners and anchors throughout: the box sizes itself from its content, and
        // a QtQuick.Layouts layout inside a container sized from the layout's implicit height is
        // reported as a binding loop.
        Column {
            id: content
            width: flickable.width
            spacing: 40

            Text {
                id: description
                x: 20
                width: parent.width - 40

                wrapMode: Text.WordWrap
                color: colors.light
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("You can add integrations or change configuration via the Web configurator.")
                font: fonts.secondaryFont(24)
            }

            Rectangle {
                id: webConfiguratorBox
                x: 20
                width: parent.width - 40
                height: webConfiguratorContent.height + 40

                color: colors.transparent
                border { color: colors.medium; width: 2 }
                radius: ui.cornerRadiusSmall
                visible: !ui.profile.restricted

                Column {
                    id: webConfiguratorContent
                    x: 20; y: 20
                    width: parent.width - 40
                    spacing: 0

                    // web configurator enable
                    Item {
                        width: parent.width
                        height: Math.max(webConfiguratorSwitch.height, webConfiguratorEnabledText.implicitHeight)

                        Text {
                            id: webConfiguratorEnabledText
                            anchors { left: parent.left; right: webConfiguratorSwitch.left; rightMargin: 10; verticalCenter: parent.verticalCenter }

                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignVCenter
                            color: colors.light
                            text: Config.webConfiguratorEnabled ? qsTr("Web configurator enabled") : qsTr("Web configurator disabled")
                            font: fonts.secondaryFont(22)
                        }

                        Components.Switch {
                            id: webConfiguratorSwitch
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }

                            icon: "uc:check"
                            checked: Config.webConfiguratorEnabled
                            KeyNavigation.down: generateNewPin.visible ? generateNewPin : doneButton
                            trigger: function() {
                                Config.webConfiguratorEnabled = !Config.webConfiguratorEnabled
                            }
                        }
                    }

                    Text {
                        id: webConfiguratorAddress

                        property bool showIp: false

                        width: parent.width
                        topPadding: 10

                        visible: Config.webConfiguratorEnabled && Config.webConfiguratorAddress != ""
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
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

                    // pin & new pin button
                    Item {
                        id: pinQrContainer
                        width: parent.width
                        height: visible ? pinContainer.containerHeight + 30 : 0
                        visible: Config.webConfiguratorEnabled

                        Row {
                            id: pinContainer
                            spacing: 10
                            anchors { left: parent.left; bottom: parent.bottom }

                            property string pin: Config.webConfiguratorPin
                            property int containerWidth: 45
                            property int containerHeight: 60

                            Repeater {
                                model: 4

                                Rectangle {
                                    width: pinContainer.containerWidth
                                    height: pinContainer.containerHeight
                                    color: colors.black
                                    border { color: colors.medium; width: 2 }
                                    radius: ui.cornerRadiusSmall

                                    Text {
                                        text: pinContainer.pin[index] ? pinContainer.pin[index] : ""
                                        color: colors.offwhite
                                        verticalAlignment: Text.AlignVCenter
                                        horizontalAlignment: Text.AlignHCenter
                                        font: fonts.primaryFont(36, "Light")
                                        anchors.centerIn: parent
                                    }
                                }
                            }
                        }

                        Components.HapticMouseArea {
                            id: generateNewPin
                            width: pinContainer.containerHeight
                            height: pinContainer.containerHeight
                            anchors { right: parent.right; bottom: parent.bottom }

                            KeyNavigation.up: webConfiguratorSwitch
                            KeyNavigation.down: doneButton

                            function generate() {
                                Config.generateNewWebConfigPin();
                            }

                            onClicked: generateNewPin.generate()

                            Keys.onReturnPressed: {
                                generateNewPin.generate();
                                event.accepted = true;
                            }

                            onPressed: generateQrCodeIcon.color = colors.highlight
                            onReleased: generateQrCodeIcon.color = colors.light

                            Components.Icon {
                                id: generateQrCodeIcon
                                icon: "uc:arrow-rotate-left"
                                color: colors.light
                                size: 60
                                anchors.centerIn: parent

                                Behavior on color {
                                    ColorAnimation { duration: 200 }
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                radius: ui.cornerRadiusSmall
                                color: colors.transparent
                                border { width: 2; color: generateNewPin.activeFocus && ui.keyNavigationActive ? colors.highlight : colors.transparent }
                            }
                        }
                    }

                    Image {
                        id: qrCode
                        width: 220
                        height: visible ? 260 : 0
                        anchors.horizontalCenter: parent.horizontalCenter

                        fillMode: Image.PreserveAspectFit
                        verticalAlignment: Image.AlignBottom
                        antialiasing: false
                        source: "data:image/png;base64," + ui.createQrCode(("http://%1/configurator").arg(Config.webConfiguratorAddress))
                        visible: Config.webConfiguratorAddress != "" && Config.webConfiguratorEnabled
                    }
                }
            }

            Components.Button {
                id: doneButton
                text: qsTr("Done")
                x: 20
                width: parent.width - 40
                height: 80

                KeyNavigation.up: generateNewPin.visible ? generateNewPin : webConfiguratorSwitch
                trigger: function() {
                    ui.setOnboarding(false);
                    ui.showHelp = true;
                }
            }
        }
    }
}
