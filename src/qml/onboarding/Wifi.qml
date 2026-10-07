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

    Components.TitleBar {
        id: title
        // onboarding steps have no back target: BACK goes to the previous step
        action: ""
        text: qsTr("Select your WiFi network")
    }

    Item {
        id: macAddressContainer
        width: parent.width - 40
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
                color: colors.textSecondary
                font: fonts.help()
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                text: Wifi.macAddress
                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignRight
                color: colors.textPrimary
                font: fonts.value()
            }
        }

    }

    Settings.WifiNetworkList {
        id: wifiNetworkList
        width: parent.width
        anchors { top: macAddressContainer.bottom; bottom: skipStepButton.top; bottomMargin: 20; horizontalCenter: parent.horizontalCenter }
        popupParent: onboardingWifiPage
        interactive: true
        model: Wifi.networkList
        state: "open"
        parentObj: onboardingWifiPage
        keypadSelected: !onboardingWifiPage.skipSelected && !onboardingWifiPage.otherSelected && !wifiFailed.visible
        otherSelected: onboardingWifiPage.otherSelected && !wifiFailed.visible
    }

    Components.ScrollIndicator {
        parentObj: wifiNetworkList
    }

    // Skip is the alternative to joining a network: the secondary variant, full width in the gutter
    Components.Button {
        id: skipStepButton
        width: parent.width - 40
        text: qsTr("Skip")
        variant: "secondary"
        anchors { bottom: parent.bottom; bottomMargin: 20; horizontalCenter: parent.horizontalCenter }
        highlight: onboardingWifiPage.skipSelected && !wifiFailed.visible && ui.keyNavigationActive
        trigger: function() {
            OnboardingController.nextStep();
        }
    }

    Rectangle {
        id: wifiFailed
        color: colors.bg
        anchors.fill: parent
        opacity: 0
        visible: opacity > 0
        enabled: opacity === 1

        Behavior on opacity {
            OpacityAnimator { easing.type: Easing.OutExpo; duration: 300}
        }

        // the failure screen always opens on "Try again"
        onVisibleChanged: onboardingWifiPage.setUpLaterSelected = false

        Components.TitleBar {
            id: failedTitle
            action: ""
            //: Failed to connect to a wifi network
            text: qsTr("Failed to connect")
        }

        Text {
            id: description
            width: parent.width - 40
            wrapMode: Text.WordWrap
            color: colors.textPrimary
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Failed to connect to the WiFi network. You can try again or proceed without setting up a WiFi network. You can set up your WiFi network later in Settings. If you skip this step, dock and integration setup won't be possible now.")
            anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
            font: fonts.prose()
            lineHeight: fonts.proseLineHeight
        }

        Components.Button {
            id: skipButton
            text: qsTr("Set up later")
            width: (parent.width - 60) / 2
            anchors { right: parent.right; rightMargin: 20; bottom: parent.bottom; bottomMargin: 20 }
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
            width: (parent.width - 60) / 2
            variant: "secondary"
            anchors { left: parent.left; leftMargin: 20; bottom: parent.bottom; bottomMargin: 20 }
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
