// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import Integration.Controller 1.0
import Entity.Controller 1.0

import "qrc:/components" as Components
import "qrc:/components/integrations" as Integrations

Flickable {
    id: integrationInfoFlickable
    contentHeight: content.childrenRect.height + deleteContainer.height

    maximumFlickVelocity: 6000
    flickDeceleration: 1000
    pressDelay: 200

    Behavior on contentY {
        NumberAnimation { duration: 300 }
    }

    property QtObject integrationObjDummy: QtObject {
        property string id
        property string name
        property string icon
        property string state
        property bool enabled
    }

    property QtObject integrationDriverObjDummy: QtObject {
        property string name
        property string icon
        property string state
        property string version
        property string developerName
        property string homepage
        property string description
    }

    property string integrationId
    property QtObject popup
    property QtObject integrationObj: integrationObjDummy
    property QtObject integrationDriverObj: integrationDriverObjDummy
    property alias buttonNavigation: buttonNavigation

    onIntegrationIdChanged: {
        manageEntitiesOpenedElements.integrationId = integrationId;

        if (integrationId) {
            EntityController.loadConfiguredEntities(integrationId);
            integrationObj = IntegrationController.getModelItem(integrationId);
            integrationDriverObj = IntegrationController.getDriversModelItem(integrationObj.driverId);
        } else {
            integrationObj = integrationObjDummy;
            integrationDriverObj = integrationDriverObjDummy;
        }
    }

    function reset() {
        integrationInfoFlickable.contentY = 0;
        deleteContainer.state = "closed";
        integrationId = "";
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        overrideActive: ui.inputController.activeObject === String(popup)
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    integrationInfoFlickable.popup.close()
                }
            },
            "HOME": {
                "pressed": function() {
                    integrationInfoFlickable.popup.close()
                }
            }
        }
    }

    ColumnLayout {
        id: content
        spacing: 0
        x: 10
        width: integrationInfoFlickable.width - 20

        RowLayout {
            Layout.topMargin: 20

            Rectangle {
                Layout.alignment: Qt.AlignTop

                width: 80
                height: width
                radius: 40
                color: integrationObj.icon.includes("uc") ? colors.offwhite : colors.transparent

                Components.Icon {
                    color: colors.black
                    icon: integrationObj.icon === "" ? "uc:puzzle" : integrationObj.icon
                    size: 80
                    anchors.centerIn: parent
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Components.Icon {
                Layout.rightMargin: -20

                color: colors.offwhite
                size: 80
                icon: "uc:xmark"

                Components.HapticMouseArea {
                    anchors.fill: parent
                    onClicked: {
                        integrationInfoFlickable.popup.close();
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 20

            text: integrationObj.name
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.offwhite
            font: fonts.primaryFont(50)
        }

        Text {
            Layout.fillWidth: true

            text: integrationDriverObj.external ? qsTr("External integration") : qsTr("Local integration")
            maximumLineCount: 1
            color: colors.light
            font: fonts.secondaryFont(24)
        }

        Item {
            id: manageEntitiesContainer

            Layout.topMargin: 20
            Layout.fillWidth: true
            Layout.preferredHeight: manageEntitiesText.implicitHeight + entityCountText.implicitHeight + entityDescText.implicitHeight + 20

            states: [
                State {
                    name: "opened"
                    when: manageEntitiesPopup.opened
                    ParentChange { target: manageEntities; parent: manageEntitiesPopupContent; width: manageEntitiesPopupContent.width; height: manageEntitiesPopupContent.height }
                    PropertyChanges {target: manageEntities; color: colors.black; border.color: colors.transparent }
                    PropertyChanges {target: manageEntitiesClosedElements; opacity: 0; visible: false }
                    PropertyChanges {target: manageEntitiesOpenedElements; opacity: 1; visible: true }
                },
                State {
                    name: "closed"
                    when: manageEntitiesPopup.closed
                    ParentChange { target: manageEntities; parent: manageEntitiesContainer; width: manageEntitiesContainer.width; height: manageEntitiesContainer.height }
                    PropertyChanges {target: manageEntities; color: colors.dark; border.color: colors.medium }
                    PropertyChanges {target: manageEntitiesClosedElements; opacity: 1; visible: true }
                    PropertyChanges {target: manageEntitiesOpenedElements; opacity: 0; visible: false }
                }
            ]

            transitions: [
                Transition {
                    to: "opened"
                    SequentialAnimation {
                        ParallelAnimation {
                            ParentAnimation {
                                NumberAnimation { properties: "width, height"; easing.type: Easing.OutExpo; duration: 600 }
                            }

                            ColorAnimation { target: manageEntities; duration: 600 }
                            SequentialAnimation {
                                PropertyAnimation { target: manageEntitiesClosedElements; properties: "opacity"; easing.type: Easing.OutExpo; duration: 200 }
                                PropertyAnimation { target: manageEntitiesClosedElements; properties: "visible"; duration: 0 }
                            }
                            SequentialAnimation {
                                PauseAnimation { duration: 200 }
                                PropertyAnimation { target: manageEntitiesOpenedElements; properties: "visible"; duration: 0 }
                                PropertyAnimation { target: manageEntitiesOpenedElements; properties: "opacity"; easing.type: Easing.OutExpo; duration: 400 }
                            }
                        }
                        ScriptAction { script: manageEntitiesOpenedElements.open() }
                    }
                },
                Transition {
                    to: "closed"
                    ParallelAnimation {
                        SequentialAnimation {
                            PropertyAnimation { target: manageEntitiesOpenedElements; properties: "opacity"; easing.type: Easing.OutExpo; duration: 200 }
                            PropertyAnimation { target: manageEntitiesOpenedElements; properties: "visible"; duration: 0 }
                        }
                        SequentialAnimation {
                            PauseAnimation { duration: 200 }
                            PropertyAnimation { target: manageEntitiesClosedElements; properties: "visible"; duration: 0 }
                            PropertyAnimation { target: manageEntitiesClosedElements; properties: "opacity"; easing.type: Easing.OutExpo; duration: 400 }
                        }
                        ColorAnimation { target: manageEntities; duration: 400 }
                        ParentAnimation {
                            NumberAnimation { properties: "width, height"; easing.type: Easing.OutExpo; duration: 500 }
                        }
                    }
                }
            ]


            Popup {
                id: manageEntitiesPopup

                parent: Overlay.overlay
                width: parent.width; height: parent.height
                modal: false
                closePolicy: Popup.NoAutoClose
                padding: 0

                onClosed: {
                    ui.setTimeOut(500, () => { EntityController.loadConfiguredEntities(integrationId); });
                }

                enter: Transition {
                    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 0 }
                }

                exit: Transition {
                    NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 0 }
                }

                background: Rectangle { color: colors.black }
                contentItem: Item {
                    id: manageEntitiesPopupContent
                }
            }

            Rectangle {
                id: manageEntities
                anchors.centerIn: parent
                color: colors.dark
                radius: ui.cornerRadiusSmall
                border {
                    color: colors.medium
                    width: 1
                }

                Item {
                    id: manageEntitiesClosedElements
                    anchors.fill: parent
                    enabled: visible

                    Text {
                        id: manageEntitiesText
                        text: qsTr("Manage entities")
                        maximumLineCount: 1
                        color: colors.offwhite
                        font: fonts.primaryFont(24)
                        anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 20; right: parent.right; rightMargin: 20 }
                    }

                    Text {
                        id: entityCountText
                        text: EntityController.configuredEntitiesCount
                        maximumLineCount: 1
                        color: colors.offwhite
                        font: fonts.primaryFont(120, "Thin")
                        anchors { left: parent.left; leftMargin: 20; top: manageEntitiesText.top; topMargin: 20  }
                    }

                    Text {
                        id: entityDescText
                        text: qsTr("configured entities")
                        maximumLineCount: 1
                        color: colors.offwhite
                        font: fonts.primaryFont(24)
                        anchors { left: parent.left; leftMargin: 20; top: entityCountText.bottom; topMargin: -20 }
                    }

                    Components.HapticMouseArea {
                        anchors.fill: parent
                        onClicked: {
                            manageEntitiesPopup.open();
                        }
                    }
                }

                Integrations.ManageEntities {
                    id: manageEntitiesOpenedElements
                    enabled: visible
                    onClosed: {
                        manageEntitiesPopup.close();
                        ui.setTimeOut(500, () => { EntityController.getConfiguredEntities(integrationId); });
                    }
                }
            }
        }

        ColumnLayout {
            Layout.topMargin: 20
            Layout.bottomMargin: 10
            Layout.fillWidth: true

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 10
                Layout.bottomMargin: 10

                Text {
                    id: title

                    Layout.fillWidth: true

                    text: connectedSwitch.checked ? qsTr("Connected") : qsTr("Disconnected")
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.light
                    font: fonts.secondaryFont(24)
                }

                Components.Switch {
                    id: connectedSwitch

                    Layout.alignment: Qt.AlignRight

                    icon: "uc:check"
                    checked: integrationObj.state === "connected"
                    trigger: function() {
                        if (integrationObj.state === "connected") {
                            IntegrationController.integrationDisconnect(integrationObj.id);
                        } else if (integrationObj.state === "disconnected" || integrationObj.state === "error") {
                            IntegrationController.integrationConnect(integrationObj.id);
                        }
                    }
                    enabled: integrationObj.state === "connected" || integrationObj.state === "disconnected" || integrationObj.state === "error"
                    opacity: enabled ? 1 : 0.3
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1

                color: colors.medium
            }
        }

        Components.AboutInfo {
            Layout.bottomMargin: 10

            key: qsTr("State")
            value: integrationObj.state
        }

        Components.AboutInfo {
            Layout.bottomMargin: 10

            key: qsTr("Enabled")
            value: integrationObj.enabled
        }

        Components.AboutInfo {
            Layout.bottomMargin: 10

            key: qsTr("Id")
            value: integrationObj.id
        }

        Components.AboutInfo {
            Layout.bottomMargin: 10

            key: qsTr("Version")
            value: integrationDriverObj.version
        }

        Components.AboutInfo {
            Layout.bottomMargin: 10

            key: qsTr("Developer")
            value: integrationDriverObj.developerName
            multiline: true
        }

        Components.AboutInfo {
            Layout.bottomMargin: 10

            key: qsTr("Website")
            value: integrationDriverObj.homepage
            multiline: true
            lineBottom: descriptionInfo.visible
            visible: integrationDriverObj.homepage
        }

        Components.AboutInfo {
            id: descriptionInfo

            Layout.bottomMargin: 10

            value: integrationDriverObj.description
            multiline: true
            lineBottom: false
            visible: integrationDriverObj.description
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
        }
    }

    Item {
        id: deleteContainer
        width: integrationInfoFlickable.width
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
                    "pressed": function() {
                        deleteContainer.state = "closed";
                    }
                },
                "HOME": {
                    "pressed": function() {
                        deleteContainer.state = "closed";
                    }
                },
                "DPAD_MIDDLE": {
                    "pressed": function() {
                        deleteContainer.deleteIntegration();
                    }
                }
            }
        }

        function deleteIntegration() {
            if (integrationDriverObj.external) {
                IntegrationController.deleteIntegrationDriver(integrationDriverObj.id);
            } else {
                IntegrationController.deleteIntegration(integrationObj.id);
            }

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
                PropertyChanges { target: integrationInfoFlickable; interactive: true }
            },
            State {
                name: "opened"
                PropertyChanges { target: deleteContainerContent; y: -deleteContainerContent.height + deleteContainer.height + ui.cornerRadiusLarge }
                PropertyChanges { target: openIcon; opacity: 0 }
                PropertyChanges { target: deleteConfirmText; opacity: 1 }
                PropertyChanges { target: deleteConfirmButtons; opacity: 1 }
                PropertyChanges { target: blockOutOverlay; opacity: 0.8 }
                PropertyChanges { target: integrationInfoFlickable; interactive: false }
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
                    ScriptAction  { script: integrationInfoFlickable.contentY = (content.childrenRect.height + deleteContainer.height) - integrationInfoFlickable.height }
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

                        text: qsTr("Delete integration")
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

                    text: qsTr("Are you sure you want to delete the %1 integration?").arg(integrationObj.name)
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
                                deleteContainer.deleteIntegration();
                            }
                        }
                    }
                }
            }
        }
    }
}

