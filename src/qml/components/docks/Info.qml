// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import Dock.Controller 1.0

import "qrc:/components" as Components
import "qrc:/components/docks" as Docks

Item {
    id: dockInfoContainer
    width: parent.width
    height: parent.height

    property QtObject dockObjDummy: QtObject {
        property string id
        property string name
        property int state
        property string connectionType
        property int ledBrightness
    }

    property string dockId
    property QtObject popup
    property QtObject dockObj: dockObjDummy
    property bool dockEditable: dockInfoContainer.dockObj.state === DockStates.ACTIVE || dockInfoContainer.dockObj.state === DockStates.IDLE
    property alias buttonNavigation: buttonNavigation

    onDockIdChanged: {
        if (dockId) {
            DockController.getConfiguredDockFromCore(dockId);
            connectSignalSlot(DockController.gotDock, function(success, dockIdFromCore) {
                dockObj = DockController.getConfiguredDock(dockIdFromCore);
                if (!success) {
                    ui.createNotification("There was an error while getting the latest dock data", true);
                }
            });
        } else {
            dockObj = dockObjDummy;
        }
    }

    function reset() {
        dockInfoFlickable.contentY = 0;
        deleteContainer.state = "closed";
        dockId = "";
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        overrideActive: ui.inputController.activeObject === String(popup)
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    dockInfoContainer.popup.close()
                }
            },
            "HOME": {
                "pressed": function() {
                    dockInfoContainer.popup.close()
                }
            }
        }
    }

    Flickable {
        id: dockInfoFlickable
        width: parent.width
        height: parent.height
        contentHeight: content.childrenRect.height + deleteContainer.height

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        Behavior on contentY {
            NumberAnimation { duration: 300 }
        }

        ColumnLayout {
            id: content
            spacing: 0
            x: 10
            width: dockInfoFlickable.width - 20

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height

                Image {
                    anchors { top: parent.top; topMargin: 100; horizontalCenter: parent.horizontalCenter }
                    fillMode: Image.PreserveAspectFit
                    antialiasing: true
                    asynchronous: true
                    cache: true
                    source: {
                        const model = dockInfoContainer.dockObj.model.toUpperCase();
                        const serial = dockInfoContainer.dockObj.serial.toUpperCase();

                        if (model == "UCR2") {
                            return "qrc:/images/dock_2.png"
                        } else if (model == "UCR3") {
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
                    opacity: dockInfoContainer.dockObj.state === DockStates.ACTIVE || dockInfoContainer.dockObj.state === DockStates.IDLE ? 0.8 : 0.25
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

                    Behavior on opacity {
                        OpacityAnimator { easing.type: Easing.OutExpo; duration: 300 }
                    }

                    Rectangle {
                        width: 6; height: 6
                        x: parent.width * 0.24; y: parent.height * 0.7
                        radius: 3
                        color: dockObj.learningActive ? colors.green : colors.transparent

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
                    width: parent.width
                    height: parent.height
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true

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
                                    icon: dockInfoContainer.dockObj.state === DockStates.ERROR ? "uc:xmark" : "uc:check"
                                    size: 26
                                    color: colors.black
                                    anchors.centerIn: parent
                                }
                            }

                            Text {
                                Layout.alignment: Qt.AlignTop
                                Layout.fillWidth: true

                                text: dockInfoContainer.dockObj.name
                                textFormat: Text.RichText
                                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                elide: Text.ElideRight
                                maximumLineCount: 2
                                color: colors.offwhite
                                font: fonts.primaryFont(30)

                                Components.HapticMouseArea {
                                    anchors.fill: parent
                                    enabled: dockInfoContainer.dockEditable
                                    onClicked: {
                                        renameContainer.open(dockInfoContainer.dockObj.id, dockInfoContainer.dockObj.name);
                                    }
                                }
                            }
                        }

                        Components.Icon {
                            Layout.rightMargin: -20

                            color: colors.offwhite
                            size: 80
                            icon: "uc:xmark"

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    dockInfoContainer.popup.close();
                                }
                            }
                        }
                    }

                    Text {
                        Layout.topMargin: -10
                        Layout.fillWidth: true

                        text: qsTr("Tap to edit name")
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        color: colors.light
                        font: fonts.secondaryFont(20)
                        visible: dockInfoContainer.dockEditable

                        Components.HapticMouseArea {
                            anchors.fill: parent
                            onClicked: {
                                renameContainer.open(dockInfoContainer.dockObj.id, dockInfoContainer.dockObj.name);
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true

                        text: qsTr("Something is wrong")
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        color: colors.red
                        font: fonts.secondaryFont(20)
                        visible: dockInfoContainer.dockObj.state === DockStates.ERROR
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    Components.Button {
                        Layout.alignment: Qt.AlignRight

                        text: qsTr("Identify")
                        fontSize: 20
                        height: 50
                        color: colors.medium
                        visible: dockInfoContainer.dockEditable
                        trigger: function() {
                            DockController.identify(dockInfoContainer.dockObj.id);
                            identifyAnimation.start();
                        }
                    }

                    Components.Button {
                        text: qsTr("Connect")
                        fontSize: 20
                        height: 50
                        color: colors.medium
                        visible: dockInfoContainer.dockObj.state === DockStates.ERROR
                        trigger: function() {
                            DockController.connect(dockInfoContainer.dockObj.id);
                        }
                    }
                }


            }

            Components.AboutInfo {
                Layout.topMargin: 20
                Layout.bottomMargin: 10

                key: qsTr("State")
                value: {
                    switch (dockObj.state) {
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
            }

            Components.AboutInfo {
                Layout.bottomMargin: 10

                key: qsTr("Connection type")
                value: dockObj.connectionType ? dockObj.connectionType : qsTranslate("Abbreviation for not available", "N/A")
            }

            Components.AboutInfo {
                Layout.bottomMargin: 10

                key: qsTr("Service name")
                value: dockObj.id
                multiline: true
            }

            Components.AboutInfo {
                Layout.bottomMargin: 10

                key: qsTr("Custom IP or URL")
                value: dockObj.customWsUrl ? dockObj.customWsUrl : qsTr("Not set")
                multiline: true
            }


            Components.AboutInfo {
                Layout.bottomMargin: 10

                key: qsTr("Firmware version")
                value: dockObj.version ? dockObj.version : qsTranslate("Abbreviation for not available", "N/A")
//                lineBottom: false
            }

//            Components.Button {
//                Layout.fillWidth: true

//                text: qsTr("Check for updates")
//                trigger: function() {
//                    console.debug("Check for dock update")
//                }
//            }

            // TODO(marton): add firmware update section

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.bottomMargin: 20

                opacity: dockInfoContainer.dockEditable ? 1 : 0.3
                enabled: opacity === 1

                Text {
                    Layout.fillWidth: true

                    text: qsTr("Led brightness")
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.offwhite
                    font: fonts.primaryFont(30)
                }

                Components.Slider {
                    height: 60
                    from: 0
                    to: 100
                    stepSize: 1
                    value: dockObj.ledBrightness
                    live: true

                    onUserInteractionEnded: {
                        DockController.setDockLedBrightness(dockObj.id, value);
                    }
                }
            }

            Components.HapticMouseArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                opacity: dockInfoContainer.dockEditable ? 1 : 0.3
                enabled: opacity === 1
                onClicked: {
                    passwordChangeContainter.open(dockInfoContainer.dockObj.id);
                }

                Text {
                    width: parent.width
                    anchors.centerIn: parent

                    text: qsTr("Change password")
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.offwhite
                    font: fonts.primaryFont(30)
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: colors.medium
                    anchors.top: parent.top
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: colors.medium
                    anchors.bottom: parent.bottom
                }
            }

            Components.HapticMouseArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                opacity: dockInfoContainer.dockEditable ? 1 : 0.3
                enabled: opacity === 1
                onClicked: {
                    ui.createNotification("Not implemented yet");
                }

                Text {
                    width: parent.width
                    anchors.centerIn: parent

                    text: qsTr("Change WiFi settings")
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.offwhite
                    font: fonts.primaryFont(30)
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: colors.medium
                    anchors.bottom: parent.bottom
                }
            }

            Components.HapticMouseArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                opacity: dockInfoContainer.dockEditable ? 1 : 0.3
                enabled: opacity === 1
                onClicked: {
                    ui.createActionableWarningNotification(qsTr("Factory reset"),
                                                           qsTr("Are you sure you want to factory reset %1?").arg(dockObj.name),
                                                           "uc:triangle-exclamation",
                                                           function(){
                                                               DockController.factoryReset(dockObj.id);
                                                               dockInfoContainer.reset();
                                                               popup.close();
                                                           }, qsTr("Reset"));
                }

                Text {
                    width: parent.width
                    anchors.centerIn: parent

                    text: qsTr("Factory reset")
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.offwhite
                    font: fonts.primaryFont(30)
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
            }
        }

        Item {
            id: deleteContainer
            width: dockInfoFlickable.width
            height: 100
            anchors.top: content.bottom
            state: "closed"

            onStateChanged: {
                if (state == "opened") {
                    deleteContainerButtonNavigation.takeControl();
                } else {
                    deleteContainerButtonNavigation.releaseControl();
                }
            }

            Components.ButtonNavigation {
                id: deleteContainerButtonNavigation
                defaultConfig: {
                    "BACK": {
                        "released": function() {
                            deleteContainer.state = "closed";
                        }
                    },
                    "HOME": {
                        "released": function() {
                            deleteContainer.state = "closed";
                        }
                    },
                    "DPAD_MIDDLE": {
                        "released": function() {
                            deleteContainer.deleteDock();
                        }
                    }
                }
            }

            function deleteDock() {
                DockController.deleteDock(dockObj.id);
                dockInfoContainer.reset();
                popup.close();
            }

            states: [
                State {
                    name: "closed"
                    PropertyChanges { target: deleteContainerContent; y: 0 }
                    PropertyChanges { target: openIcon; opacity: 1 }
                    PropertyChanges { target: deleteConfirmText; opacity: 0 }
                    PropertyChanges { target: deleteConfirmButtons; opacity: 0 }
                    PropertyChanges { target: blockOutOverlay; opacity: 0 }
                    PropertyChanges { target: dockInfoFlickable; interactive: true }
                },
                State {
                    name: "opened"
                    PropertyChanges { target: deleteContainerContent; y: -deleteContainerContent.height + deleteContainer.height + ui.cornerRadiusLarge }
                    PropertyChanges { target: openIcon; opacity: 0 }
                    PropertyChanges { target: deleteConfirmText; opacity: 1 }
                    PropertyChanges { target: deleteConfirmButtons; opacity: 1 }
                    PropertyChanges { target: blockOutOverlay; opacity: 0.8 }
                    PropertyChanges { target: dockInfoFlickable; interactive: false }
                }
            ]

            transitions: [
                Transition {
                    to: "closed"
                    ParallelAnimation {
                        PropertyAnimation { target: deleteContainerContent; properties: "y"; easing.type: Easing.OutExpo; duration: 300 }
                        PropertyAnimation { target: openIcon; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        PropertyAnimation { target: deleteConfirmText; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        PropertyAnimation { target: deleteConfirmButtons; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        PropertyAnimation { target: blockOutOverlay; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                    }
                },
                Transition {
                    to: "opened"

                    ParallelAnimation {
                        ScriptAction  { script: dockInfoFlickable.contentY = (content.childrenRect.height + deleteContainer.height) - dockInfoFlickable.height }
                        PropertyAnimation { target: deleteContainerContent; properties: "y"; easing.type: Easing.OutExpo; duration: 300 }
                        PropertyAnimation { target: openIcon; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        PropertyAnimation { target: deleteConfirmText; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        PropertyAnimation { target: deleteConfirmButtons; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        SequentialAnimation {
                            PauseAnimation { duration: 100 }
                            PropertyAnimation { target: blockOutOverlay; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                        }
                    }
                }
            ]

            Rectangle {
                id: blockOutOverlay

                width: parent.width
                height: ui.height - deleteContainerContent.height + ui.cornerRadiusLarge
                color: colors.black
                anchors.bottom: deleteContainerContent.top
                enabled: opacity != 0

                Components.Icon {
                    icon: "uc:trash"
                    size: 200
                    color: colors.red
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                }
            }

            Rectangle {
                id: deleteContainerContent
                width: parent.width
                height: childrenRect.height + ui.cornerRadiusLarge
                color: Qt.darker(colors.red, 1.3)
                radius: ui.cornerRadiusLarge

                ColumnLayout {
                    width: parent.width
                    spacing: 0

                    RowLayout {
                        Layout.leftMargin: 20
                        spacing: 20

                        Text {
                            Layout.fillWidth: true

                            text: qsTr("Delete dock")
                            elide: Text.ElideRight
                            maximumLineCount: 1
                            color: colors.offwhite
                            font: fonts.secondaryFont(28)
                        }

                        Components.Icon {
                            id: openIcon

                            Layout.alignment: Qt.AlignRight

                            icon: "uc:xmark"
                            size: 100
                            color: colors.offwhite

                            Components.HapticMouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    deleteContainer.state = "opened";
                                }

                                enabled: openIcon.opacity == 1
                            }
                        }
                    }

                    Text {
                        id: deleteConfirmText

                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20

                        text: qsTr("Are you sure you want to delete %1?").arg(dockObj.name)
                        wrapMode: Text.WordWrap
                        color: colors.offwhite
                        font: fonts.secondaryFont(24)
                    }

                    RowLayout {
                        id: deleteConfirmButtons

                        Layout.fillWidth: true
                        Layout.topMargin: 40
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        Layout.bottomMargin: 20

                        Text {
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight

                            text: qsTr("Cancel")
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                            color: colors.offwhite
                            font: fonts.secondaryFont(26, "Bold")

                            Components.HapticMouseArea {
                                width: parent.width + 40
                                height: parent.width + 40
                                anchors.centerIn: parent
                                onClicked: {
                                    deleteContainer.state = "closed";
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.preferredHeight: implicitHeight

                            text: qsTr("Delete")
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignRight
                            color: colors.offwhite
                            font: fonts.secondaryFont(26, "Bold")

                            Components.HapticMouseArea {
                                width: parent.width + 40
                                height: parent.width + 40
                                anchors.centerIn: parent
                                onClicked: {
                                    deleteContainer.deleteDock();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Docks.Rename {
        id: renameContainer
    }

    Docks.PasswordChange {
        id: passwordChangeContainter
    }
}
