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
        // a reopened popup starts on its first control again
        buttonNavigation.lastFocusItem = null;
        buttonNavigation.lastFocusAnchor = null;
    }

    /** KEYBOARD NAVIGATION **/
    // The popup navigates through the QML focus chain: Manage entities -> connected switch ->
    // Delete. The button navigation only binds BACK / HOME; a DPAD handler here would act on the
    // same key press as the focused control.
    Components.ButtonNavigation {
        id: buttonNavigation
        overrideActive: ui.inputController.activeItem === popup
        manageFocus: true
        scrollTarget: integrationInfoFlickable
        initialFocusItem: manageEntitiesRow
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

    // a key nobody in the chain accepted: the focus is at the end of the chain, scroll on so the
    // rest of the content can be reached (see Settings.Page)
    function scrollChainEnd(event, direction) {
        if (!buttonNavigation.hasInputControl) {
            return;
        }

        const focused = buttonNavigation.windowFocusItem;
        if (!focused || focused === integrationInfoFlickable || focused === buttonNavigation
                || !buttonNavigation.isInScope(focused)) {
            return;
        }

        event.accepted = buttonNavigation.scrollBy(direction * Math.round(integrationInfoFlickable.height / 2));
    }

    Keys.onDownPressed: scrollChainEnd(event, 1)
    Keys.onUpPressed: scrollChainEnd(event, -1)

    ColumnLayout {
        id: content
        spacing: 0
        width: integrationInfoFlickable.width

        RowLayout {
            Layout.topMargin: 20
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            Rectangle {
                Layout.alignment: Qt.AlignTop

                width: 80
                height: width
                radius: 40
                color: integrationObj.icon.includes("uc") ? colors.textPrimary : colors.transparent

                Components.Icon {
                    color: colors.bg
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

                color: colors.textPrimary
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
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            text: integrationObj.name
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.textPrimary
            font: fonts.title()
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            text: integrationDriverObj.external ? qsTr("External integration") : qsTr("Local integration")
            maximumLineCount: 1
            color: colors.textSecondary
            font: fonts.help()
        }

        Item {
            id: manageEntitiesContainer

            Layout.topMargin: 20
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.fillWidth: true
            Layout.preferredHeight: manageEntitiesText.implicitHeight + entityCountText.implicitHeight + entityDescText.implicitHeight + 20

            states: [
                State {
                    name: "opened"
                    when: manageEntitiesPopup.opened
                    ParentChange { target: manageEntities; parent: manageEntitiesPopupContent; width: manageEntitiesPopupContent.width; height: manageEntitiesPopupContent.height }
                    PropertyChanges {target: manageEntities; color: colors.bg; border.color: colors.transparent }
                    PropertyChanges {target: manageEntitiesClosedElements; opacity: 0; visible: false }
                    PropertyChanges {target: manageEntitiesOpenedElements; opacity: 1; visible: true }
                },
                State {
                    name: "closed"
                    when: manageEntitiesPopup.closed
                    ParentChange { target: manageEntities; parent: manageEntitiesContainer; width: manageEntitiesContainer.width; height: manageEntitiesContainer.height }
                    PropertyChanges {target: manageEntities; color: colors.surface; border.color: colors.divider }
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

                background: Rectangle { color: colors.bg }
                contentItem: Item {
                    id: manageEntitiesPopupContent
                }
            }

            Rectangle {
                id: manageEntities
                anchors.centerIn: parent
                color: colors.surface
                radius: ui.cornerRadiusSmall
                border {
                    color: colors.divider
                    width: 2
                }

                Item {
                    id: manageEntitiesClosedElements
                    anchors.fill: parent
                    enabled: visible

                    Text {
                        id: manageEntitiesText
                        text: qsTr("Manage entities")
                        maximumLineCount: 1
                        color: colors.textPrimary
                        font: fonts.label()
                        anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 20; right: parent.right; rightMargin: 20 }
                    }

                    Text {
                        id: entityCountText
                        text: EntityController.configuredEntitiesCount
                        maximumLineCount: 1
                        color: colors.offwhite
                        font: fonts.display(120)
                        anchors { left: parent.left; leftMargin: 20; top: manageEntitiesText.top; topMargin: 20  }
                    }

                    Text {
                        id: entityDescText
                        text: qsTr("configured entities")
                        maximumLineCount: 1
                        color: colors.textSecondary
                        font: fonts.help()
                        anchors { left: parent.left; leftMargin: 20; top: entityCountText.bottom; topMargin: -20 }
                    }

                    Components.Selectable {
                        selected: manageEntitiesRow.activeFocus
                    }

                    Components.HapticMouseArea {
                        id: manageEntitiesRow
                        anchors.fill: parent
                        keypadActivatable: true
                        KeyNavigation.down: connectedSwitch
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

        Components.SettingRow {
            Layout.topMargin: 10
            title: connectedSwitch.checked ? qsTr("Connected") : qsTr("Disconnected")

            Components.Switch {
                id: connectedSwitch

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
                opacity: enabled ? 1 : 0.4

                /** KEYBOARD NAVIGATION **/
                KeyNavigation.up: manageEntitiesRow
                KeyNavigation.down: deleteRow
                highlight: activeFocus && ui.keyNavigationActive
            }
        }

        Components.KeyValueRow {
            key: qsTr("State")
            value: integrationObj.state
        }

        Components.KeyValueRow {
            key: qsTr("Enabled")
            value: integrationObj.enabled
        }

        Components.KeyValueRow {
            key: qsTr("Id")
            value: integrationObj.id
        }

        Components.KeyValueRow {
            key: qsTr("Version")
            value: integrationDriverObj.version
        }

        Components.KeyValueRow {
            key: qsTr("Developer")
            value: integrationDriverObj.developerName
            stacked: true
        }

        Components.KeyValueRow {
            key: qsTr("Website")
            value: integrationDriverObj.homepage
            stacked: true
            showDivider: descriptionInfo.visible
            visible: integrationDriverObj.homepage
        }

        Text {
            id: descriptionInfo

            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.topMargin: 14

            text: integrationDriverObj.description
            wrapMode: Text.WordWrap
            color: colors.textPrimary
            font: fonts.prose()
            lineHeight: fonts.proseLineHeight
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

        // the selection starts on Cancel: deleting an integration must take a deliberate second step
        property bool cancelSelected: true

        onStateChanged: {
            if (state == "opened") {
                deleteContainer.cancelSelected = true;
                deleteContainerButtonNavigation.takeControl();
                // The opener row lives inside this drawer, so the page's focus management leaves
                // the focus on it (a layer inside the scope keeps its own control). Park it on the
                // page's button navigation ourselves: otherwise DPAD_MIDDLE on Cancel would close
                // the drawer on the input path and reopen it through the row's Return on the focus
                // path. The page claims the row back when the drawer releases the input.
                if (deleteRow.activeFocus) {
                    buttonNavigation.forceActiveFocus();
                }
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
                "DPAD_LEFT": {
                    "pressed": function() {
                        deleteContainer.cancelSelected = true;
                    }
                },
                "DPAD_RIGHT": {
                    "pressed": function() {
                        deleteContainer.cancelSelected = false;
                    }
                },
                "DPAD_MIDDLE": {
                    "pressed": function() {
                        if (deleteContainer.cancelSelected) {
                            deleteContainer.state = "closed";
                        } else {
                            deleteContainer.deleteIntegration();
                        }
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
                PropertyChanges { target: blockOutOverlay; opacity: 0.85 }
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
            color: colors.bg
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

        // a bottom sheet like every other (docs/design-system.md section 5, Q-4): black with a divider edge
        Item {
            id: deleteContainerContent
            width: parent.width
            height: deleteColumn.height + ui.cornerRadiusLarge

            Components.Sheet {
                anchors.fill: parent
            }

            ColumnLayout {
                id: deleteColumn
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
                        color: colors.red
                        font: fonts.heading()
                    }

                    Components.Icon {
                        id: openIcon

                        Layout.alignment: Qt.AlignRight

                        icon: "uc:trash"
                        size: 100
                        color: colors.red

                        Components.Selectable {
                            anchors.margins: 10
                            selected: deleteRow.activeFocus
                        }

                        Components.HapticMouseArea {
                            id: deleteRow
                            anchors.fill: parent
                            keypadActivatable: true
                            KeyNavigation.up: connectedSwitch
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
                    color: colors.textPrimary
                    font: fonts.prose()
                    lineHeight: fonts.proseLineHeight
                }

                // Cancel and Delete are buttons, Cancel first and preselected (docs/design-system.md section 6)
                RowLayout {
                    id: deleteConfirmButtons

                    Layout.fillWidth: true
                    Layout.topMargin: 40
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.bottomMargin: 20
                    spacing: 20

                    Components.Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: qsTr("Cancel")
                        variant: "secondary"
                        highlight: deleteContainer.cancelSelected && ui.keyNavigationActive
                        trigger: function() {
                            deleteContainer.state = "closed";
                        }
                    }

                    Components.Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: qsTr("Delete")
                        variant: "destructive"
                        highlight: !deleteContainer.cancelSelected && ui.keyNavigationActive
                        trigger: function() {
                            deleteContainer.deleteIntegration();
                        }
                    }
                }
            }
        }
    }
}

