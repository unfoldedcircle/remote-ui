// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/components" as Components

import Haptic 1.0
import Entity.Controller 1.0
import Group.Controller 1.0

Rectangle {
    id: addGroupContainer
    color: colors.black
    anchors.fill: parent
    enabled: opacity == 1

    signal closed()

    property string pageId
    property string groupId
    property bool inputHasFocus: false

    function open() {
        state = "visible";
    }

    function close() {
        state = "hidden";
    }

    onStateChanged: {
        if (state == "visible") {
            inputFieldContainer.inputField.focus = true;
            inputFieldContainer.inputField.forceActiveFocus();
            keyboard.show();
            buttonNavigation.takeControl();
        } else {
            keyboard.hide();
            buttonNavigation.releaseControl();
        }
    }

    Connections {
        target: EntityController.configuredEntities

        function onEntitiesLoaded(count) {
            loading.stop();
        }
    }

    Connections {
        target: GroupController

        function onGroupAdded(groupId, success) {
            // add group to page
            loading.stop();

            if (success) {
                steps.incrementCurrentIndex();
                keyboard.hide();
                loading.start();
                EntityController.configuredEntities.init();
                ui.pages.get(addGroupContainer.pageId).addGroup(groupId);
                addGroupContainer.groupId = groupId;
            } else {
                steps.currentIndex = 0;
                inputFieldContainer.showError(qsTr("There was an error. Try again"));
            }
        }

        function onGroupUpdated(groupId, success) {
            loading.stop();

            if (success) {
                addGroupContainer.close();
                ui.updatePageItems(addGroupContainer.pageId);
            } else {
                steps.currentIndex = 0;
                inputFieldContainer.showError(qsTr("There was an error. Try again"));
            }
        }

        function onGroupAlreadyExists() {
            loading.stop();
            steps.currentIndex = 0;
            inputFieldContainer.showError(qsTr("Group already exists"));
        }
    }

    state: "hidden"

    states: [
        State {
            name: "hidden"
            PropertyChanges { target: addGroupContainer; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: addGroupContainer; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            from: "visible"
            to: "hidden"
            ParallelAnimation {
                PropertyAnimation { target: addGroupContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }

            onRunningChanged: {
                if (!running) {
                    addGroupContainer.closed();
                }
            }
        },
        Transition {
            from: "hidden"
            to: "visible";
            ParallelAnimation {
                PropertyAnimation { target: addGroupContainer; properties: "opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        }
    ]

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    close();
                }
            },
            "HOME": {
                "pressed": function() {
                    close();
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    SwipeView {
        id: steps
        interactive: false
        anchors.fill: parent

        // add name
        Item {
            Item {
                id: addGroupContainerTitle
                width: parent.width; height: 60
                anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }

                Text {
                    id: addGroupContainerTitleText
                    color: colors.offwhite
                    //: Name for a group of entities
                    text: qsTr("Name your group")
                    anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
                    font: fonts.primaryFont(26)
                }
            }

            Components.InputField {
                id: inputFieldContainer
                width: parent.width; height: 80
                anchors { top: addGroupContainerTitle.bottom; horizontalCenter: parent.horizontalCenter }

                //: Example for a group name
                inputField.placeholderText: qsTr("All lights")
                inputField.onAccepted: {
                    if (inputFieldContainer.isEmpty()) {
                        inputFieldContainer.showError();
                    } else {
                        steps.incrementCurrentIndex();
                    }
                }
                moveInput: false
            }

            Components.Button {
                id: cancelButton
                text: qsTr("Cancel")
                width: parent.width / 2 - 10
                color: colors.secondaryButton
                anchors { left: inputFieldContainer.left; top: inputFieldContainer.bottom; topMargin: 40 }
                trigger: function() {
                    addGroupContainer.state = "hidden";
                }
            }

            Components.Button {
                text: qsTr("Next")
                width: parent.width / 2 - 10
                anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
                trigger: function() {
                    if (inputFieldContainer.isEmpty()) {
                        inputFieldContainer.showError();
                    } else {
                        GroupController.addGroup(ui.profile.id, inputFieldContainer.inputField.text)
                    }
                }
            }
        }

        // add entities
        Item {
            Rectangle {
                id: titleContainer
                color: colors.black
                width: parent.width
                height: 180
                z: 200
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

                Text {
                    id: titleText
                    text: qsTr("Select entities to add")
                    width: parent.width - 40
                    elide: Text.ElideRight
                    color: colors.offwhite
                    horizontalAlignment: Text.AlignHCenter
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 10 }
                    font: fonts.primaryFont(30)
                }

                Components.SearchField {
                    id: entitySearch
                    width: parent.width - 40
                    anchors { horizontalCenter: parent.horizontalCenter; top: titleText.bottom; topMargin: 20 }

                    placeholderText: qsTr("Search")

                    inputField.onFocusChanged: {
                        if (inputField.focus) {
                            inputHasFocus = true;
                        } else {
                            if (keyboard.active) {
                                inputField.forceActiveFocus();
                            }
                        }
                    }
                    inputField.onTextChanged: {
                        EntityController.configuredEntities.search(inputField.text);
                    }
                }
            }

            ListView {
                id: itemList
                width: parent.width
                height: parent.height - titleContainer.height
                anchors { horizontalCenter: parent.horizontalCenter; top: titleContainer.bottom }

                maximumFlickVelocity: 6000
                flickDeceleration: 1000
                highlightMoveDuration: 200

                model: EntityController.configuredEntities

                delegate: listItem
                footer: footer
                footerPositioning: ListView.OverlayFooter

                ScrollBar.vertical: ScrollBar {
                    opacity: 0.5
                }

                currentIndex: 0
            }

            MouseArea {
                anchors.fill: parent
                enabled: inputHasFocus
                onClicked: {
                    keyboard.hide();
                    inputHasFocus = false;
                }
            }
        }
    }

    Component {
        id: listItem

        Rectangle {
            width: ui.width; height: 100
            color: isCurrentItem && ui.keyNavigationEnabled ? colors.dark : colors.transparent

            property bool isCurrentItem: ListView.isCurrentItem
            property string key: itemKey
            property bool selected: itemSelected

            Components.Icon {
                id: entityIcon
                color: colors.offwhite
                icon: itemIcon
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter; }
                size: 80
            }

            Item {
                width: ui.width - entityIcon.size - 110
                height: childrenRect.height
                anchors { left: entityIcon.right; leftMargin: 10; verticalCenter: parent.verticalCenter; }

                Text {
                    id: entityTitle
                    text: itemName
                    width: parent.width
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    color: colors.offwhite
                    anchors { left: parent.left; }
                    font: fonts.primaryFont(26)
                    lineHeight: 0.8
                }

                Text {
                    id: entityIntegration
                    text: itemIntegration
                    width: parent.width
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.light
                    anchors { left: parent.left; top: entityTitle.bottom }
                    font: fonts.secondaryFont(22)
                }
            }

            Item {
                width: 100; height: 100
                anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 20 }

                states: [
                    State {
                        name: "selected"
                        when: itemSelected
                        PropertyChanges { target: small; width: 24 }
                        PropertyChanges { target: large; height: 48 }
                    }
                ]
                transitions: [
                    Transition {
                        to: "selected"
                        reversible: true
                        SequentialAnimation {
                            PropertyAnimation { target: small; properties: "width"; easing.type: Easing.OutExpo; duration: 75 }
                            PropertyAnimation { target: large; properties: "height"; easing.type: Easing.OutExpo; duration: 100 }
                        }
                    }
                ]

                Item {
                    anchors { horizontalCenter: parent.horizontalCenter; horizontalCenterOffset: -10; verticalCenter: parent.verticalCenter }
                    rotation: 45
                    transformOrigin: Item.Center

                    Rectangle {
                        id: small
                        width: 0
                        height: 4
                        color: colors.offwhite
                    }

                    Rectangle {
                        id: large
                        width: 4
                        height: 0
                        color: colors.offwhite
                        anchors { bottom: small.bottom; right: small.right }
                    }
                }
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    Haptic.play(Haptic.Click);
                    EntityController.configuredEntities.setSelected(itemKey, !itemSelected);
                }
            }
        }
    }

    Component {
        id: footer

        Rectangle {
            width: parent.width
            height: 100
            color: colors.black
            z: itemList.z + 1000

            Components.Button {
                text: qsTr("Cancel")
                width: (parent.width - 20 ) / 2
                color: colors.secondaryButton
                anchors { left: parent.left; bottom: parent.bottom }
                trigger: function() {
                    addGroupContainer.close();
                }
            }

            Components.Button {
                //: Button that will add the selected entities
                text: qsTr("Add")
                width: (parent.width - 20 ) / 2
                anchors { right: parent.right; bottom: parent.bottom }
                trigger: function() {
                    let selectedEntities = EntityController.configuredEntities.getSelected();

                    if (selectedEntities.length > 0) {
                        loading.start();
                        GroupController.updateGroup(addGroupContainer.groupId, ui.profile.id, "", selectedEntities)
                        EntityController.configuredEntities.clearSelected();
                    } else {
                        ui.createActionableNotification(qsTr("Select entities"), qsTr("Please select entities to add by tapping in the list."));
                    }
                }
            }
        }
    }
}
