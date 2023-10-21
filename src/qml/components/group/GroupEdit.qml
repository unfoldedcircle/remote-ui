// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQml.Models 2.1

import "qrc:/components" as Components
import "qrc:/components/entities" as Entities

import Haptic 1.0
import Entity.Controller 1.0
import Group.Controller 1.0

Rectangle {
    id: editGroupContainer
    color: colors.black
    anchors.fill: parent
    enabled: opacity == 1

    signal closed()

    property string groupId
    property string groupName
    property bool inputHasFocus: false
    property bool entitiesListLoaded: false
    property QtObject groupData

    function open() {
        state = "visible";
        loading.stop();
    }

    function close() {
        state = "hidden";
    }


    onStateChanged: {
        if (state == "hidden") {
            keyboard.hide();
            buttonNavigation.releaseControl();
        } else {
            buttonNavigation.takeControl();
        }
    }

    Connections {
        target: GroupController

        function onGroupUpdated(groupId, success) {
            // add group to page
            loading.stop();

            if (success) {
                editGroupContainer.state = "hidden";
            } else {
                console.debug("Error updating group");
            }
        }
    }

    state: "hidden"

    states: [
        State {
            name: "hidden"
            PropertyChanges { target: editGroupContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: editGroupContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            from: "visible"
            to: "hidden"

            SequentialAnimation {
                PropertyAnimation { target: editGroupContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
                ScriptAction { script: editGroupContainer.closed(); }
            }
        },
        Transition {
            from: "hidden"
            to: "visible"
            ParallelAnimation {
                PropertyAnimation { target: editGroupContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    close();
                }
            },
            "HOME": {
                "released": function() {
                    close();
                }
            },
            "DPAD_MIDDLE": {
                "released": function() {
                    close();
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    ListView {
        id: entityList
        width: parent.width; height: parent.height - editGroupContainerTitle.height - 120
        anchors.top: editGroupContainerTitle.bottom

        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        highlightMoveDuration: 200
        cacheBuffer: 280 * 30

        model: visualModel

        DelegateModel {
            id: visualModel
            model: groupData.groupItems()
            delegate: listItem
        }

        header: header

        currentIndex: 0
    }

    Item {
        id: editGroupContainerTitle
        width: parent.width; height: 60
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

        Text {
            id: editGroupContainerTitleText
            color: colors.offwhite
            text: groupName
            anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
            font: fonts.primaryFont(26)
        }
    }

    Rectangle {
        width: parent.width
        height: 100
        color: colors.black
        anchors.bottom: parent.bottom

        Components.Button {
            //: Button caption
            text: qsTr("Done")
            width: parent.width
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
            trigger: function() {
                loading.start();
                // create group
                GroupController.updateGroup(groupId, ui.profile.id, groupName, groupData.getEntities())
            }
        }
    }

    Entities.EntityList {
        id: entitySelectionList
        anchors { top: parent.top; bottom: parent.bottom; left: parent.left; right: parent.right }
        visible: false
        title: qsTr("Add entities")
        model: EntityController.configuredEntities
        entityDescriptionIntegration: true
        closeListOnTrigger: false
        showCloseIcon: true

        okTrigger: function() {
            let selectedEntities = EntityController.configuredEntities.getSelected();

            if (selectedEntities.length > 0) {
                entitySelectionList.close();
                loading.start();
                groupData.addEntities(selectedEntities);
                loading.stop();
                EntityController.configuredEntities.clearSelected();
            } else {
                ui.createActionableNotification(qsTr("Select entities"), qsTr("Please select entities to add by tapping in the list."));
            }
        }

        onClosed: entitySelectionList.visible = false
    }

    ButtonGroup {
        id: buttonGroup
    }

    Component {
        id: listItem

        MouseArea {
            id: dragArea
            width: delegate.item ? delegate.item.width : delegate.width
            height: delegate.item ? delegate.item.height : delegate.height
            enabled: editMode
            pressAndHoldInterval: 200

            property bool editMode: true

            property alias dragArea: dragArea
            property alias delegate: delegate

            property bool isCurrentItem: ListView.isCurrentItem
            property bool held: false
            property bool deleteOpen: false
            property int toVal: 0

            property string itemId: groupItemId

            drag.target: held ? delegate : undefined
            drag.axis: Drag.YAxis

            onPressAndHold:  {
                entityList.interactive = false;

                if (!held) {
                    Haptic.play(Haptic.Click);
                    held = true;
                    swipeMouseArea.enabled = false;

                    if (delegate.item.groupObj) {
                        delegate.item.close();
                    }
                }
            }

            onReleased: {
                entityList.interactive = true;

                if (held) {
                    Haptic.play(Haptic.Click);
                    held = false;
                } else {
                    swipeMouseAreaTimer.start();
                }
            }

            property int scrollEdgeSize: 200
            property int scrollingDirection: 0

            SmoothedAnimation {
                id: upAnimation
                target: entityList
                property: "contentY"
                to: -100
                running: scrollingDirection == -1
                duration: 500
            }

            SmoothedAnimation {
                id: downAnimation
                target: entityList
                property: "contentY"
                to: entityList.contentHeight - 150 - entityList.height
                running: scrollingDirection == 1
                duration: 500
            }

            states: [
                State {
                    when: dragArea.drag.active
                    name: "dragging"

                    PropertyChanges {
                        target: dragArea
                        scrollingDirection: {
                            var yCoord = entityList.mapFromItem(dragArea, dragArea.mouseY, 0).y;

                            if (yCoord < scrollEdgeSize) {
                                -1;
                            } else if (yCoord > entityList.height - 150 - scrollEdgeSize) {
                                1;
                            } else {
                                0;
                            }
                        }
                    }
                }
            ]

            Timer {
                id: swipeMouseAreaTimer
                running: false
                repeat: false
                interval: 1000

                onTriggered: {
                    swipeMouseArea.enabled = true;
                }
            }

            Flickable {
                id: swipeMouseArea
                width: parent.width - deleteButton.width - 40; height: parent.height
                anchors { left: parent.left; leftMargin: deleteOpen ? 200 : 0 }

                flickableDirection: Flickable.HorizontalFlick
                boundsMovement: Flickable.StopAtBounds

                onFlickStarted: {
                    if (horizontalVelocity < 0) {
                        delegate.x = 150;
                        deleteOpen = true;
                    }
                    if (horizontalVelocity > 0) {
                        delegate.x = 0;
                        deleteOpen = false;
                    }
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        if (deleteOpen) {
                            delegate.x = 0;
                            deleteOpen = false;
                        }
                    }
                }
            }

            Item {
                id: deleteButton
                width: 150; height: 150
                anchors { right: delegate.left }
                visible: editMode

                Components.Icon {
                    color: colors.red
                    icon: "uc:close"
                    anchors.centerIn: parent
                    size: 80
                }

                Rectangle {
                    color: colors.medium
                    width: parent.width; height: 1
                    anchors.top: parent.top
                }

                Components.HapticMouseArea {
                    anchors.fill: parent
                    enabled: deleteOpen

                    onClicked: {
                        groupData.groupItems().removeItem(modelData);
                    }
                }
            }

            Loader {
                id: delegate
                asynchronous: true
                enabled: false

                Drag.active: dragArea.held
                Drag.source: dragArea
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2

                states: State {
                    when: dragArea.held
                    ParentChange { target: delegate; parent: entityList }
                }

                property bool isCurrentItem: isCurrentItem
                property bool editMode: editMode

                onEditModeChanged: {
                    if (!editMode) {
                        delegate.x = 0;
                        deleteOpen = false;
                    }
                }

                Behavior on x {
                    NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
                }

                AbstractButton {
                    width: editMode ? parent.width - 200 : parent.width; height: parent.height
                    anchors.left: parent.left
                    ButtonGroup.group: buttonGroup
                    checkable: true
                    checked: deleteOpen
                    enabled: !dragArea.held
                    onCheckedChanged: {
                        Haptic.play(Haptic.Click);
                        if (!checked) {
                            delegate.x = 0;
                            deleteOpen = false;
                        }
                    }
                }

                Component.onCompleted: {
                    delegate.setSource("qrc:/components/entities/Base.qml", { "entityId": groupItemId });
                }

                Components.Icon {
                    id: moveIcon
                    z: delegate.item ? delegate.item.z + 100 : 100
                    color: colors.light
                    opacity: editMode ? 1 : 0
                    icon: "uc:hamburger"
                    anchors { right: delegate.right; rightMargin: 20; top: delegate.top; topMargin: 35 }
                    size: 80

                    Behavior on opacity {
                        OpacityAnimator { duration: 300 }
                    }
                }
            }

            DropArea {
                anchors.fill: parent

                onEntered: {
                    dragArea.toVal = dragArea.DelegateModel.itemsIndex;
                    groupData.groupItems().swapData(drag.source.DelegateModel.itemsIndex, dragArea.toVal);

                }
            }
        }
    }



    Component {
        id: header

        Rectangle {
            width: parent.width
            height: 150
            color: colors.black

            Components.ButtonAdd {
                width: ui.width
                height: 150
                radius: ui.cornerRadiusSmall
                text: qsTr("Add entity")
                trigger: function() {
                    if (!editGroupContainer.entitiesListLoaded) {
                        editGroupContainer.entitiesListLoaded = true;
                    }
                    entitySelectionList.visible = true;
                    entitySelectionList.open();
                }
            }
        }
    }
}
