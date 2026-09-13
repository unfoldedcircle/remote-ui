// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0
import Wifi 1.0
import Wifi.SignalStrength 1.0
import Wifi.Security 1.0

import "qrc:/components" as Components

ListView {
    id: wifiNetworkList
    width: parent.width
    clip: true
    interactive: dockNetworkSelection

    maximumFlickVelocity: 6000
    flickDeceleration: 1000
    highlightMoveDuration: 200

    header: wifiNetworkListHeader
    delegate: wifiNetwork
    footer: wifiNetworkListFooter

    signal wifiNetworkSelected(string ssid, string password)

    property QtObject popupParent: parent
    property string headerTitle: qsTr("Other Networks")
    property bool knownNetworks: false
    property bool dockNetworkSelection: false
    property string networkToJoin: ""
    property var parentObj: wifiPageContent

    function joinNetwork(network, joinLoadingAnimation) {
        wifiNetworkList.networkToJoin = network;
        joinLoadingAnimation.visible = true;
    }

    /** KEYBOARD NAVIGATION (focus chain) **/
    // The list is a link in the page's KeyNavigation chain. It moves its own selection on DPAD_UP/DOWN
    // and lets the key through at either end, so the page's KeyNavigation carries the focus on to
    // the neighbouring control. The ListView's built-in arrow handling stays off: it is gated on
    // `interactive` (false on the settings page, true for the onboarding and dock callers), and it
    // would move the selection a second time where it is on.
    keyNavigationEnabled: false
    keyNavigationWraps: false

    // lets the page's button navigation fall back to this list when the focused delegate was
    // rebuilt while a popup owned the input (see ButtonNavigation.lastFocusAnchor)
    property bool keypadFocusAnchor: true

    onActiveFocusChanged: {
        // a collapsed section can not be expanded with the keypad, so open it on arrival
        if (activeFocus && wifiNetworkList.state === "closed") {
            wifiNetworkList.state = "open";
        }
    }

    // The scan result replaces the model now and then (a network appeared or disappeared): the
    // delegates are rebuilt and the ListView resets its current index. Remember the selected network
    // and put the selection back on it.
    property string selectedIdentifier: ""

    onCurrentItemChanged: {
        if (wifiNetworkList.currentItem && wifiNetworkList.currentItem.network) {
            wifiNetworkList.selectedIdentifier = wifiNetworkList.currentItem.network.identifier;
        }
    }

    function restoreSelection() {
        if (wifiNetworkList.count === 0) {
            return;
        }

        const networks = wifiNetworkList.model;
        for (let i = 0; i < wifiNetworkList.count; i++) {
            if (networks[i] && networks[i].identifier === wifiNetworkList.selectedIdentifier) {
                wifiNetworkList.currentIndex = i;
                return;
            }
        }

        if (wifiNetworkList.currentIndex >= wifiNetworkList.count) {
            wifiNetworkList.currentIndex = wifiNetworkList.count - 1;
        }
    }

    onModelChanged: restoreSelection()
    onCountChanged: restoreSelection()

    function selectFirst() {
        wifiNetworkList.otherSelected = false;
        if (wifiNetworkList.count > 0) {
            wifiNetworkList.currentIndex = 0;
        }
    }

    function selectLast() {
        if (wifiNetworkList.hasOther) {
            wifiNetworkList.otherSelected = true;
        } else if (wifiNetworkList.count > 0) {
            wifiNetworkList.currentIndex = wifiNetworkList.count - 1;
        }
    }

    function scrollFooterIntoView() {
        if (wifiNetworkList.parentObj && typeof wifiNetworkList.parentObj.ensureVisible === "function"
                && wifiNetworkList.footerItem) {
            wifiNetworkList.parentObj.ensureVisible(wifiNetworkList.footerItem);
        }
    }

    Keys.onDownPressed: {
        if (wifiNetworkList.state === "closed") {
            wifiNetworkList.state = "open";
        }

        if (wifiNetworkList.otherSelected) {
            // leaving the list at its end: the next control in the chain starts at its beginning
            const next = wifiNetworkList.KeyNavigation.down;
            if (next && typeof next.selectFirst === "function") {
                next.selectFirst();
            }

            event.accepted = false;
            return;
        }

        if (wifiNetworkList.currentIndex < wifiNetworkList.count - 1) {
            wifiNetworkList.currentIndex++;
            event.accepted = true;
        } else if (wifiNetworkList.hasOther) {
            wifiNetworkList.otherSelected = true;
            wifiNetworkList.scrollFooterIntoView();
            event.accepted = true;
        } else {
            const next = wifiNetworkList.KeyNavigation.down;
            if (next && typeof next.selectFirst === "function") {
                next.selectFirst();
            }

            event.accepted = false;
        }
    }

    Keys.onUpPressed: {
        if (wifiNetworkList.otherSelected) {
            wifiNetworkList.otherSelected = false;
            if (wifiNetworkList.count > 0) {
                wifiNetworkList.currentIndex = wifiNetworkList.count - 1;
                event.accepted = true;
                return;
            }
        } else if (wifiNetworkList.currentIndex > 0) {
            wifiNetworkList.currentIndex--;
            event.accepted = true;
            return;
        }

        // leaving the list at its top: the previous control in the chain starts at its end
        const previous = wifiNetworkList.KeyNavigation.up;
        if (previous && typeof previous.selectLast === "function") {
            previous.selectLast();
        }

        event.accepted = false;
    }

    Keys.onReturnPressed: {
        if (wifiNetworkList.otherSelected) {
            wifiNetworkList.activateOther();
        } else {
            wifiNetworkList.selectCurrent();
        }

        event.accepted = true;
    }

    // Alternative to the focus chain for pages that drive the list through their button
    // navigation (DPAD_UP/DOWN on currentIndex, DPAD_MIDDLE on selectCurrent()): set while the
    // keypad selection is on this list to render the highlight without the keyboard focus.
    property bool keypadSelected: false
    // the "Join other" button in the footer, selected after the last network
    property bool otherSelected: false
    readonly property bool hasOther: !wifiNetworkList.knownNetworks

    function activateOther() {
        wifiSetup.open();
    }

    function selectCurrent() {
        if (wifiNetworkList.currentItem && wifiNetworkList.currentItem.network) {
            wifiNetworkList.selectNetwork(wifiNetworkList.currentItem.network,
                                          wifiNetworkList.currentItem.loadingAnimation);
        } else if (wifiNetworkList.count > 0 && wifiNetworkList.currentIndex >= 0 && !wifiNetworkList.retrySelect) {
            // the delegate is not created yet (the section is still animating open): make the view
            // create it and try once more
            wifiNetworkList.retrySelect = true;
            wifiNetworkList.positionViewAtIndex(wifiNetworkList.currentIndex, ListView.Contain);
            Qt.callLater(function() {
                wifiNetworkList.retrySelect = false;
                wifiNetworkList.selectCurrent();
            });
        }
    }

    property bool retrySelect: false

    function selectNetwork(network, loadingAnimation) {
        if (!wifiNetworkList.knownNetworks) {
            if (network.encrypted) {
                wifiPassword.start(network);
            } else if (wifiNetworkList.dockNetworkSelection) {
                wifiNetworkList.wifiNetworkSelected(network.ssid, "");
            } else {
                wifiJoin.start(network);
            }

            return;
        }

        if (network.identifier === Wifi.currentNetwork.identifier) {
            wifiInfo.showWifiInfo(network.id, Wifi.currentNetwork.ssid, network.identifier,
                                  Wifi.macAddress, Wifi.ipAddress);
            return;
        }

        popupMenu.title = network.ssid;
        let menuItems = [];
        menuItems.push({
                           //: Wifi network join
                           title: qsTr("Join and disable others"),
                           icon: "uc:wifi",
                           callback: function() {
                               wifiNetworkList.joinNetwork(network.id, loadingAnimation);
                               Wifi.connectSavedNetwork(network.id);
                               ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                           }
                       });
        menuItems.push({
                           //: Wifi network enable or disable
                           title: network.enabled ? qsTr("Disable") : qsTr("Enable"),
                           icon: network.enabled ? "uc:circle-xmark": "uc:circle-check",
                           callback: function() {
                               Wifi.enableSavedNetwork(network.id, !network.enabled);
                               ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                           }
                       });
        menuItems.push({
                           //: Wifi network delete
                           title:qsTr("Delete"),
                           icon: "uc:trash",
                           callback: function() {
                               ui.createActionableWarningNotification(qsTr("Remove WiFi network"), qsTr("Are you sure you want to remove the network %1?").arg(network.ssid), "uc:triangle-exclamation",
                                                                      function(){
                                                                          Wifi.deleteSavedNetwork(network.identifier);
                                                                          ui.setTimeOut(500, ()=>{ Wifi.getAllWifiNetworks(); });
                                                                      }, qsTr("Remove"));
                           }
                       });
        popupMenu.menuItems = menuItems;
        popupMenu.open();
    }

    populate: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 400 }
    }

    displaced: Transition {
        NumberAnimation { properties: "y"; duration: 400; easing.type: Easing.OutBounce }
    }

    state: wifiNetworkList.knownNetworks ? "open" : "closed"

    states: [
        State {
            name: "open"
            PropertyChanges {target: wifiNetworkList; height: wifiNetworkList.count * 80 + wifiNetworkList.footerItem.height + wifiNetworkList.headerItem.height + 20 }
        },
        State {
            name: "closed"
            PropertyChanges {target: wifiNetworkList; height: wifiNetworkList.headerItem.height }
        },
        State {
            name: "dock"
            PropertyChanges {target: wifiNetworkList }
        }
    ]

    transitions: [
        Transition {
            to: "open"
            PropertyAnimation { target: wifiNetworkList; properties: "height"; easing.type: Easing.OutExpo; duration: 300 }
        },
        Transition {
            to: "closed"
            PropertyAnimation { target: wifiNetworkList; properties: "height"; easing.type: Easing.OutExpo; duration: 300 }
        }
    ]

    Component {
        id: wifiNetworkListHeader

        Item {
            width: ListView.view.width
            height: headerTitleText.implicitHeight + 20

            property string headerTitle: ListView.view.headerTitle

            Text {
                id: headerTitleText
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                color: colors.light
                text: headerTitle
                anchors { left: parent.left; bottom: parent.bottom; bottomMargin: 10 }
                font: fonts.secondaryFont(24)
            }

            Image {
                visible: Wifi.scanActive && !wifiNetworkList.knownNetworks
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                source: "qrc:/images/loader_small.png"
                anchors { left: headerTitleText.right; leftMargin: 20; verticalCenter: parent.verticalCenter }

                RotationAnimation on rotation {
                    running: visible
                    loops: Animation.Infinite
                    from: 0; to: 360
                    duration: 2000
                }
            }

            Components.Icon {
                color: colors.offwhite
                icon: "uc:arrow-up"
                anchors { right: parent.right; verticalCenter: headerTitleText.verticalCenter; }
                size: 60
                rotation: wifiNetworkList.state === "open" ? 0 : 180
                visible: !wifiNetworkList.knownNetworks
                enabled: visible

                Behavior on width {
                    NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
                }

                Behavior on rotation {
                    NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
                }

                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (wifiNetworkList.state === "open") {
                            wifiNetworkList.state = "closed";
                        } else {
                            wifiNetworkList.state = "open";
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width; height: 2
                color: colors.medium
                anchors.bottom: parent.bottom
                visible: wifiNetworkList.state === "open"
            }
        }
    }

    Component {
        id: wifiNetworkListFooter

        Item {
            width: ListView.view.width
            height: wifiNetworkList.knownNetworks ? 0 : (joinOtherButton.height + (wifiNetworkList.count === 0 ? 80 : 20))

            Text {
                text: qsTr("No networks found")
                color: colors.offwhite
                anchors { top: parent.top; topMargin: 20; left: parent.left }
                font: fonts.primaryFont(30)
                visible: wifiNetworkList.count === 0
            }

            Components.Button {
                id: joinOtherButton
                width: parent.width
                //: Join other wifi network
                text: qsTr("Join other")
                highlight: wifiNetworkList.otherSelected && ui.keyNavigationActive
                trigger: function() { wifiSetup.open(); }
                anchors.bottom: parent.bottom
                visible: !wifiNetworkList.knownNetworks
                enabled: visible
            }
        }
    }

    Component {
        id: wifiNetwork

        Components.HapticMouseArea {
            id: networkDelegate
            width: ListView.view.width
            height: currentNetworkSSID.height + 40

            property var network: modelData
            property alias loadingAnimation: joinLoadingAnimation

            onClicked: {
                wifiNetworkList.currentIndex = index;
                wifiNetworkList.selectNetwork(modelData, joinLoadingAnimation);
            }

            Rectangle {
                anchors { fill: parent; margins: 2 }
                radius: ui.cornerRadiusSmall
                color: colors.transparent
                border {
                    width: 2
                    color: networkDelegate.ListView.isCurrentItem
                           && (wifiNetworkList.activeFocus || wifiNetworkList.keypadSelected)
                           && !wifiNetworkList.otherSelected
                           && ui.keyNavigationActive ? colors.highlight : colors.transparent
                }
            }

            Text {
                id: currentNetworkSSID
                width: parent.width - 80
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.offwhite
                text: modelData.ssid
                anchors { left: currentNetworkStrenght.right; leftMargin: 10; right: currentNetworkSecurity.left; top: parent.top; topMargin: 5 }
                font: fonts.primaryFont(30)
            }

            Text {
                color: colors.offwhite
                text: (wifiNetworkList.knownNetworks ? (modelData.identifier === Wifi.currentNetwork.identifier ? (Wifi.currentNetwork.frequency < 5000 ? "2.4 GHz - " : "5 GHz - ") : "") : (modelData.frequency < 5000 ? "2.4 GHz" : "5 GHz")) + (wifiNetworkList.knownNetworks ? (modelData.enabled ? "Enabled" : "Disabled") : "")
                font: fonts.secondaryFont(18)
                anchors { top: currentNetworkSSID.bottom; left: currentNetworkSSID.left }
            }

            Rectangle {
                anchors.fill: currentNetworkStrenght
                radius: 30
                color: modelData.identifier === Wifi.currentNetwork.identifier && Wifi.isConnected ? colors.green : colors.transparent
            }

            Components.Icon {
                icon: "uc:wifi"
                opacity: 0.3
                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                size: 60
            }

            Components.Icon {
                id: currentNetworkStrenght
                icon: {
                    switch (modelData.signalStrength) {
                    case SignalStrength.NONE:
                        return "";
                    case SignalStrength.WEAK:
                        return "uc:wifi-01";
                    case SignalStrength.OK:
                    case SignalStrength.GOOD:
                        return "uc:wifi-02";
                    case SignalStrength.EXCELLENT:
                        return "uc:wifi-03";
                    default:
                        return "";
                    }
                }
                opacity: icon === "" ? 0 : 1
                size: 60
                anchors { left: parent.left; verticalCenter: parent.verticalCenter }
            }

            Rectangle {
                anchors.fill: currentNetworkStrenght
                radius: 30
                color: colors.red
                visible: modelData.identifier === Wifi.currentNetwork.identifier && !Wifi.isConnected

                Components.Icon {
                    icon: "uc:xmark"
                    size: 60
                    anchors.centerIn: parent
                }
            }

            Rectangle {
                id: joinLoadingAnimation
                anchors.fill: currentNetworkStrenght
                radius: 30
                color: colors.black
                visible: false

                onVisibleChanged: {
                    if (visible == true) {
                        joinLoadingAnimationTimer.start();
                    }
                }

                Image {
                    visible: joinLoadingAnimation.visible
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/images/loader_small.png"
                    anchors.centerIn: parent

                    RotationAnimation on rotation {
                        running: visible
                        loops: Animation.Infinite
                        from: 0; to: 360
                        duration: 2000
                    }
                }

                Timer {
                    id: joinLoadingAnimationTimer
                    running: false
                    interval: 10000
                    onTriggered: joinLoadingAnimation.visible = false;
                }

                Connections {
                    target: modelData
                    ignoreUnknownSignals: true

                    function onIdChanged() {
                        if (modelData.id == wifiNetworkList.networkToJoin) {
                            joinLoadingAnimation.visible = false;
                            wifiNetworkList.networkToJoin = "";
                        }
                    }
                }
            }

            Components.Icon {
                id: currentNetworkSecurity
                icon: "uc:lock"
                size: 40
                anchors { right: parent.right; verticalCenter: currentNetworkStrenght.verticalCenter }
                visible: modelData.encrypted
            }

            Rectangle {
                width: parent.width; height: 2
                color: colors.medium
                anchors.bottom: parent.bottom
            }
        }
    }
    Components.ScrollIndicator {
        hideOverride: wifiNetworkList.count === 0 || wifiNetworkList.state === "closed"
        anchors { right: parent.right; rightMargin: 20; bottom: parent.bottom; bottomMargin: 20 + (parent.headerItem ? parent.headerItem.height : 0) }
    }

    WifiJoin {
        id: wifiJoin
        parent: wifiNetworkList.popupParent
    }

    WifiSetup {
        id: wifiSetup
        parent: wifiNetworkList.popupParent
        dockNetworkSelection: wifiNetworkList.dockNetworkSelection
    }

    WifiPassword {
        id: wifiPassword
        parent: wifiNetworkList.popupParent
        dockNetworkSelection: wifiNetworkList.dockNetworkSelection
    }

    Components.PopupMenu {
        id: popupMenu
        parent: wifiNetworkList.parentObj
    }

    Connections {
        target: wifiPassword
        ignoreUnknownSignals: true

        function onWifiNetworkSelected(ssid, password) {
            wifiNetworkList.wifiNetworkSelected(ssid, password);
        }
    }

    Connections {
        target: wifiSetup
        ignoreUnknownSignals: true

        function onWifiNetworkSelected(ssid, password) {
            wifiNetworkList.wifiNetworkSelected(ssid, password);
        }
    }
}
