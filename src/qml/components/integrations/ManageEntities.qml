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

    // the keypad selection is on the tab bar (above the list's filter button)
    property bool tabsSelected: false

    function open() {
        manageEntities.tabsSelected = false;
        buttonNavigation.takeControl();
        entitySelectionList.open();
        configfuredEntitySelectionList.open();
    }

    function close() {
        entitySelectionList.close();
        configfuredEntitySelectionList.close();
        // deferred: closing on DPAD_MIDDLE (Add / Remove) hands the input back to the details
        // popup, which focuses its "Manage entities" opener - the same key press must not reach it
        Qt.callLater(buttonNavigation.releaseControl);
        manageEntities.closed();
        ui.setTimeOut(300, ()=>{ tabBar.currentIndex = 0; });
    }

    function currentList() {
        return entityListSwipeView.currentItem;
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_DOWN": {
                "pressed": function() {
                    if (manageEntities.tabsSelected) {
                        manageEntities.tabsSelected = false;
                        return;
                    }

                    manageEntities.currentList().moveSelection(1);
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    if (!manageEntities.tabsSelected && !manageEntities.currentList().moveSelection(-1)) {
                        // above the list's filter button
                        manageEntities.tabsSelected = true;
                    }
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (!manageEntities.tabsSelected) {
                        manageEntities.currentList().activateSelection();
                    }
                }
            },
            // LEFT / RIGHT switch the footer buttons while the selection is on them, the tabs otherwise
            "DPAD_LEFT": {
                "pressed": function() {
                    if (!manageEntities.currentList().moveFooter(-1)) {
                        tabBar.decrementCurrentIndex();
                    }
                }
            },
            "DPAD_RIGHT": {
                "pressed": function() {
                    if (!manageEntities.currentList().moveFooter(1)) {
                        tabBar.incrementCurrentIndex();
                    }
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

    Components.TitleBar {
        id: titleContainer
        anchors.top: parent.top
        action: "close"
        text: qsTr("Manage entities")
        goBack: function() {
            manageEntities.close();
        }
    }

    TabBar {
        id: tabBar
        width: parent.width - 40
        implicitHeight: 60

        anchors { horizontalCenter: parent.horizontalCenter; top: titleContainer.bottom; topMargin: 20 }

        background: Rectangle {
            color: colors.surfaceRaised
            radius: ui.cornerRadiusLarge
        }

        TabButton {
            id: availableTabButton
            //: Tab caption that contains available entities
            text: qsTr("Available: %1").arg(EntityController.availableEntities.count)
            implicitHeight: 60

            contentItem: Text {
                text: availableTabButton.text
                font: fonts.caption()
                color: colors.offwhite
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color: tabBar.currentIndex == 0 ? colors.buttonPrimary : colors.transparent
                radius: ui.cornerRadiusLarge

                Components.Selectable {
                    selected: manageEntities.tabsSelected && tabBar.currentIndex == 0
                    radius: parent.radius
                }
            }
        }

        TabButton {
            id: configuredTabButton
            //: Tab caption that contains configured entities
            text: qsTr("Configured: %1").arg(EntityController.configuredEntities.count)
            implicitHeight: 60

            contentItem: Text {
                text: configuredTabButton.text
                font: fonts.caption()
                color: colors.offwhite
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }

            background: Rectangle {
                color: tabBar.currentIndex == 1 ? colors.buttonPrimary : colors.transparent
                radius: ui.cornerRadiusLarge

                Components.Selectable {
                    selected: manageEntities.tabsSelected && tabBar.currentIndex == 1
                    radius: parent.radius
                }
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
            keypadSelected: entityListSwipeView.currentIndex === 0 && !manageEntities.tabsSelected

            okTrigger: function() {
                let selectedEntities = EntityController.availableEntities.getSelected();
                if (selectedEntities.length > 0) {
                    EntityController.configureEntities(manageEntities.integrationId, selectedEntities);
                    entitySelectionList.close();
                    configfuredEntitySelectionList.close();
                    EntityController.availableEntities.clearSelected();
                    manageEntities.close();
                } else {
                    ui.createActionableNotification(qsTr("Select entities"), qsTr("Please select the entities to add in the list."));
                }
            }
        }

        Entities.EntityList {
            id: configfuredEntitySelectionList
            model: EntityController.configuredEntities
            entityDescriptionIntegration: false
            closeListOnTrigger: false
            integrationId: manageEntities.integrationId
            keypadSelected: entityListSwipeView.currentIndex === 1 && !manageEntities.tabsSelected

            okTrigger: function() {
                let selectedEntities = EntityController.configuredEntities.getSelected();
                if (selectedEntities.length > 0) {
                    EntityController.deleteEntities(selectedEntities);
                    entitySelectionList.close();
                    configfuredEntitySelectionList.close();
                    EntityController.configuredEntities.clearSelected();
                    manageEntities.close();
                } else {
                    ui.createActionableNotification(qsTr("Select entities"), qsTr("Please select the entities to remove in the list."));
                }
            }
            okText: qsTr("Remove")
        }
    }
}
