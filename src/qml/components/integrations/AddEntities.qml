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

    property string integrationId
    property bool currentItem: false

    onCurrentItemChanged: {
        if (integrationAddEntitiesContainer.currentItem) {
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

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_DOWN": {
                "pressed": function() {
                    entitySelectionList.itemList.incrementCurrentIndex();
                    entitySelectionList.itemList.positionViewAtIndex(entitySelectionList.itemList.currentIndex, ListView.Contain);
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    entitySelectionList.itemList.decrementCurrentIndex();
                    entitySelectionList.itemList.positionViewAtIndex(entitySelectionList.itemList.currentIndex, ListView.Contain);
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    const item = entitySelectionList.itemList.currentItem;
                    if (!item) {
                        return;
                    }

                    entitySelectionList.itemSelected(item.key, !item.selected);
                }
            },
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
