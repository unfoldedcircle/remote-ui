// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Dock.Controller 1.0
import Wifi 1.0
import Wifi.SignalStrength 1.0
import Wifi.Security 1.0

import "qrc:/components" as Components
import "qrc:/settings/settings" as Settings

Item {
    id: dockConfigureContainer

    property QtObject dockObj: DockController.getDiscoveredDock(DockController.dockToSetup)
    property bool wifiSet: false
    property bool needsWifi: true
    property string wifiSsid
    property string wifiPassword
    property alias dockNameField: dockNameField

    signal cancelled

    function cancelSetup() {
        loading.stop();
        DockController.stopSetup(DockController.dockToSetup);
        dockConfigureContainer.cancelled();
    }

    function startDockSetup() {
        loading.start();
        if (wifiSet) {
            DockController.setupDock(DockController.dockToSetup, dockNameField.inputField.text, dockPasswordField.inputField.text, dockObj.itemDiscoveryType(), dockConfigureContainer.wifiSsid, dockConfigureContainer.wifiPassword);
        } else {
            DockController.setupDock(DockController.dockToSetup, dockNameField.inputField.text, dockPasswordField.inputField.text, dockObj.itemDiscoveryType());
        }
    }

    Component.onCompleted: {
        if (dockConfigureContainer.dockObj.itemDiscoveryType() === "NET") {
            dockConfigureContainer.needsWifi = false;
        } else {
            if (ui.isOnboarding) {
                dockConfigureContainer.wifiSsid = Wifi.getLastConnectedSsid();
                dockConfigureContainer.wifiPassword = Wifi.getLastConnectedPassword();
                dockConfigureContainer.wifiSet = true;
            }
        }
    }

    Timer {
        id: scanStartTimer
        repeat: true
        running: false
        interval: 10000
        triggeredOnStart: true

        onTriggered: {
            Wifi.getWifiScanStatus();
        }
    }

    Rectangle {
        id: dockItemContainer

        width: parent.width - 40
        height: childrenRect.height
        color: colors.dark
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        radius: ui.cornerRadiusSmall
        border {
            color: colors.medium
            width: 1
        }

        Behavior on anchors.topMargin {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
        }

        RowLayout {
            width: parent.width - 60
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Rectangle {
                Layout.preferredWidth: 60
                Layout.preferredHeight: 60
                Layout.topMargin: 30
                Layout.bottomMargin: 30

                radius: 30
                color: colors.offwhite

                Components.Icon {
                    icon: dockConfigureContainer.needsWifi ? "uc:bluetooth" : "uc:ethernet"
                    size: 60
                    color: colors.black
                    anchors.centerIn: parent
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                spacing: 0

                Text {
                    Layout.fillWidth: true

                    color: colors.offwhite
                    text: dockConfigureContainer.needsWifi ? dockConfigureContainer.dockObj.itemId() : dockConfigureContainer.dockObj.itemFriendlyName()
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    font: fonts.primaryFont(30)
                }

                Text {
                    Layout.fillWidth: true

                    color: colors.light
                    text: dockConfigureContainer.dockObj.itemAddress()
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    font: fonts.secondaryFont(22)
                }
            }
        }
    }

    SwipeView {
        id: configurationStepsSwipeView

        interactive: false
        clip: true
        anchors { top: dockItemContainer.bottom; topMargin: 20; bottom: parent.bottom; bottomMargin: 20; left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20 }

        // name
        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                Text {
                    Layout.fillWidth: true

                    color: colors.offwhite
                    text: qsTr("Name")
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    font: fonts.primaryFont(26)
                }

                Components.InputField {
                    id: dockNameField

                    Layout.fillWidth: true

                    inputField.text: dockConfigureContainer.needsWifi ? dockConfigureContainer.dockObj.itemId() : dockConfigureContainer.dockObj.itemFriendlyName()
                    inputField.onAccepted: {
                        dockConfigureContainer.startDockSetup();
                    }
                    moveInput: false
                }

                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: 20

                    color: colors.offwhite
                    text: qsTr("Password")
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    font: fonts.primaryFont(26)
                }

                Components.InputField {
                    id: dockPasswordField

                    Layout.fillWidth: true

                    inputField.placeholderText: qsTr("Optional")
                    inputField.inputMethodHints: Qt.ImhNoAutoUppercase
                    inputField.echoMode: TextInput.Password
                    inputField.passwordMaskDelay: 1000
                }

                Components.HapticMouseArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: childrenRect.height
                    Layout.topMargin: 50

                    visible: !dockConfigureContainer.wifiSet
                    onClicked: {
                        configurationStepsSwipeView.incrementCurrentIndex();
                        Wifi.startNetworkScan();
                        scanStartTimer.start();
                    }

                    ColumnLayout {
                        width: parent.width
                        spacing: 5

                        Text {
                            Layout.fillWidth: true

                            color: colors.light
                            text: dockConfigureContainer.needsWifi ? qsTr("Required") :  qsTr("Optional")
                            maximumLineCount: 1
                            elide: Text.ElideRight
                            font: fonts.secondaryFont(22)
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                Layout.fillWidth: true

                                color: colors.offwhite
                                text: qsTr("Add WiFi network")
                                maximumLineCount: 1
                                elide: Text.ElideRight
                                font: fonts.primaryFont(26)
                            }

                            Components.Icon {
                                icon: "uc:arrow-right"
                                size: 60
                                color: colors.offwhite
                            }
                        }
                    }
                }

                Components.HapticMouseArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: childrenRect.height
                    Layout.topMargin: 50

                    visible: dockConfigureContainer.wifiSet
                    onClicked: {
                        configurationStepsSwipeView.incrementCurrentIndex();
                        Wifi.startNetworkScan();
                        scanStartTimer.start();
                    }

                    ColumnLayout {
                        width: parent.width
                        spacing: 5

                        Text {
                            Layout.fillWidth: true

                            color: colors.light
                            text: qsTr("Selected WiFi network")
                            maximumLineCount: 1
                            elide: Text.ElideRight
                            font: fonts.secondaryFont(22)
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Components.Icon {
                                icon: "uc:pen-to-square"
                                size: 60
                                color: colors.offwhite
                            }

                            Text {
                                Layout.fillWidth: true

                                color: colors.offwhite
                                text: dockConfigureContainer.wifiSsid
                                maximumLineCount: 1
                                elide: Text.ElideRight
                                font: fonts.primaryFont(26)
                            }

                            Components.Icon {
                                Layout.alignment: Qt.AlignVCenter

                                icon: "uc:lock"
                                size: 40
                                color: colors.offwhite
                                visible: dockConfigureContainer.wifiPassword !== ""
                            }

                            Components.Icon {
                                icon: "uc:xmark"
                                size: 60
                                color: colors.offwhite

                                Components.HapticMouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        dockConfigureContainer.wifiSsid = "";
                                        dockConfigureContainer.wifiPassword = "";
                                        dockConfigureContainer.wifiSet = false;
                                    }
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                RowLayout {
                    Layout.fillWidth: true

                    spacing: 20

                    Components.Button {
                        Layout.preferredWidth: parent.width / 2 - 10

                        text: qsTr("Cancel")
                        color: colors.secondaryButton
                        trigger: function() {
                            dockConfigureContainer.cancelSetup();
                        }
                    }

                    Components.Button {
                        Layout.fillWidth: true

                        text: qsTr("Next")
                        opacity: enabled ? 1 : 0.3
                        enabled: dockConfigureContainer.wifiSet || !dockConfigureContainer.needsWifi
                        trigger: function() {
                            dockConfigureContainer.startDockSetup();
                        }
                    }
                }

            }
        }

        // wifi
        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    Components.Icon {
                        icon: "uc:arrow-left"
                        size: 60
                        color: colors.offwhite

                        Components.HapticMouseArea {
                            anchors.fill: parent
                            onClicked: {
                                configurationStepsSwipeView.decrementCurrentIndex();
                                scanStartTimer.stop();
                                Wifi.stopNetworkScan();
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true

                        color: colors.offwhite
                        text: qsTr("Select WiFi network")
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        font: fonts.primaryFont(26)
                    }
                }

                Settings.WifiNetworkList {
                    id: wifiNetworkList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignTop

                    popupParent: Overlay.overlay
                    model: Wifi.networkList
                    dockNetworkSelection: true
                    state: "dock"

                    Connections {
                        target: wifiNetworkList
                        ignoreUnknownSignals: true

                        function onWifiNetworkSelected(ssid, password) {
                            scanStartTimer.stop();
                            Wifi.stopNetworkScan();
                            dockConfigureContainer.wifiSet = true;
                            dockConfigureContainer.wifiSsid = ssid;
                            dockConfigureContainer.wifiPassword = password;
                            configurationStepsSwipeView.decrementCurrentIndex();
                        }
                    }
                }
            }
        }
    }
}
