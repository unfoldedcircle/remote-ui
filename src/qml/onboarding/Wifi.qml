// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Onboarding 1.0
import Wifi 1.0

import "qrc:/components" as Components
import "qrc:/settings/settings" as Settings
import "qrc:/onboarding" as OnboardingComponents

OnboardingComponents.Page {
    id: onboardingWifiPage

    // The keypad selection walks the network list and ends on the Skip button below it; on the
    // failure screen it toggles between its two buttons. It is driven through the button
    // navigation only - no keyboard focus is involved, so one key press can not act on two
    // controls. The join popups take the input themselves.
    property bool skipSelected: false
    property bool otherSelected: false
    property bool setUpLaterSelected: false

    Component.onCompleted: {
        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         if (wifiFailed.visible || onboardingWifiPage.skipSelected) {
                                                             return;
                                                         }

                                                         if (onboardingWifiPage.otherSelected) {
                                                             onboardingWifiPage.otherSelected = false;
                                                             onboardingWifiPage.skipSelected = true;
                                                             return;
                                                         }

                                                         const next = wifiNetworkList.currentIndex + 1;
                                                         if (next >= wifiNetworkList.count) {
                                                             if (wifiNetworkList.hasOther) {
                                                                 onboardingWifiPage.otherSelected = true;
                                                             } else {
                                                                 onboardingWifiPage.skipSelected = true;
                                                             }
                                                         } else {
                                                             wifiNetworkList.currentIndex = next;
                                                         }
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         if (wifiFailed.visible) {
                                                             return;
                                                         }

                                                         if (onboardingWifiPage.skipSelected) {
                                                             onboardingWifiPage.skipSelected = false;
                                                             if (wifiNetworkList.hasOther) {
                                                                 onboardingWifiPage.otherSelected = true;
                                                             } else if (wifiNetworkList.count > 0) {
                                                                 wifiNetworkList.currentIndex = wifiNetworkList.count - 1;
                                                             }
                                                         } else if (onboardingWifiPage.otherSelected) {
                                                             onboardingWifiPage.otherSelected = false;
                                                             if (wifiNetworkList.count > 0) {
                                                                 wifiNetworkList.currentIndex = wifiNetworkList.count - 1;
                                                             }
                                                         } else if (wifiNetworkList.currentIndex > 0) {
                                                             wifiNetworkList.currentIndex--;
                                                         }
                                                     }
                                                 },
                                                 "DPAD_LEFT": {
                                                     "pressed": function() {
                                                         onboardingWifiPage.setUpLaterSelected = false;
                                                     }
                                                 },
                                                 "DPAD_RIGHT": {
                                                     "pressed": function() {
                                                         onboardingWifiPage.setUpLaterSelected = true;
                                                     }
                                                 },
                                                 "DPAD_MIDDLE": {
                                                     "pressed": function() {
                                                         if (wifiFailed.visible) {
                                                             if (onboardingWifiPage.setUpLaterSelected) {
                                                                 skipButton.activate();
                                                             } else {
                                                                 tryAgainButton.activate();
                                                             }
                                                         } else if (onboardingWifiPage.skipSelected) {
                                                             skipStepButton.activate();
                                                         } else if (onboardingWifiPage.otherSelected) {
                                                             wifiNetworkList.activateOther();
                                                         } else {
                                                             wifiNetworkList.selectCurrent();
                                                         }
                                                     }
                                                 }
                                             });
    }

    onStepEntered: {
        onboardingWifiPage.skipSelected = false;
        onboardingWifiPage.otherSelected = false;
        Wifi.getWifiStatus();
        ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
        ui.setTimeOut(1000, ()=>{ Wifi.startNetworkScan(); });
        scanTimer.start();
    }

    // Giving up on a join deletes every saved network, so it must only happen once the join
    // really is lost: on a definitive error from the remote, or after the attempt ran out of
    // time. wpa_supplicant reports DISCONNECTED while it associates with the new network, so a
    // connected(false) during an attempt is not a failure on its own - acting on it deleted the
    // very network that was still being connected.
    function connectionFailed() {
        connectionTimeoutTimer.stop();
        Wifi.deleteAllNetworks();
        loading.failure(true, function() { wifiFailed.opacity = 1; });
        Wifi.getWifiStatus();
        Wifi.startNetworkScan();
        scanTimer.start();
    }

    Connections {
        target: Wifi
        ignoreUnknownSignals: true

        function onConnecting() {
            connectionTimeoutTimer.restart();
        }

        function onConnected(success) {
            if (!success) {
                return;
            }

            connectionTimeoutTimer.stop();

            loading.success(true, function() {
                OnboardingController.nextStep();
                Wifi.stopNetworkScan();
                scanStartTimer.stop();
                scanTimer.stop();
            });
        }

        function onWrongKey() {
            if (connectionTimeoutTimer.running) {
                onboardingWifiPage.connectionFailed();
            }
        }

        function onNetworkNotFound() {
            if (connectionTimeoutTimer.running) {
                onboardingWifiPage.connectionFailed();
            }
        }
    }

    Item {
        id: title
        width: parent.width
        height: 60

        Text {
            text: qsTr("Select your WiFi network")
            width: parent.width
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(24)
        }
    }

    Item {
        id: macAddressContainer
        width: parent.width - 20
        height: 60
        anchors { top: title.bottom; horizontalCenter: parent.horizontalCenter }

        RowLayout {
            width: parent.width
            height: parent.height
            spacing: 20

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                text: qsTr("Wi-Fi address")
                wrapMode: Text.NoWrap
                elide: Text.ElideNone
                color: colors.offwhite
                font: fonts.primaryFont(20)
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                text: Wifi.macAddress
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignRight
                color: colors.offwhite
                opacity: 0.7
                font: fonts.secondaryFont(20)
            }
        }

    }

    Settings.WifiNetworkList {
        id: wifiNetworkList
        width: parent.width - 20
        anchors { top: macAddressContainer.bottom; bottom: skipStepButton.top; bottomMargin: 20; horizontalCenter: parent.horizontalCenter }
        popupParent: onboardingWifiPage
        interactive: true
        model: Wifi.networkList
        state: "open"
        parentObj: onboardingWifiPage
        keypadSelected: !onboardingWifiPage.skipSelected && !onboardingWifiPage.otherSelected && !wifiFailed.visible
        otherSelected: onboardingWifiPage.otherSelected && !wifiFailed.visible
    }

    Components.Button {
        id: skipStepButton
        width: parent.width - 20
        text: qsTr("Skip")
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        highlight: onboardingWifiPage.skipSelected && !wifiFailed.visible && ui.keyNavigationActive
        trigger: function() {
            OnboardingController.nextStep();
        }
    }

    Rectangle {
        id: wifiFailed
        color: colors.black
        anchors.fill: parent
        opacity: 0
        visible: opacity > 0
        enabled: opacity === 1

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300}
        }

        // the failure screen always opens on "Try again"
        onVisibleChanged: onboardingWifiPage.setUpLaterSelected = false

        Item {
            id: failedTitle
            width: parent.width
            height: 60

            Text {
                //: Failed to connect to a wifi network
                text: qsTr("Failed to connect")
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
            width: parent.width
            wrapMode: Text.WordWrap
            color: colors.light
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Failed to connect to the WiFi network. You can try again or proceed without setting up a WiFi network. You can set up your WiFi network later in Settings. If you skip this step, dock and integration setup won't be possible now.")
            anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
            font: fonts.secondaryFont(24)
        }

        Components.Button {
            id: skipButton
            text: qsTr("Set up later")
            width: parent.width / 2 - 10
            anchors { right: parent.right; bottom: parent.bottom }
            highlight: wifiFailed.visible && onboardingWifiPage.setUpLaterSelected && ui.keyNavigationActive
            trigger: function() {
                OnboardingController.nextStep();
                OnboardingController.nextStep();
                OnboardingController.nextStep();
            }
        }

        Components.Button {
            id: tryAgainButton
            text: qsTr("Try again")
            width: parent.width / 2 - 10
            color: colors.secondaryButton
            anchors { left: parent.left; bottom: parent.bottom }
            highlight: wifiFailed.visible && !onboardingWifiPage.setUpLaterSelected && ui.keyNavigationActive
            trigger: function() {
                wifiFailed.opacity = 0;
            }
        }
    }

    Timer {
        id: scanTimer
        repeat: true
        interval: 2000
        running: false

        onTriggered: {
            Wifi.getWifiScanStatus();

            if (!Wifi.scanActive) {
                scanStartTimer.start();
                scanTimer.stop();
            }
        }
    }

    Timer {
        id: scanStartTimer
        repeat: false
        running: false
        interval: 10000

        onTriggered: {
            Wifi.startNetworkScan();
            scanTimer.start();
        }
    }

    Timer {
        id: connectionTimeoutTimer
        repeat: false
        running: false
        // adding the network, enabling it, the WPA handshake and DHCP together regularly need
        // more than a handful of seconds - the previous 3s budget expired during every join
        interval: 30000
        onTriggered: onboardingWifiPage.connectionFailed()
    }
}
