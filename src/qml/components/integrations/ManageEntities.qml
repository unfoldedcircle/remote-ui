// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0

import Entity.Controller 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as Entities

Item {
    id: manageEntities
    anchors.fill: parent

    signal closed()

    property string integrationId
    property alias entityListSwipeView: entityListSwipeView

    function open() {
        buttonNavigation.takeControl();
        entitySelectionList.open();
        configfuredEntitySelectionList.open();
    }

    function close() {
        entitySelectionList.close();
        configfuredEntitySelectionList.close();
        buttonNavigation.releaseControl();
        manageEntities.closed();
        ui.setTimeOut(300, ()=>{ tabBar.currentIndex = 0; });
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_DOWN": {
                "pressed": function() {
                    entityListSwipeView.currentItem.itemList.incrementCurrentIndex();
                    entityListSwipeView.currentItem.itemList.positionViewAtIndex(entityListSwipeView.currentItem.itemList.currentIndex, ListView.Contain);
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    entityListSwipeView.currentItem.itemList.decrementCurrentIndex();
                    entityListSwipeView.currentItem.itemList.positionViewAtIndex(entityListSwipeView.currentItem.itemList.currentIndex, ListView.Contain);
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    entityListSwipeView.currentItem.itemSelected(
                                entityListSwipeView.currentItem.itemList.currentItem.key,
                                !entityListSwipeView.currentItem.itemList.currentItem.selected);
                }
            },
            "DPAD_LEFT": {
                "pressed": function() {
                    tabBar.decrementCurrentIndex();
                }
            },
            "DPAD_RIGHT": {
                "pressed": function() {
                    tabBar.incrementCurrentIndex();
                }
            },
            "BACK": {
                "pressed": function() {
                    manageEntities.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    manageEntities.close();
                }
            }
        }
    }

    Item {
        id: titleContainer
        width: parent.width
        height: 60
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

        Text {
            id: titleText
            text: qsTr("Manage entities")
            width: parent.width - 40
            wrapMode: Text.NoWrap
            elide: Text.ElideRight
            maximumLineCount: 1
            color: colors.offwhite
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(30)
        }

        Components.Icon {
            color: colors.offwhite
            icon: "uc:xmark"
            anchors { verticalCenter: parent.verticalCenter; right: parent.right }
            size: 60

            Components.HapticMouseArea {
                width: parent.width + 20; height: width
                anchors.centerIn: parent
                onClicked: {
                    manageEntities.close();
                }
            }
        }
    }

    TabBar {
        id: tabBar
        width: parent.width - 20
        implicitHeight: 60

        anchors { horizontalCenter: parent.horizontalCenter; top: titleContainer.bottom; topMargin: 20 }

        background: Rectangle {
            color: colors.dark
            radius: ui.cornerRadiusLarge
            border {
                color: colors.medium
                width: 1
            }
        }

        TabButton {
            id: availableTabButton
            //: Tab caption that contains available entities
            text: qsTr("Available: %1").arg(EntityController.availableEntities.count)
            implicitHeight: 60

            contentItem: Text {
                text: availableTabButton.text
                font: fonts.secondaryFont(22)
                color: colors.offwhite
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color: tabBar.currentIndex == 0 ? colors.primaryButton : colors.transparent
                radius: ui.cornerRadiusLarge
            }
        }

        TabButton {
            id: configuredTabButton
            //: Tab caption that contains configured entities
            text: qsTr("Configured: %1").arg(EntityController.configuredEntities.count)
            implicitHeight: 60

            contentItem: Text {
                text: configuredTabButton.text
                font: fonts.secondaryFont(22)
                color: colors.offwhite
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color: tabBar.currentIndex == 1 ? colors.primaryButton : colors.transparent
                radius: ui.cornerRadiusLarge
            }
        }
    }

    SwipeView {
        id: entityListSwipeView
        width: parent.width
        anchors { horizontalCenter: parent.horizontalCenter; top: tabBar.bottom; bottom: parent.bottom }

        currentIndex: tabBar.currentIndex
        interactive: false
        clip: true

        Entities.EntityList {
            id: entitySelectionList
            model: EntityController.availableEntities
            entityDescriptionIntegration: false
            closeListOnTrigger: false
            integrationId: manageEntities.integrationId

            okTrigger: function() {
                let selectedEntities = EntityController.availableEntities.getSelected();
                if (selectedEntities.length > 0) {
                    EntityController.configureEntities(manageEntities.integrationId, selectedEntities);
                    entitySelectionList.close();
                    configfuredEntitySelectionList.close();
                    EntityController.availableEntities.clearSelected();
                    manageEntities.close();
                } else {
                    ui.createActionableNotification(qsTr("Select entities"), qsTr("Please select entities to add by tapping in the list."));
                }
            }
        }

        Entities.EntityList {
            id: configfuredEntitySelectionList
            model: EntityController.configuredEntities
            entityDescriptionIntegration: false
            closeListOnTrigger: false
            integrationId: manageEntities.integrationId

            okTrigger: function() {
                let selectedEntities = EntityController.configuredEntities.getSelected();
                if (selectedEntities.length > 0) {
                    EntityController.deleteEntities(selectedEntities);
                    entitySelectionList.close();
                    configfuredEntitySelectionList.close();
                    EntityController.configuredEntities.clearSelected();
                    manageEntities.close();
                } else {
                    ui.createActionableNotification(qsTr("Select entities"), qsTr("Please select entities to remove by tapping in the list."));
                }
            }
            okText: qsTr("Remove")
        }
    }
}
