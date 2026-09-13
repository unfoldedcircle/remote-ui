// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Config 1.0
import Dock.Controller 1.0

import "qrc:/components" as Components

ListView {
    id: dockList

    anchors.fill: parent
    clip: true
    spacing: 20
    model: DockController.discoveredDocks
    delegate: dockItem
    header: headerItem

    // the list starts empty and the ListView does not select the first result on its own: without
    // this the first OK after a discovery did nothing until DOWN was pressed, and nothing was outlined
    onCountChanged: {
        if (dockList.count > 0 && dockList.currentIndex < 0) {
            dockList.currentIndex = 0;
        }
    }

    add: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 300; easing.type: Easing.OutExpo }
    }

    remove: Transition {
        NumberAnimation { properties: "x"; to: ui.width; duration: 300; easing.type: Easing.OutBounce }
    }

    populate: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 300; easing.type: Easing.OutExpo }
    }

    displaced: Transition {
        NumberAnimation { properties: "x"; to: ui.width; duration: 300; easing.type: Easing.OutBounce }
    }

    signal skip()

    property alias startMessageContainer: startMessageContainer
    property alias buttonNavigation: buttonNavigation

    /** KEYPAD SELECTION **/
    // Driven by the page's button navigation (not the keyboard focus): the page moves the selection
    // with moveSelection(), activates it with activateSelection() and sets keypadSelected while the
    // selection is on this component. On the start screen the selection walks its controls, once
    // discovery runs it walks the found docks.
    property bool keypadSelected: false
    property int startSelection: 0

    function startControls() {
        let controls = [];

        if (bluetoothContainer.visible) {
            controls.push(bluetoothSwitch);
        }
        controls.push(discoverButton);

        return controls;
    }

    readonly property Item selectedStartControl: startMessageContainer.visible
                                                 ? startControls()[startSelection] : null

    // the start screen can be taller than the view: keep the selected control on screen
    onSelectedStartControlChanged: buttonNavigation.ensureVisible(selectedStartControl)

    // returns false when the selection would leave the component, so the page can move on
    function moveSelection(delta) {
        if (startMessageContainer.visible) {
            const next = dockList.startSelection + delta;
            if (next < 0 || next >= startControls().length) {
                return false;
            }

            dockList.startSelection = next;
            return true;
        }

        const nextIndex = dockList.currentIndex + delta;
        if (dockList.count === 0 || nextIndex < 0 || nextIndex >= dockList.count) {
            return false;
        }

        dockList.currentIndex = nextIndex;
        return true;
    }

    function selectLast() {
        if (startMessageContainer.visible) {
            dockList.startSelection = startControls().length - 1;
        } else if (dockList.count > 0) {
            dockList.currentIndex = dockList.count - 1;
        }
    }

    function activateSelection() {
        if (startMessageContainer.visible) {
            const control = selectedStartControl;
            if (control) {
                control.activate();
            }
            return;
        }

        if (dockList.currentItem && dockList.currentItem.dockId) {
            dockList.selectDock(dockList.currentItem.dockId);
        }
    }

    function selectDock(dockId) {
        DockController.stopDiscovery();
        DockController.selectDockToSetup(dockId);
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        scrollTarget: startFlickable
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    DockController.stopDiscovery();
                }
            },
            "HOME": {
                "pressed": function() {
                    DockController.stopDiscovery();
                }
            }
        }
    }

    Components.ScrollIndicator {
        hideOverride: dockList.atYEnd
    }

    Rectangle {
        id: startMessageContainer
        anchors.fill: parent
        color: ui.isOnboarding ? colors.black : Qt.darker(colors.dark, 1.5)
        visible: opacity > 0
        enabled: opacity === 1

        // the start screen always opens on its first control
        onVisibleChanged: dockList.startSelection = 0

        MouseArea {
            anchors.fill: parent
        }

        Flickable {
            id: startFlickable
            anchors.fill: parent
            contentHeight: startMessageContainerContent.height
            clip: true

            Behavior on contentY {
                NumberAnimation { duration: 300 }
            }

            Behavior on opacity {
                OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
            }

            ColumnLayout {
                id: startMessageContainerContent
                width: parent.width - 40
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 40

                ColumnLayout {
                    id: bluetoothContainer
                    Layout.fillWidth: true
                    spacing: 20
                    visible: !Config.bluetoothEnabled

                    Text {
                        Layout.fillWidth: true

                        color: colors.red
                        text: qsTr("Bluetooth is disabled. Discovery limited to network only.")
                        wrapMode: Text.WordWrap
                        font: fonts.secondaryFont(22)
                    }

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            Layout.fillWidth: true

                            color: colors.offwhite
                            text: qsTr("Bluetooth")
                            wrapMode: Text.WordWrap
                            font: fonts.primaryFont(30)
                        }

                        Components.Switch {
                            id: bluetoothSwitch
                            icon: "uc:check"
                            checked: Config.bluetoothEnabled
                            highlight: dockList.keypadSelected && dockList.selectedStartControl === bluetoothSwitch
                                       && ui.keyNavigationActive
                            trigger: function() {
                                Config.bluetoothEnabled = !Config.bluetoothEnabled;
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true

                    color: colors.light
                    text: qsTr("Tap discover to search for docks on your network or via Bluetooth. If you would like to wirelessly setup a new dock, make sure it’s in close proximity to the remote.")
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font: fonts.secondaryFont(22)
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Components.Button {
                    id: discoverButton
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width
                    Layout.bottomMargin: 20

                    text: qsTr("Discover")
                    highlight: dockList.keypadSelected && dockList.selectedStartControl === discoverButton
                               && ui.keyNavigationActive
                    trigger: function() {
                        startMessageContainer.opacity = 0;
                        DockController.startDiscovery();
                    }
                }
            }
        }
    }

    Component {
        id: headerItem

        Item {
            width: ListView.view.width
            height: 100

            Components.HapticMouseArea {
                width: childrenRect.width
                height: 70
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

                onClicked: {
                    DockController.startDiscovery();
                }

                Text {
                    id: headerTitle

                    color: colors.offwhite
                    opacity: 0.6
                    //: Title for searching for integrations to setup
                    text: qsTr("Discovering")
                    verticalAlignment: Text.AlignVCenter
                    anchors { left: scanLoading.right; leftMargin: 20; verticalCenter: parent.verticalCenter }
                    font: fonts.secondaryFont(24)
                }

                Image {
                    id: scanLoading

                    visible: false
                    asynchronous: true
                    fillMode: Image.PreserveAspectFit
                    source: "qrc:/images/loader_small.png"
                    anchors { left: parent.left; leftMargin: scanLoading.visible ? 0 : -scanLoading.width/2 - 10; verticalCenter: parent.verticalCenter }

                    RotationAnimation on rotation {
                        running: visible
                        loops: Animation.Infinite
                        from: 0; to: 360
                        duration: 2000
                    }
                }
            }

            Connections {
                target: DockController
                ignoreUnknownSignals: true

                function onDiscoveryStarted() {
                    scanLoading.visible = true;
                    headerTitle.text = qsTr("Discovering");
                }

                function onDiscoveryStopped() {
                    scanLoading.visible = false;
                    headerTitle.text = qsTr("%1 dock(s) found").arg(DockController.discoveredDocks.count);
                }
            }
        }
    }

    Component {
        id: dockItem

        Rectangle {
            id: dockItemContainer

            property string dockId: itemId
            readonly property bool selected: ListView.isCurrentItem && dockList.keypadSelected
                                             && !startMessageContainer.visible && ui.keyNavigationActive

            x: 10
            width: ListView.view.width - 20
            height: childrenRect.height
            color: ListView.isCurrentItem ? colors.black : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                width: dockItemContainer.selected ? 2 : 1
                color: dockItemContainer.selected ? colors.highlight : colors.medium
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
                        icon: itemDiscoveryType === "NET" ? "uc:ethernet" : "uc:bluetooth"
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
                        text: itemDiscoveryType === "NET" ? itemFriendlyName : itemId
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        font: fonts.primaryFont(30)
                    }

                    Text {
                        Layout.fillWidth: true

                        color: colors.light
                        text: itemAddress
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        font: fonts.secondaryFont(22)
                    }
                }
            }

            Components.HapticMouseArea {
                anchors.fill: parent

                onClicked: {
                    dockList.currentIndex = index;
                    dockList.selectDock(itemId);
                }
            }
        }
    }
}
