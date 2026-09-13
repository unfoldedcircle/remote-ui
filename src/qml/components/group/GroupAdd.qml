// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import "qrc:/components" as Components
import "qrc:/components/entities" as Entities

import Haptic 1.0
import Entity.Controller 1.0
import Group.Controller 1.0

Rectangle {
    id: addGroupContainer
    color: colors.black
    anchors.fill: parent
    // enabled by state, not by the finished fade-in: the dialog takes the input and focuses its
    // field as soon as it is shown, and the input controller drops a disabled owner right away
    enabled: state === "visible"

    signal closed()

    property string pageId
    property string groupId

    function open() {
        state = "visible";
    }

    function close() {
        state = "hidden";
    }

    function submitName() {
        if (inputFieldContainer.isEmpty()) {
            inputFieldContainer.showError();
        } else {
            GroupController.addGroup(ui.profile.id, inputFieldContainer.inputField.text)
        }
    }

    function addSelectedEntities() {
        let selectedEntities = EntityController.configuredEntities.getSelected();

        if (selectedEntities.length > 0) {
            loading.start();
            GroupController.updateGroup(addGroupContainer.groupId, ui.profile.id, "", selectedEntities)
            EntityController.configuredEntities.clearSelected();
        } else {
            ui.createActionableNotification(qsTr("Select entities"), qsTr("Please select entities to add by tapping in the list."));
        }
    }

    /** KEYBOARD NAVIGATION **/
    // Step 0 (the name) is a focus chain: field -> Next, LEFT to Cancel; Return on the field
    // submits it. Step 1 (the entities) is the entity list driven through the button navigation:
    // no control has the focus there, so the DPAD handlers only act on that step.
    onStateChanged: {
        if (state == "visible") {
            steps.currentIndex = 0;
            keyboard.show();
            // deferred: the dialog is shown by a key press that is still being delivered; the
            // field is focused after the input was taken, so the layer below keeps its own control
            // as the one to come back to
            Qt.callLater(function() {
                buttonNavigation.takeControl();
                inputFieldContainer.inputField.forceActiveFocus();
            });
        } else {
            keyboard.hide();
            buttonNavigation.releaseControl();
        }
    }

    Component.onCompleted: {
        const listConfig = entityList.keypadConfig();
        let config = {};

        for (const key of Object.keys(listConfig)) {
            const handler = listConfig[key].pressed;
            config[key] = {
                "pressed": function() {
                    if (steps.currentIndex === 1) {
                        handler();
                    }
                }
            };
        }

        buttonNavigation.extendDefaultConfig(config);
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
                entityList.resetSelection();
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
        manageFocus: true
        // no focused control on the entity step: the focus is parked on this inert item and its
        // keys go through the handlers above
        initialFocusItem: steps.currentIndex === 0 ? inputFieldContainer.inputField : buttonNavigation
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

        // the name field keeps the focus of step 0; on step 1 the focus is parked on the dialog
        onCurrentIndexChanged: {
            Qt.callLater(function() {
                if (!buttonNavigation.hasInputControl) {
                    return;
                }

                buttonNavigation.lastFocusItem = null;
                if (steps.currentIndex === 0) {
                    inputFieldContainer.inputField.forceActiveFocus();
                } else {
                    keyboard.hide();
                    buttonNavigation.forceActiveFocus();
                }
            });
        }

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
                inputField.onAccepted: addGroupContainer.submitName()
                moveInput: false
                keyboardFollowsFocus: true

                navDown: nextButton
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

                KeyNavigation.up: inputFieldContainer.inputField
                KeyNavigation.right: nextButton
            }

            Components.Button {
                id: nextButton
                text: qsTr("Next")
                width: parent.width / 2 - 10
                anchors { right: inputFieldContainer.right; top: inputFieldContainer.bottom; topMargin: 40 }
                trigger: function() {
                    addGroupContainer.submitName();
                }

                KeyNavigation.up: inputFieldContainer.inputField
                KeyNavigation.left: cancelButton
            }
        }

        // add entities
        Entities.EntityList {
            id: entityList
            title: qsTr("Select entities to add")
            model: EntityController.configuredEntities
            entityDescriptionIntegration: true
            closeListOnTrigger: false
            showCloseIcon: true
            //: Button that will add the selected entities
            okText: qsTr("Add")
            keypadSelected: steps.currentIndex === 1
            okTrigger: function() {
                addGroupContainer.addSelectedEntities();
            }
            closeTrigger: function() {
                addGroupContainer.close();
            }
        }
    }
}
