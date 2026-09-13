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
    signal home

    function cancelSetup() {
        loading.stop();
        DockController.stopSetup(DockController.dockToSetup);
        dockConfigureContainer.cancelled();
    }

    function leaveWifiPage() {
        // the user lands on Next when coming back with a network
        dockConfigureContainer.pageZeroFocus = nextButton;
        configurationStepsSwipeView.decrementCurrentIndex();
        scanStartTimer.stop();
        Wifi.stopNetworkScan();
    }

    function openWifiPage() {
        configurationStepsSwipeView.incrementCurrentIndex();
        Wifi.startNetworkScan();
        scanStartTimer.start();
    }

    /** KEYBOARD NAVIGATION **/
    // The form navigates through the QML focus chain: name -> password -> WiFi row -> Next, LEFT
    // to Cancel; on the WiFi page the back arrow and the network list. The step owns the input
    // while it is the current step of the setup; taking it is deferred, as the step change is
    // triggered by a key press that is still being delivered.
    readonly property bool isCurrentStep: SwipeView.isCurrentItem
    onIsCurrentStepChanged: Qt.callLater(activate)

    function activate() {
        if (dockConfigureContainer.isCurrentStep) {
            buttonNavigation.lastFocusItem = null;
            buttonNavigation.lastFocusAnchor = null;
            buttonNavigation.takeControl();
        } else {
            buttonNavigation.releaseControl();
        }
    }

    property Item pageZeroFocus: null
    readonly property Item visibleWifiRow: dockConfigureContainer.wifiSet ? selectedWifiRow : addWifiRow

    // The focus stays on the previous page after a page change: it is still visible in the swipe
    // view and counts as a control of this scope. Move it to the page's first control.
    //
    // The page can change while a popup still owns the input: the network password / join popups
    // emit their result before they close, and only release the input once they have faded out.
    // The hand-over is then kept pending and applied the moment the input comes back - if it were
    // dropped, the claim on regaining the input would restore the network list on the hidden page
    // and the next OK would reopen the popup from there.
    property bool pageFocusPending: false

    function focusPage() {
        buttonNavigation.lastFocusItem = null;
        buttonNavigation.lastFocusAnchor = null;

        if (!buttonNavigation.hasInputControl) {
            dockConfigureContainer.pageFocusPending = true;
            return;
        }

        dockConfigureContainer.pageFocusPending = false;
        let target = buttonNavigation.initialFocusItem;
        if (configurationStepsSwipeView.currentIndex === 0 && dockConfigureContainer.pageZeroFocus) {
            target = dockConfigureContainer.pageZeroFocus;
            dockConfigureContainer.pageZeroFocus = null;
        }

        if (target) {
            target.forceActiveFocus();
        }
    }

    Connections {
        target: buttonNavigation

        // Synchronous on purpose: the input comes back from the popup's closed signal, long after
        // the key that closed it, and the button navigation's own (deferred) claim then finds the
        // focus already on the right control and leaves it there.
        function onHasInputControlChanged() {
            if (buttonNavigation.hasInputControl && dockConfigureContainer.pageFocusPending) {
                dockConfigureContainer.focusPage();
            }
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        manageFocus: true
        initialFocusItem: configurationStepsSwipeView.currentIndex === 0 ? dockNameField.inputField : wifiNetworkList
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    if (configurationStepsSwipeView.currentIndex > 0) {
                        dockConfigureContainer.leaveWifiPage();
                    } else {
                        dockConfigureContainer.cancelSetup();
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    dockConfigureContainer.cancelSetup();
                    dockConfigureContainer.home();
                }
            }
        }
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

        onCurrentIndexChanged: Qt.callLater(dockConfigureContainer.focusPage)
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
                    // Return moves on to the password: submitting from here skipped the WiFi
                    // requirement that Next enforces
                    inputField.onAccepted: {
                        dockPasswordField.inputField.forceActiveFocus();
                    }
                    moveInput: false
                    keyboardFollowsFocus: true
                    navDown: dockPasswordField.inputField
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
                    inputField.onAccepted: {
                        dockConfigureContainer.visibleWifiRow.forceActiveFocus();
                    }
                    moveInput: false
                    keyboardFollowsFocus: true
                    navUp: dockNameField.inputField
                    navDown: addWifiRow
                }

                Components.HapticMouseArea {
                    id: addWifiRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: childrenRect.height
                    Layout.topMargin: 50
                    visible: !dockConfigureContainer.wifiSet
                    keypadActivatable: true
                    KeyNavigation.up: dockPasswordField.inputField
                    KeyNavigation.down: selectedWifiRow
                    onActiveFocusChanged: if (activeFocus && keyboard.active) keyboard.hide()
                    onClicked: {
                        dockConfigureContainer.openWifiPage();
                    }

                    Components.RowHighlight { }

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
                    id: selectedWifiRow
                    Layout.fillWidth: true
                    Layout.preferredHeight: childrenRect.height
                    Layout.topMargin: 50
                    visible: dockConfigureContainer.wifiSet
                    keypadActivatable: true
                    KeyNavigation.up: addWifiRow
                    KeyNavigation.right: clearWifiButton
                    KeyNavigation.down: nextButton.enabled ? nextButton : cancelButton
                    onActiveFocusChanged: if (activeFocus && keyboard.active) keyboard.hide()
                    onClicked: {
                        dockConfigureContainer.openWifiPage();
                    }

                    Components.RowHighlight { }

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

                                Rectangle {
                                    anchors { fill: parent; margins: 4 }
                                    radius: ui.cornerRadiusSmall
                                    color: colors.transparent
                                    border {
                                        width: 2
                                        color: clearWifiButton.activeFocus && ui.keyNavigationActive ? colors.highlight : colors.transparent
                                    }
                                }

                                Components.HapticMouseArea {
                                    id: clearWifiButton
                                    anchors.fill: parent
                                    keypadActivatable: true
                                    KeyNavigation.left: selectedWifiRow
                                    KeyNavigation.up: dockPasswordField.inputField
                                    KeyNavigation.down: nextButton.enabled ? nextButton : cancelButton
                                    onClicked: {
                                        dockConfigureContainer.wifiSsid = "";
                                        dockConfigureContainer.wifiPassword = "";
                                        dockConfigureContainer.wifiSet = false;
                                        addWifiRow.forceActiveFocus();
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
                        id: cancelButton
                        Layout.preferredWidth: parent.width / 2 - 10
                        text: qsTr("Cancel")
                        color: colors.secondaryButton
                        trigger: function() {
                            dockConfigureContainer.cancelSetup();
                        }

                        KeyNavigation.up: dockConfigureContainer.visibleWifiRow
                        KeyNavigation.right: nextButton
                    }

                    Components.Button {
                        id: nextButton
                        Layout.fillWidth: true
                        text: qsTr("Next")
                        opacity: enabled ? 1 : 0.3
                        enabled: dockConfigureContainer.wifiSet || !dockConfigureContainer.needsWifi
                        trigger: function() {
                            dockConfigureContainer.startDockSetup();
                        }

                        KeyNavigation.up: dockConfigureContainer.visibleWifiRow
                        KeyNavigation.left: cancelButton
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

                        Rectangle {
                            anchors { fill: parent; margins: 4 }
                            radius: ui.cornerRadiusSmall
                            color: colors.transparent
                            border {
                                width: 2
                                color: wifiBackButton.activeFocus && ui.keyNavigationActive ? colors.highlight : colors.transparent
                            }
                        }

                        Components.HapticMouseArea {
                            id: wifiBackButton
                            anchors.fill: parent
                            keypadActivatable: true
                            KeyNavigation.down: wifiNetworkList
                            onClicked: {
                                dockConfigureContainer.leaveWifiPage();
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
                    parentObj: dockConfigureContainer
                    model: Wifi.networkList
                    dockNetworkSelection: true
                    state: "dock"

                    KeyNavigation.up: wifiBackButton

                    Connections {
                        target: wifiNetworkList
                        ignoreUnknownSignals: true

                        function onWifiNetworkSelected(ssid, password) {
                            dockConfigureContainer.wifiSet = true;
                            dockConfigureContainer.wifiSsid = ssid;
                            dockConfigureContainer.wifiPassword = password;
                            dockConfigureContainer.leaveWifiPage();
                        }
                    }
                }
            }
        }
    }
}
