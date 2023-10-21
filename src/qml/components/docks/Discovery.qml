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
    footer: ui.isOnboarding ? footerItem : null

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

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    DockController.stopDiscovery();
                }
            },
            "HOME": {
                "released": function() {
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
        enabled: opacity === 1

        MouseArea {
            anchors.fill: parent
        }

        Flickable {
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

                Image {
                    Layout.fillWidth: true
                    Layout.topMargin: 20

                    fillMode: Image.PreserveAspectFit
                    antialiasing: true
                    asynchronous: true
                    cache: true
                    source: "qrc:/images/dock_setup.png"

                    Rectangle {
                        width: 6; height: 6
                        x: parent.width * 0.3; y: parent.height * 0.71
                        radius: 3
                        color: colors.orange

                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { from: 0; to: 1; duration: 1 }
                            PauseAnimation { duration: 1000 }
                            NumberAnimation { from: 1; to: 0; duration: 1 }
                            PauseAnimation { duration: 1000 }
                        }
                    }
                }

                ColumnLayout {
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
                            icon: "uc:check"
                            checked: Config.bluetoothEnabled
                            trigger: function() {
                                Config.bluetoothEnabled = !Config.bluetoothEnabled;
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true

                    color: colors.light
                    text: qsTr("Ensure the dock is nearby the remote and displaying a blinking orange light or connected via an ethernet cable. To reset, press and hold the bottom pin for over 10 seconds.")
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    font: fonts.secondaryFont(22)
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Components.Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width
                    Layout.bottomMargin: 20

                    text: qsTr("Discover docks")
                    trigger: function() {
                        startMessageContainer.opacity = 0;
                        DockController.startDiscovery();
                    }
                }

                Components.Button {
                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width
                    Layout.topMargin: -40
                    Layout.bottomMargin: 20

                    text: qsTr("Skip")
                    color: colors.secondaryButton
                    trigger: function() {
                        dockList.skip();
                    }
                    visible: ui.isOnboarding
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
        id: footerItem

        Item {
            width: ListView.view.width
            height: childrenRect.height + 20

            Components.Button {
                width: parent.width
                anchors.bottom: parent.bottom
                text: qsTr("Skip")
                color: colors.secondaryButton
                trigger: function() {
                    dockList.skip();
                }
            }
        }
    }

    Component {
        id: dockItem

        Rectangle {
            id: dockItemContainer

            x: 10
            width: ListView.view.width - 20
            height: childrenRect.height
            color: ListView.isCurrentItem ? colors.black : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                width: 1
                color: colors.medium
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
                        icon: itemDiscoveryType === "NET" ? "uc:unfolded-circle" : "uc:bluetooth"
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
                    DockController.stopDiscovery();
                    DockController.selectDockToSetup(itemId);
                }
            }
        }
    }
}
