// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQml.Models 2.1

import Haptic 1.0


import "qrc:/components" as Components

Rectangle {
    id: roomSelector
    width: parent.width; height: parent.height
    color: colors.black

    state: "hidden"

    transform: Scale {
        origin.x: roomSelector.width/2; origin.y: roomSelector.height/2
    }

    states: [
        State {
            name: "hidden"
            PropertyChanges { target: roomSelector; scale: 0.5; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: roomSelector; scale: 1; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"

            SequentialAnimation {
                PropertyAnimation { target: roomSelector; properties: "scale, opacity"; easing.type: Easing.InExpo; duration: 200 }
                ScriptAction { script: roomSelector.closed() }
            }
        },
        Transition {
            to: "visible"

            SequentialAnimation {
                PropertyAnimation { target: roomSelector; properties: "scale, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                ScriptAction { script: buttonNavigation.takeControl() }
            }
        }
    ]

    signal editModeOff
    signal closed

    property bool editMode: false

    function open() {
        state = "visible";
        loading.stop();
    }

    function close() {
        buttonNavigation.releaseControl();
        state = "hidden";
    }

    /** KEYPAD SELECTION (edit mode) **/
    // The selection is the main screen's page index, as for the normal mode. In edit mode a long
    // press on DPAD_MIDDLE picks the page up (UP / DOWN move it, DPAD_MIDDLE drops it), RIGHT
    // reveals the delete of the row and DPAD_MIDDLE then deletes it, LEFT hides it again, DOWN past
    // the last page selects the "+" footer. The edit mode itself is entered with a long press on
    // DPAD_MIDDLE, like the pencil at the top with a tap.
    property bool footerSelected: false
    property int heldIndex: -1

    readonly property var pages: containerMain.item.pages

    function currentRow() {
        return roomList.currentItem;
    }

    function leaveEditMode() {
        roomSelector.heldIndex = -1;
        roomSelector.footerSelected = false;
        editMode = false;
        editModeOff();
    }

    function dropHeld() {
        const index = roomSelector.heldIndex;
        roomSelector.heldIndex = -1;
        Haptic.play(Haptic.Click);
        roomSelector.pages.currentIndex = index;
        roomSelector.pages.positionViewAtIndex(index, ListView.Visible);
        ui.updatePagePos();
    }

    function moveSelection(delta) {
        if (roomSelector.heldIndex >= 0) {
            const to = roomSelector.heldIndex + delta;
            if (to < 0 || to >= roomList.count) {
                return;
            }

            ui.pages.swapData(roomSelector.heldIndex, to);
            roomSelector.heldIndex = to;
            roomSelector.pages.currentIndex = to;
            return;
        }

        if (roomSelector.footerSelected) {
            if (delta < 0) {
                roomSelector.footerSelected = false;
            }
            return;
        }

        if (delta > 0 && editMode && roomSelector.pages.currentIndex >= roomList.count - 1) {
            const row = roomSelector.currentRow();
            if (row) {
                row.closeDelete();
            }
            roomSelector.footerSelected = true;
            roomList.positionViewAtEnd();
            return;
        }

        if (delta > 0) {
            roomSelector.pages.incrementCurrentIndex();
        } else {
            roomSelector.pages.decrementCurrentIndex();
        }
    }

    function activateSelection() {
        if (!editMode) {
            if (keyboard.state === "") {
                roomSelector.close();
            }
            return;
        }

        if (roomSelector.footerSelected) {
            pageAdd.state = "visible";
            return;
        }

        if (roomSelector.heldIndex >= 0) {
            roomSelector.dropHeld();
            return;
        }

        const row = roomSelector.currentRow();
        if (!row) {
            return;
        }

        if (row.deleteOpen) {
            ui.deletePage(row.rowPageId);
            return;
        }

        pageRename.currentPage = row.rowPageName;
        pageRename.pageId = row.rowPageId;
        pageRename.state = "visible";
    }

    function holdSelection() {
        if (ui.profile.restricted) {
            return;
        }

        if (!editMode) {
            editMode = true;
            return;
        }

        const row = roomSelector.currentRow();
        if (roomSelector.footerSelected || roomSelector.heldIndex >= 0 || !row) {
            return;
        }

        row.closeDelete();
        Haptic.play(Haptic.Click);
        roomSelector.heldIndex = roomSelector.pages.currentIndex;
    }

    onEditModeChanged: {
        if (!editMode) {
            roomSelector.heldIndex = -1;
            roomSelector.footerSelected = false;
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_DOWN": {
                "pressed": function() {
                    roomSelector.moveSelection(1);
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    roomSelector.moveSelection(-1);
                }
            },
            "DPAD_RIGHT": {
                "pressed": function() {
                    const row = roomSelector.currentRow();
                    if (editMode && !roomSelector.footerSelected && roomSelector.heldIndex < 0 && row) {
                        row.openDelete();
                    }
                }
            },
            "DPAD_LEFT": {
                "pressed": function() {
                    const row = roomSelector.currentRow();
                    if (editMode && row) {
                        row.closeDelete();
                    }
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    roomSelector.activateSelection();
                },
                "long_press": function() {
                    roomSelector.holdSelection();
                }
            },
            "BACK": {
                "pressed": function() {
                    // one layer at a time: drop the held page, hide the delete, leave the edit
                    // mode, close the selector
                    if (roomSelector.heldIndex >= 0) {
                        roomSelector.dropHeld();
                        return;
                    }

                    const row = roomSelector.currentRow();
                    if (editMode && row && row.deleteOpen) {
                        row.closeDelete();
                        return;
                    }

                    if (editMode) {
                        roomSelector.leaveEditMode();
                        return;
                    }

                    roomSelector.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    roomSelector.leaveEditMode();
                    roomSelector.close();
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    DelegateModel {
        id: visualModel

        model: ui.pages
        delegate: roomListItem
    }

    ListView {
        id: roomList
        width: parent.width; height: parent.height-60
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200

        model: visualModel
        currentIndex: containerMain.item.pages.currentIndex

        footer: footerItem

        ScrollBar.vertical: ScrollBar {
            opacity: 0.5
        }

        remove: Transition {
            NumberAnimation { properties: "x"; from:0; to: 100; duration: 300; easing.type: Easing.OutExpo }
            NumberAnimation { properties: "opacity"; from:1; to: 0; duration: 300; easing.type: Easing.OutExpo }
        }

        displaced: Transition {
            NumberAnimation { property: "y"; duration: 300; easing.type: Easing.OutExpo }
        }

        Component.onCompleted: {
            roomList.positionViewAtIndex(roomList.currentIndex, ListView.Visible);
        }
    }

    ButtonGroup {
        id: pageListGroup
    }

    Rectangle {
        width: ui.width; height: 60
        color: colors.black
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        Text {
            id: titleText
            color: colors.offwhite
            //: Title for the page selector menu
            text: editMode ? qsTr("Edit pages")  : qsTr("Select page")
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(26)
        }

        Components.Icon {
            visible: !ui.profile.restricted
            color: editMode ? colors.offwhite : colors.light
            icon: "uc:pen-to-square"
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
            size: 60

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    editMode = !editMode;
                    if (!editMode) {
                        editModeOff();
                    }
                }
            }
        }
    }

    Components.PageAdd {
        id: pageAdd
        anchors.centerIn: parent
    }

    Components.PageRename {
        id: pageRename
        anchors.centerIn: parent
    }

    Component {
        id: roomListItem

        MouseArea {
            id: dragArea
            width: ui.width
            height: 150
            enabled: editMode
            pressAndHoldInterval: 200

            property alias dragArea: dragArea
            property bool held: false
            property bool deleteOpen: false
            property int toVal: 0
            readonly property string rowPageId: pageId
            readonly property string rowPageName: pageName
            readonly property bool keypadHeld: roomSelector.heldIndex === index

            function openDelete() {
                content.x = 150;
                deleteOpen = true;
            }

            function closeDelete() {
                content.x = 0;
                deleteOpen = false;
            }

            drag.target: held ? content : undefined
            drag.axis: Drag.YAxis

            onPressAndHold:  {
                roomList.interactive = false;

                if (!held) {
                    Haptic.play(Haptic.Click);
                    held = true;
                }
            }

            onReleased: {
                roomList.interactive = true;

                if (held) {
                    Haptic.play(Haptic.Click);
                    held = false;

                    containerMain.item.pages.currentIndex = index;
                    containerMain.item.pages.positionViewAtIndex(index, ListView.Visible);

                    ui.updatePagePos();
                    console.debug("update pos for: " + pageName + " new pos: " + index);
                }
            }

            property int scrollEdgeSize: 100
            property int scrollingDirection: 0

            SmoothedAnimation {
                id: upAnimation
                target: roomList
                property: "contentY"
                to: 0
                running: scrollingDirection == -1
            }

            SmoothedAnimation {
                id: downAnimation
                target: roomList
                property: "contentY"
                to: roomList.contentHeight - 150 - roomList.height
                running: scrollingDirection == 1
            }

            states: [
                State {
                    when: dragArea.drag.active
                    name: "dragging"

                    PropertyChanges {
                        target: dragArea
                        scrollingDirection: {
                            var yCoord = roomList.mapFromItem(dragArea, dragArea.mouseY, 0).y;

                            if (yCoord < scrollEdgeSize) {
                                -1;
                            } else if (yCoord > roomList.height - 150 - scrollEdgeSize) {
                                1;
                            } else {
                                0;
                            }
                        }
                    }
                }
            ]

            Item {
                id: closeButton
                width: 150; height: 150
                anchors { bottom: parent.bottom; right: content.left }
                visible: editMode

                Components.Icon {
                    color: colors.red
                    icon: "uc:xmark"
                    anchors.centerIn: parent
                    size: 80
                }

                Components.HapticMouseArea {
                    anchors.fill: parent
                    enabled: deleteOpen

                    onClicked: {
                        ui.deletePage(pageId);
                    }
                }
            }

            Rectangle {
                id: content
                width: ui.width
                height: 150
                color: colors.black
                radius: ui.cornerRadiusSmall

                Drag.active: dragArea.held
                Drag.source: dragArea
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2

                states: State {
                    when: dragArea.held
                    ParentChange { target: content; parent: roomList }
                }

                Connections {
                    target: roomSelector
                    ignoreUnknownSignals: true

                    function onEditModeOff() {
                        content.x = 0;
                        deleteOpen = false;
                    }
                }

                Behavior on x {
                    NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
                }

                AbstractButton {
                    width: editMode ? parent.width - 200 : parent.width; height: parent.height
                    anchors.left: parent.left
                    ButtonGroup.group: pageListGroup
                    checkable: true
                    checked: deleteOpen
                    enabled: !dragArea.held
                    onCheckedChanged: {
                        Haptic.play(Haptic.Click);
                        if (!checked) {
                            content.x = 0;
                            deleteOpen = false;
                        }
                    }
                    onClicked: {
                        editMode = false;
                        containerMain.item.pages.currentIndex = index;
                        containerMain.item.pages.positionViewAtIndex(index, ListView.Visible);
                        roomSelector.close();
                    }
                }

                Flickable {
                    width: parent.width - moveIcon.width - 40; height: parent.height
                    anchors { left: parent.left; leftMargin: 0 }
                    enabled: editMode

                    flickableDirection: Flickable.HorizontalFlick
                    boundsMovement: Flickable.StopAtBounds

                    onFlickStarted: {
                        if (horizontalVelocity < 0) {
                            content.x = 150;
                            deleteOpen = true;
                        }
                        if (horizontalVelocity > 0) {
                            content.x = 0;
                            deleteOpen = false;
                        }
                    }

                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            if (deleteOpen) {
                                content.x = 0;
                                deleteOpen = false;
                            } else {
                                editMode = false;
                                containerMain.item.pages.currentIndex = index;
                                containerMain.item.pages.positionViewAtIndex(index, ListView.Visible);
                                roomSelector.close();
                            }
                        }
                    }
                }

                Rectangle {
                    anchors.fill: parent
                    color: index === containerMain.item.pages.currentIndex && !roomSelector.footerSelected
                           && ui.keyNavigationActive ? colors.dark : colors.transparent
                    radius: ui.cornerRadiusSmall
                    border {
                        width: 2
                        color: dragArea.keypadHeld && ui.keyNavigationActive ? colors.highlight : colors.transparent
                    }
                }

                Text {
                    id: titleText
                    color: colors.offwhite
                    text: pageName
                    width: parent.width - (editMode ? 160 : 20)
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                    anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter; horizontalCenterOffset: deleteOpen ? -80 : 0 }
                    font: fonts.primaryFont(50, "Light")
                    lineHeight: 0.8

                    Behavior on anchors.horizontalCenterOffset {
                        NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
                    }

                }

                Components.Icon {
                    id: moveIcon
                    color: colors.light
                    opacity: editMode ? 1 : 0
                    icon: "uc:bars"
                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                    size: 80

                    Behavior on opacity {
                        OpacityAnimator { duration: 300 }
                    }
                }

                Components.Icon {
                    color: colors.light
                    visible: !deleteOpen
                    opacity: editMode ? deleteOpen ? 0 : 1 : 0
                    icon: "uc:pen-to-square"
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                    size: 80

                    Behavior on opacity {
                        OpacityAnimator { duration: 300 }
                    }

                    Components.HapticMouseArea {
                        anchors.fill: parent
                        enabled: editMode && !dragArea.held

                        onClicked: {
                            pageRename.currentPage = pageName;
                            pageRename.pageId = pageId;
                            pageRename.state = "visible";
                        }
                    }
                }
            }

            DropArea {
                width: parent.width
                height: 150

                onEntered: {
                    let from = drag.source.DelegateModel.itemsIndex;
                    dragArea.toVal = dragArea.DelegateModel.itemsIndex;

                    ui.pages.swapData(from, dragArea.toVal);
                }
            }
        }
    }

    Component {
        id: footerItem

        Item {
            width: ui.width; height: editMode ? 150 : 0
            visible: editMode

            Rectangle {
                anchors { fill: parent; margins: 20 }
                radius: ui.cornerRadiusSmall
                color: colors.transparent
                border {
                    width: 2
                    color: roomSelector.footerSelected && ui.keyNavigationActive ? colors.highlight : colors.transparent
                }
            }

            Item {
                id: plusIcon
                anchors.centerIn: parent

                Rectangle {
                    width: 60
                    height: 2
                    color: colors.offwhite
                    anchors.centerIn: parent
                }

                Rectangle {
                    width: 2
                    height: 60
                    color: colors.offwhite
                    anchors.centerIn: parent
                }
            }

            Components.HapticMouseArea {
                anchors.fill: parent

                onClicked: {
                    pageAdd.state = "visible";
                }
            }
        }
    }
}
