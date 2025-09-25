// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0
import QtQml.Models 2.1

import Entity.Controller 1.0
import Entity.MediaPlayer 1.0
import Group.Controller 1.0
import Haptic 1.0

import Config 1.0
import HwInfo 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents
import "qrc:/components/entities/media_player" as MediaPlayerComponents

Item {
    id: mainContainerRoot
    width: parent.width; height: parent.height
    clip: true
    layer.enabled: true
    enabled: state === "visible" ? true : false
    
    Component.onCompleted: {
        buttonNavigation.takeControl();
    }

    property alias statusBar: statusBar
    property alias pages: pages
    property alias currentPage: pages.currentItem
    property QtObject currentEntity: pages.currentItem.currentItem

    property QtObject entityObjToEdit
    property QtObject groupObjToEdit

    property int menuShift: 150
    property double menuFade: 0

    function openPageEditMenu() {
        popupMenu.title = pages.currentItem.title;
        let menuItems = [];

        menuItems.push({
                           title: qsTr("Add entity"),
                           icon: "uc:plus",
                           callback: function() {
                               loadSecondContainer("qrc:/components/entities/EntityAdd.qml", { "pageId": currentPage._id });
                           }
                       });

        menuItems.push({
                           title: qsTr("Add group"),
                           icon: "uc:plus",
                           callback: function() {
                               loadSecondContainer("qrc:/components/group/GroupAdd.qml", { "pageId": currentPage._id });
                           }
                       });

        menuItems.push({
                           title: qsTr("Reorder"),
                           icon: "uc:bars",
                           callback: function() {
                               if (currentPage.count > 0) {
                                   ui.editMode = true;
                               } else {
                                   ui.createActionableNotification(qsTr("Page is empty"), qsTr("There is nothing to reorder. Try adding entities or groups first."))
                               }
                           }
                       });

        menuItems.push({
                           title: qsTr("Show tips"),
                           icon: "uc:circle-info",
                           callback: function() {
                               ui.showHelp = true;
                           }
                       });

        popupMenu.menuItems = menuItems;
        popupMenu.open();
    }

    function openEntityEditMenu(obj, parentGroupId) {
        mainContainerRoot.entityObjToEdit = obj;

        popupMenu.title = obj.name;
        let menuItems = [];

        menuItems.push({
                           title: qsTr("Rename"),
                           icon: "uc:pen-to-square",
                           callback: function() {
                               loadSecondContainer("qrc:/components/entities/EntityRename.qml", { "entityId": obj.id, "entityName": obj.name });
                           }
                       });

        menuItems.push({
                           title: qsTr("Change icon"),
                           icon: obj.icon,
                           callback: function() {
                               iconSelector.open();
                               entityIconChange.enabled = true;
                           }
                       });

        menuItems.push({
                           title: qsTr("Remove"),
                           icon: "uc:trash",
                           callback: function() {
                               if (parentGroupId !== "") {
                                   let group = GroupController.get(parentGroupId);
                                   group.removeEntity(obj.id);
                               } else {
                                   currentPage.items.removeItem(obj.id);
                               }
                               currentPage.items.removeItem(obj.id);
                               ui.updatePageItems(currentPage._id);
                           }
                       });

        popupMenu.menuItems = menuItems;
        popupMenu.open();
    }

    function openGroupEditMenu(obj) {
        mainContainerRoot.groupObjToEdit = obj;

        popupMenu.title = obj.groupName();
        let menuItems = [];

        menuItems.push({
                           title: qsTr("Rename"),
                           icon: "uc:pen-to-square",
                           callback: function() {
                               loadSecondContainer("qrc:/components/group/GroupRename.qml", { "groupId": obj.groupId(), "groupName": obj.groupName() });
                           }
                       });

        menuItems.push({
                           title: qsTr("Edit entities"),
                           icon: "uc:list",
                           callback: function() {
                               loadSecondContainer("qrc:/components/group/GroupEdit.qml", { "groupId": obj.groupId(), "groupName": obj.groupName(), "groupData": obj });
                           }
                       });

        menuItems.push({
                           title: qsTr("Delete"),
                           icon: "uc:trash",
                           callback: function() {
                               GroupController.deleteGroup(obj.groupId());
                               currentPage.items.removeItem(obj.groupId());
                               ui.updatePageItems(currentPage._id);
                           }
                       });

        popupMenu.menuItems = menuItems;
        popupMenu.open();
    }

    function closeMenu() {
        pages.anchors.topMargin = 0;
        pages.opacity = 1;
        statusBar.opacity = 1;
        mainContainerRoot.menuFade = 0;
        mainContainerBlockingMouseArea.enabled = false;
    }

    state: "visible"
    
    transform: Scale {
        origin.x: mainContainerRoot.width/2; origin.y: mainContainerRoot.height/2
    }
    
    states: [
        State {
            name: "hidden"
            PropertyChanges { target: mainContainerRoot; y: -140; scale: 0.6; opacity: 0 }
        },
        State {
            name: "visible"
            PropertyChanges { target: mainContainerRoot; scale: 1; opacity: 1 }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"
            
            ParallelAnimation {
                PropertyAnimation { target: mainContainerRoot; properties: "y, scale, opacity"; easing.type: Easing.OutExpo; duration: 300 }
            }
        },
        Transition {to: "visible";
            SequentialAnimation {
                PauseAnimation { duration: 200 }
                ParallelAnimation {
                    PropertyAnimation { target: mainContainerRoot; properties: "y, scale, opacity"; easing.type: Easing.OutExpo; duration: 300 }
                }
            }
        }
    ]

    onStateChanged: {
        if (mainContainerRoot.state === "visible") {
            buttonNavigation.takeControl();
        }
    }
    
    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "DPAD_RIGHT": {
                "pressed": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    pages.incrementCurrentIndex();
                    console.debug("Pages increment current index");
                }
            },
            "DPAD_LEFT": {
                "pressed": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    pages.decrementCurrentIndex();
                    console.debug("Pages decrement current index");
                }
            },
            // page navigation
            "DPAD_DOWN": {
                "pressed": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    if (!currentEntity.delegateItem.groupObj) {
                        pages.currentItem.incrementCurrentIndex();
                        console.debug("Entitylist increment current index");
                    } else if (currentEntity.delegateItem.groups.currentIndex === currentEntity.delegateItem.groups.count - 1 || currentEntity.delegateItem.state === "closed") {
                        pages.currentItem.incrementCurrentIndex();
                        console.debug("Entitylist increment current index");
                    } else {
                        currentEntity.delegateItem.groups.incrementCurrentIndex();
                    }
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    if (!currentEntity.delegateItem.groupObj) {
                        pages.currentItem.decrementCurrentIndex();
                        console.debug("Entitylist decrement current index");
                    }  else if (currentEntity.delegateItem.groups.currentIndex === 0 || currentEntity.delegateItem.state === "closed") {
                        pages.currentItem.decrementCurrentIndex();
                        console.debug("Entitylist decrement current index");
                    } else {
                        currentEntity.delegateItem.groups.decrementCurrentIndex();
                    }
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    if (!currentEntity.delegateItem.groupObj) {
                        if (Config.entityButtonFuncInverted) {
                            currentEntity.delegateItem.open();
                        } else {
                            currentEntity.delegateItem.controlTrigger();
                        }
                    } else {
                        if (currentEntity.delegateItem.state === "closed") {
                            if (Config.entityButtonFuncInverted) {
                                currentEntity.delegateItem.open();
                            } else {
                                currentEntity.delegateItem.toggle();
                            }
                        } else {
                            if (currentEntity.delegateItem.groups.currentItem.item.entityObj.state == 0) {
                                return;
                            }

                            if (Config.entityButtonFuncInverted) {
                                currentEntity.delegateItem.groups.currentItem.item.open();
                            } else {
                                currentEntity.delegateItem.groups.currentItem.item.controlTrigger();
                            }
                        }
                    }
                },
                "long_press": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    if (!currentEntity.delegateItem.groupObj) {
                        if (Config.entityButtonFuncInverted) {
                            currentEntity.delegateItem.controlTrigger();
                        } else {
                            currentEntity.delegateItem.open();
                        }
                    } else {
                        if (currentEntity.delegateItem.state === "closed") {
                            if (Config.entityButtonFuncInverted) {
                                currentEntity.delegateItem.toggle();
                            } else {
                                currentEntity.delegateItem.open();
                            }
                        } else {
                            if (currentEntity.delegateItem.groups.currentItem.item.entityObj.state == 0) {
                                return;
                            }

                            if (Config.entityButtonFuncInverted) {
                                currentEntity.delegateItem.groups.currentItem.item.controlTrigger();
                            } else {
                                currentEntity.delegateItem.groups.currentItem.item.open();
                            }
                        }
                    }
                }
            },
            "CHANNEL_UP": {
                "pressed": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    if (currentEntity.delegateItem.groupObj) {
                        if (currentEntity.delegateItem.state === "open") {
                            currentEntity.delegateItem.close();
                        }
                    }
                }
            },
            "CHANNEL_DOWN": {
                "pressed": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    if (currentEntity.delegateItem.groupObj) {
                        if (currentEntity.delegateItem.state === "closed") {
                            currentEntity.delegateItem.open();
                        }
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    if (ui.editMode) {
                        ui.editMode = false;
                    }
                    pages.currentItem.currentIndex = 0;
                    pages.currentItem.positionViewAtBeginning();
                },
                "long_press": function() {
                    if (mainContainerBlockingMouseArea.enabled) {
                        mainContainerRoot.closeMenu();
                        return;
                    }

                    if (ui.profile.restricted) {
                        ui.createNotification(qsTr("Profile is restricted"), true);
                    } else {
                        openPageEditMenu();
                    }
                }
            }
        }
    }

    Components.IconSelector {
        id: iconSelector

        Connections {
            id: entityIconChange
            target: iconSelector
            ignoreUnknownSignals: true
            enabled: false

            function onIconSelected(icon) {
                entityIconChange.enabled = false;
                EntityController.setEntityIcon(mainContainerRoot.entityObjToEdit.id, icon);
            }
        }
    }

    DelegateModel {
        id: visualModel

        model: ui.pages
        delegate: Components.Page { width: PathView.view.width; height: PathView.view.height; anchors.top: parent.top }
    }

    Item {
        id: mainMenu
        width: parent.width
        height: 150
        opacity: mainContainerRoot.menuFade * 2

        RowLayout {
            anchors.fill: parent
            anchors.topMargin: 35
            anchors.bottomMargin: 35

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80

                Components.HapticMouseArea {
                    width: 80
                    height: 80
                    anchors.centerIn: parent

                    Rectangle {
                        width: parent.width; height: parent.height
                        radius: width / 2
                        color: colors.offwhite
                        opacity: 0.1
                        anchors.centerIn: parent
                    }

                    Components.Icon {
                        Layout.alignment: Qt.AlignVCenter

                        color: colors.offwhite
                        icon: ui.profile.icon
                        size: 80
                    }

                    onClicked: {
                        mainContainerRoot.closeMenu();
                        loadSecondContainer("qrc:/components/ProfileSwitch.qml");
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                visible: !ui.profile.restricted

                Components.HapticMouseArea {
                    width: 80
                    height: 80
                    anchors.centerIn: parent

                    Rectangle {
                        width: parent.width; height: parent.height
                        radius: width / 2
                        color: colors.offwhite
                        opacity: 0.1
                        anchors.centerIn: parent
                    }

                    Components.Icon {
                        Layout.alignment: Qt.AlignVCenter

                        color: colors.offwhite
                        icon: "uc:globe-pointer"
                        size: 80
                    }

                    onClicked: {
                        mainContainerRoot.closeMenu();
                        loadSecondContainer("qrc:/components/WebConfig.qml");
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                visible: !ui.profile.restricted

                Components.HapticMouseArea {
                    width: 80
                    height: 80
                    anchors.centerIn: parent

                    Rectangle {
                        width: parent.width; height: parent.height
                        radius: width / 2
                        color: colors.offwhite
                        opacity: 0.1
                        anchors.centerIn: parent
                    }

                    Components.Icon {
                        Layout.alignment: Qt.AlignVCenter

                        color: colors.offwhite
                        icon: "uc:gear"
                        size: 80
                    }

                    onClicked: {
                        mainContainerRoot.closeMenu();
                        loadSecondContainer("qrc:/components/SettingsNew.qml");
                    }
                }
            }
        }
    }

    PathView {
        id: pages
        width: parent.width; height: parent.height
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        interactive: pages.count > 1

        snapMode: PathView.SnapToItem
        highlightRangeMode: PathView.StrictlyEnforceRange
        highlightMoveDuration: 200

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        model: visualModel
        cacheItemCount: 5

        path: Path {
            startX: -pages.width / 2 * (pages.count - 1)
            startY: pages.height / 2

            PathLine { x: -(pages.width / 2 * (pages.count - 1)) + (pages.width * pages.count); y: pages.height / 2 }
        }

        preferredHighlightEnd: 0.5
        preferredHighlightBegin: 0.5

        onCurrentIndexChanged: console.debug("Pages current index: " + currentIndex)

        Behavior on anchors.topMargin {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 500 }
        }

        Behavior on opacity {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 500 }
        }
    }

    Connections {
        target: pages.currentItem
        ignoreUnknownSignals: true

        function onDraggedDownYChanged(contentY, treshold) {
            if ((contentY < -treshold + 100) && !mainContainerBlockingMouseArea.enabled) {
                // start fading
                mainContainerRoot.menuFade = (100 - (Math.abs(treshold) - Math.abs(contentY))) / 200;
                pages.anchors.topMargin = 100 - (Math.abs(treshold) - Math.abs(contentY));
                pages.opacity = 1 - mainContainerRoot.menuFade;
                statusBar.opacity = 1 - mainContainerRoot.menuFade * 2;
            }

            if ((contentY < -treshold) && !mainContainerBlockingMouseArea.enabled) {
                Haptic.play(Haptic.Bump);
                pages.anchors.topMargin = mainContainerRoot.menuShift;
                mainContainerBlockingMouseArea.enabled = true;
                pages.opacity = 0.5;
                statusBar.opacity = 0;
            }

        }
    }

    // bottom gradient
    Item {
        width: parent.width; height: 60
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }
        opacity: pages.currentItem && pages.currentItem.atYEnd ? 0 : 1

        Behavior on opacity { PropertyAnimation { duration: 300; easing.type: Easing.OutExpo } }

        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(0, 50)
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: colors.black }
            }
        }
    }

    Components.StatusBar {
        id: statusBar
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
    }

    Components.PopupMenu {
        id: popupMenu
    }

    Loader {
        anchors.fill: parent
        active: ui.showHelp
        asynchronous: true
        source: "qrc:/components/help-overlay/Main.qml"
    }

    MouseArea {
        id: mainContainerBlockingMouseArea
        anchors.fill: pages
        enabled: false

        onClicked: mainContainerRoot.closeMenu()
    }
}
