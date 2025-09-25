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
                PropertyAction { target: roomSelector; property: "hiddenAnimationDone"; value: true }
            }
        },
        Transition {
            to: "visible"

            SequentialAnimation {
                PropertyAnimation { target: roomSelector; properties: "scale, opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    signal editModeOff
    signal closed

    property bool editMode: false
    property bool hiddenAnimationDone: false

    onHiddenAnimationDoneChanged: {
        if (hiddenAnimationDone) {
            console.debug("Animation is done");
            roomSelector.closed();
        }
    }

    function open() {
        state = "visible";
        loading.stop();
        buttonNavigation.takeControl();
    }

    function close() {
        buttonNavigation.releaseControl();
        state = "hidden";
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_DOWN": {
                "pressed": function() {
                    containerMain.item.pages.incrementCurrentIndex();
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    containerMain.item.pages.decrementCurrentIndex();
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (keyboard.state === "") {
                        editMode = false;
                        roomSelector.close();
                    }
                }
            },
            "BACK": {
                "pressed": function() {
                    editMode = false;
                    roomSelector.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    editMode = false;
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
                    color: index === containerMain.item.pages.currentIndex && ui.keyNavigationEnabled ? colors.dark : colors.transparent
                    radius: ui.cornerRadiusSmall
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
