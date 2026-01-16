// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Dock.Controller 1.0

import "qrc:/settings" as Settings
import "qrc:/components" as Components
import "qrc:/components/docks" as Docks

Settings.Page {
    id: docksPage

    function loadDockInfo(dockId) {
        dockDetailPopupInfo.dockId = dockId;
        dockDetailPopup.open();
    }

    Component.onCompleted: {
        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         itemList.incrementCurrentIndex();
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         itemList.decrementCurrentIndex();
                                                     }
                                                 },
                                                 "DPAD_MIDDLE": {
                                                     "pressed": function() {
                                                         loadDockInfo(itemList.currentItem.key);
                                                     }
                                                 }
                                             });
    }

    ListView {
        id: itemList
        width: parent.width
        anchors { horizontalCenter: parent.horizontalCenter; top: topNavigation.bottom; topMargin: 20; bottom: addDockSheet.top; bottomMargin: 20 }
        clip: true
        spacing: 20

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200
        pressDelay: 200

        model: DockController.configuredDocks

        delegate: listItem
        currentIndex: 0
    }

    Components.ScrollIndicator {
        parentObj: itemList
        hideOverride: itemList.atYEnd
    }

    Components.BottomSheet {
        id: addDockSheet
        titleOpened: qsTr("Add a new dock")
        titleClosed: qsTr("Add a new dock")
        openItemSource: "qrc:/components/docks/Discovery.qml"

        onOpened: {
            addDockSheet.openItem.buttonNavigation.overrideActive = true;
        }

        onClosed: {
            DockController.stopDiscovery();
            docksPage.buttonNavigation.takeControl();
        }


        Connections {
            target: DockController
            ignoreUnknownSignals: true

            function onSetupFinished(success) {
                if (success) {
                    addDockSheet.state = "closed";
                }
            }
        }
    }

    Popup {
        id: dockDetailPopup
        width: parent.width; height: parent.height
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0

        onOpened: {
            dockDetailPopupInfo.buttonNavigation.takeControl();
        }

        onClosed: {
            dockDetailPopupInfo.reset();
            dockDetailPopupInfo.buttonNavigation.releaseControl()
        }

        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.7; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        }

        exit: Transition {
            NumberAnimation { property: "scale"; from: 1.0; to: 0.7; easing.type: Easing.InExpo; duration: 300 }
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 300 }
        }

        background: Rectangle { color: colors.black; }
        contentItem: Docks.Info {
            id: dockDetailPopupInfo
            popup: dockDetailPopup
        }
    }

    Popup {
        id: dockSetupPopup
        width: parent.width; height: parent.height
        parent: Overlay.overlay
        modal: false
        closePolicy: Popup.NoAutoClose
        padding: 0

        onOpened: {
            dockSetupPopupButtonNavigation.takeControl();
        }

        onClosed: {
            dockSetupPopupButtonNavigation.releaseControl();
        }

        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.7; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 400 }
        }

        exit: Transition {
            SequentialAnimation {
                ParallelAnimation {
                    NumberAnimation { property: "scale"; from: 1.0; to: 0.7; easing.type: Easing.InExpo; duration: 300 }
                    NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 400 }
                }
                ScriptAction { script: dockSetupLoader.active = false; }
            }
        }

        background: Rectangle { color: colors.black }

        contentItem: Loader {
            id: dockSetupLoader
            active: false
            asynchronous: true
            source: "qrc:/components/docks/Setup.qml"

            onStatusChanged: {
                if (status == Loader.Ready) {
                    dockSetupPopup.open();
                }
            }

            Behavior on y {
                NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
            }

            Connections {
                target: dockSetupLoader.item
                ignoreUnknownSignals: true

                function onDone() {
                    dockSetupPopup.close();
                }

                function onFailed() {
                    dockSetupPopup.close();
                }
            }
        }

        Connections {
            target: DockController
            ignoreUnknownSignals: true

            // if a dock is selected for setup, we open the popup
            function onDockToSetupChanged(dockId) {
                if (dockId) {
                    dockSetupLoader.active = true;
                }
            }
        }

        Components.ButtonNavigation {
            id: dockSetupPopupButtonNavigation
            defaultConfig: {
                "HOME": {
                    "pressed": function() {
                        dockSetupPopup.close();
                        goHome();
                    }
                },
                "BACK": {
                    "pressed": function() {
                        dockSetupPopup.close();
                    }
                }
            }
        }
    }

    Component {
        id: listItem

        Rectangle {
            id: dockListItem
            width: ListView.view.width
            height: 300
            color: isCurrentItem && ui.keyNavigationEnabled ? Qt.darker(colors.dark, 1.5) : colors.transparent
            radius: ui.cornerRadiusSmall
            border {
                color: colors.medium
                width: 1
            }

            property bool isCurrentItem: ListView.isCurrentItem
            property string key: dockId
            property alias identifyAnimation: identifyAnimation

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    itemList.currentIndex = index;
                    docksPage.loadDockInfo(dockId);
                }
            }

            Image {
                fillMode: Image.PreserveAspectFit
                antialiasing: true
                asynchronous: true
                cache: true
                source: {
                    const model = dockModel.toUpperCase();
                    const serial = dockSerial.toUpperCase();

                    if (model == "UCD2") {
                        return "qrc:/images/dock_2.png"
                    } else if (model == "UCD3") {
                        if (serial.length >= 2) {
                            const dockType = serial[serial.length - 1];
                            const dockColor = serial[serial.length - 2];

                            if (dockType == "C" && dockColor == "D") {
                                return "qrc:/images/dock3_dark_charging.png"
                            } else if (dockType == "N" && dockColor == "D") {
                                return "qrc:/images/dock3_dark_non_charging.png"
                            } else if (dockType == "C" && dockColor == "S") {
                                return "qrc:/images/dock3_silver_charging.png"
                            } else if (dockType == "N" && dockColor == "S") {
                                return "qrc:/images/dock3_silver_non_charging.png"
                            } else {
                                return "qrc:/images/dock3_dark_charging.png"
                            }
                        } else {
                            return "qrc:/images/dock3_dark_charging.png"
                        }
                    } else {
                        return "qrc:/images/dock3_dark_charging.png"
                    }
                }

                anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: -80 }
                opacity: dockState === DockStates.ACTIVE || dockState === DockStates.IDLE ? 0.8 : 0.25

                Behavior on opacity {
                    OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
                }

                Rectangle {
                    width: 6; height: 6
                    x: parent.width * 0.24; y: parent.height * 0.7
                    radius: 3
                    color: dockLearningActive ? colors.green : colors.transparent

                    SequentialAnimation on color {
                        id: identifyAnimation
                        loops: 2
                        running: false

                        ColorAnimation { from: colors.transparent; to: colors.blue; duration: 100 }
                        PauseAnimation { duration: 100 }
                        ColorAnimation { from: colors.blue; to: colors.transparent; duration: 100 }
                        PauseAnimation { duration: 100 }

                        ColorAnimation { from: colors.transparent; to: colors.orange; duration: 100 }
                        PauseAnimation { duration: 100 }
                        ColorAnimation { from: colors.orange; to: colors.transparent; duration: 100 }
                        PauseAnimation { duration: 100 }

                        ColorAnimation { from: colors.transparent; to: colors.green; duration: 100 }
                        PauseAnimation { duration: 100 }
                        ColorAnimation { from: colors.green; to: colors.transparent; duration: 100 }
                        PauseAnimation { duration: 100 }

                        ColorAnimation { from: colors.transparent; to: colors.red; duration: 100 }
                        PauseAnimation { duration: 100 }
                        ColorAnimation { from: colors.red; to: colors.transparent; duration: 100 }
                        PauseAnimation { duration: 100 }
                    }
                }
            }

            ColumnLayout {
                id: mainColumnLayout
                anchors { top: parent.top; topMargin: 30; bottom: parent.bottom; bottomMargin: 30; left: parent.left; leftMargin: 30; right: parent.right; rightMargin: 30 }
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 20

                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: 5

                        width: 30
                        height: width
                        color: colors.offwhite
                        radius: width / 2

                        Components.Icon {
                            icon: dockState === DockStates.ERROR ? "uc:xmark" : "uc:check"
                            size: 26
                            color: colors.black
                            anchors.centerIn: parent
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true

                        text: dockName
                        textFormat: Text.RichText
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        color: colors.offwhite
                        font: fonts.primaryFont(30)
                    }
                }

                Text {
                    Layout.fillWidth: true

                    text: {
                        switch (dockState) {
                        case DockStates.ACTIVE:
                            return qsTr("Active");
                        case DockStates.CONNECTING:
                            return qsTr("Connecting");
                        case DockStates.ERROR:
                            return qsTr("Error");
                        case DockStates.IDLE:
                            return qsTr("Idle");
                        case DockStates.RECONNECTING:
                            return qsTr("Reconnecting");
                        }
                    }

                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.light
                    font: fonts.secondaryFont(20)
                }

                Text {
                    Layout.fillWidth: true

                    text: qsTr("Something is wrong")
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.red
                    font: fonts.secondaryFont(20)
                    visible: dockState === DockStates.ERROR
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Components.Button {
                    text: qsTr("Identify")
                    fontSize: 20
                    height: 50
                    color: colors.medium
                    visible: dockState === DockStates.ACTIVE || dockState === DockStates.IDLE
                    trigger: function() {
                        DockController.identify(dockId);
                        identifyAnimation.start();
                    }
                }

                Components.Button {
                    text: qsTr("Connect")
                    fontSize: 20
                    height: 50
                    color: colors.medium
                    visible: dockState === DockStates.ERROR
                    trigger: function() {
                        DockController.connect(dockId);
                    }
                }
            }
        }
    }
}
