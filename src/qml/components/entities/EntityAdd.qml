// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Entity.Controller 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as Entities

Entities.EntityList {
    id: entitySelectionList

    property string pageId

    anchors.fill: parent
    title: qsTr("Add entities")
    model: EntityController.configuredEntities
    entityDescriptionIntegration: true
    closeListOnTrigger: false
    showCloseIcon: true

    openTrigger: function() {
        buttonNavigation.takeControl();
    }
    okTrigger: function() {
        let selectedEntities = EntityController.configuredEntities.getSelected();

        if (selectedEntities.length > 0) {
            entitySelectionList.close();
            loading.start();
            ui.pages.get(pageId).addEntities(selectedEntities);
            loading.stop();
            EntityController.configuredEntities.clearSelected();
            ui.updatePageItems(pageId);
        } else {
            ui.createActionableNotification(qsTr("Select entities"), qsTr("Please select entities to add by tapping in the list."));
        }
    }

    onClosed: buttonNavigation.releaseControl()


    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    entitySelectionList.close();
                }
            },
            "HOME": {
                "released": function() {
                    entitySelectionList.close();
                }
            },
            "DPAD_MIDDLE": {
                "released": function() {
                    entitySelectionList.okTrigger();
                }
            }
        }
    }
}
