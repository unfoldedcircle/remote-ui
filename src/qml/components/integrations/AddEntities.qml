// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Integration.Controller 1.0
import Entity.Controller 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as Entities

Item {
    id: integrationAddEntitiesContainer

    signal done
    signal home

    property string integrationId

    /** KEYBOARD NAVIGATION **/
    // The step owns the input while it is the current step of the setup (deferred, see Configure).
    // The entity list is driven through the button navigation: no control has the focus.
    readonly property bool isCurrentStep: SwipeView.isCurrentItem
    onIsCurrentStepChanged: Qt.callLater(activate)

    function activate() {
        if (integrationAddEntitiesContainer.isCurrentStep) {
            buttonNavigation.takeControl();
        } else {
            buttonNavigation.releaseControl();
        }
    }

    Connections {
        target: IntegrationController
        ignoreUnknownSignals: true

        function onIntegrationAdded(integrationId) {
            integrationAddEntitiesContainer.integrationId = integrationId;
            entitySelectionList.integrationId = integrationId;
            entitySelectionList.open();
        }
    }

    Component.onCompleted: buttonNavigation.extendDefaultConfig(entitySelectionList.keypadConfig())

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            // this step owns the input while it is shown: without these the keypad had no way
            // out of it. Leaving it skips adding entities, like its X icon.
            "BACK": {
                "pressed": function() {
                    integrationAddEntitiesContainer.done();
                }
            },
            "HOME": {
                "pressed": function() {
                    integrationAddEntitiesContainer.done();
                    integrationAddEntitiesContainer.home();
                }
            }
        }
    }

    Text {
        id: descriptionText
        width: parent.width - 20
        color: colors.offwhite
        opacity: 0.6
        text: qsTr("Select entities to control with the remote")
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        font: fonts.secondaryFont(24)
        anchors { top: parent.top; topMargin: 10; horizontalCenter: parent.horizontalCenter }
    }

    Entities.EntityList {
        id: entitySelectionList
        anchors { top: descriptionText.bottom; topMargin: 10; bottom: parent.bottom; left: parent.left; right: parent.right }
        model: EntityController.availableEntities
        entityDescriptionIntegration: false
        keypadSelected: true
        closeListOnTrigger: false

        okTrigger: function() {
            let selectedEntities = EntityController.availableEntities.getSelected();
            if (selectedEntities.length > 0) {
                EntityController.configureEntities(integrationId, selectedEntities);
                entitySelectionList.close();
                integrationAddEntitiesContainer.currentItem = false;
                integrationAddEntitiesContainer.done();
            } else {
                ui.createActionableNotification(qsTr("Select entities"), qsTr("Please select entities to add by tapping in the list."));
            }
        }
    }
}
