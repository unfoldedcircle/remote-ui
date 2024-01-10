// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0
import QtQml.Models 2.1

import Entity.Controller 1.0
import Entity.MediaPlayer 1.0
import Group.Controller 1.0

import Config 1.0

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
    
    property bool activitiesBarEnabled: EntityController.activities.length > 0
    property QtObject currentActivity: activitiesBarEnabled ? EntityController.get(EntityController.activities[activitiesBarListView.currentIndex]) : QtObject

    property alias statusBar: statusBar
    property alias pages: pages
    property alias currentPage: pages.currentItem
    property QtObject currentEntity: pages.currentItem.currentItem

    property QtObject entityObjToEdit
    property QtObject groupObjToEdit

    property bool dpadMiddleLongPressed: false

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
                           icon: "uc:hamburger",
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
                           icon: "uc:about",
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
                           icon: "uc:edit",
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
                           icon: "uc:edit",
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

    
    onActivitiesBarEnabledChanged:  {
        if (activitiesBarEnabled) {
            currentActivity = EntityController.get(EntityController.activities[activitiesBarListView.currentIndex]);
        } else {
            currentActivity = null;
            activitiesBar.y = mainContainerRoot.height;
        }
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
                    pages.incrementCurrentIndex();
                    console.debug("Pages increment current index");
                }
            },
            "DPAD_LEFT": {
                "pressed": function() {
                    pages.decrementCurrentIndex();
                    console.debug("Pages decrement current index");
                }
            },
            // page navigation
            "DPAD_DOWN": {
                "pressed": function() {
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
                "released": function() {
                    if (mainContainerRoot.dpadMiddleLongPressed) {
                        mainContainerRoot.dpadMiddleLongPressed = false;
                        return;
                    }

                    if (currentEntity.delegateItem.groupObj) {
                        if (currentEntity.delegateItem.state === "closed") {
                            currentEntity.delegateItem.toggle();
                        } else {
                            if (Config.entityButtonFuncInverted) {
                                if (currentEntity.delegateItem.groups.currentItem.item.enabled) {
                                    currentEntity.delegateItem.groups.currentItem.item.open();
                                } else {
                                    ui.createNotification(currentEntity.delegateItem.groups.currentItem.item.name + " " + qsTr("is unavailable"), true);
                                }
                            } else {
                                currentEntity.delegateItem.groups.currentItem.item.controlTrigger();
                            }
                        }
                    } else {
                        if (Config.entityButtonFuncInverted) {
                            if (currentEntity.delegateItem.enabled) {
                                currentEntity.delegateItem.open();
                            } else {
                                ui.createNotification(currentEntity.delegateItem.name + " " + qsTr("is unavailable"), true);
                            }
                        } else {
                            currentEntity.delegateItem.controlTrigger();
                        }
                    }
                },
                "long_press": function() {
                    mainContainerRoot.dpadMiddleLongPressed = true;

                    if (!currentEntity.delegateItem.groupObj) {
                        if (Config.entityButtonFuncInverted) {
                            currentEntity.delegateItem.controlTrigger();
                        } else {
                           currentEntity.delegateItem.open();
                        }
                    } else {
                        if (Config.entityButtonFuncInverted) {
                            currentEntity.delegateItem.groups.currentItem.item.controlTrigger();
                        } else {
                            currentEntity.delegateItem.groups.currentItem.item.open();
                        }
                    }
                }
            },
            "CHANNEL_UP": {
                "released": function() {
                    if (currentEntity.delegateItem.groupObj) {
                        if (currentEntity.delegateItem.state === "open") {
                            currentEntity.delegateItem.close();
                        }
                    }
                }
            },
            "CHANNEL_DOWN": {
                "released": function() {
                    if (currentEntity.delegateItem.groupObj) {
                        if (currentEntity.delegateItem.state === "closed") {
                            currentEntity.delegateItem.open();
                        }
                    }
                }
            },
            "HOME": {
                "released": function() {
                    if (ui.editMode) {
                        ui.editMode = false;
                    }
                    pages.currentItem.currentIndex = 0;
                    pages.currentItem.positionViewAtBeginning();
                },
                "long_press": function() {
                    if (ui.profile.restricted) {
                        ui.createNotification(qsTr("Profile is restricted"), true);
                    } else {
                        openPageEditMenu();
                    }
                }
            }
        }
    }

    Components.ButtonNavigation {
        id: activitiesBarButtonNavigation
        overrideActive: ui.inputController.activeObject === String(mainContainerRoot) && activitiesBarEnabled
        defaultConfig: {
            "VOLUME_UP": {
                "pressed": function() {
                    currentActivity.volumeUp();
                    volume.start(currentActivity);
                }
            },
            "VOLUME_DOWN": {
                "pressed": function() {
                    currentActivity.volumeDown();
                    volume.start(currentActivity, false);
                }
            },
            "MUTE": {
                "released": function() {
                    currentActivity.muteToggle();
                }
            },
            "PLAY": {
                "released": function() {
                    currentActivity.playPause();
                }
            },
            "PREV": {
                "released": function() {
                    currentActivity.previous();
                }
            },
            "NEXT": {
                "released": function() {
                    currentActivity.next();
                }
            },
            "POWER": {
                "pressed": function() {
                    if (EntityController.activities.length === 0) {
                        return;
                    }

                    popupMenu.title = qsTr("Turn off");
                    let menuItems = [];

                    for (let i = 0; i<EntityController.activities.length; i++) {
                        let e = EntityController.activities[i];

                        menuItems.push({
                                           title: EntityController.get(e).name,
                                           icon: EntityController.get(e).icon,
                                           callback: function() {
                                               EntityController.get(e).turnOff();
                                           }
                                       });
                    }

                    if (EntityController.activities.length > 1) {
                        menuItems.push({
                                           title: qsTr("Turn off all"),
                                           icon: "uc:power-on",
                                           callback: function() {
                                               for (let i = 0; i<EntityController.activities.length; i++) {
                                                   EntityController.get(EntityController.activities[i]).turnOff();
                                               }
                                           }
                                       });
                    }

                    popupMenu.menuItems = menuItems;
                    popupMenu.open();
                }
            }
        }
    }

    Connections {
        target: currentPage
        ignoreUnknownSignals: true

        function onEditModeChanged() {
            if (currentPage.editMode) {
                activitiesBar.y = mainContainerRoot.height;
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
        delegate: Components.Page { width: PathView.view.width; height: PathView.view.height - (activitiesBar.opened ? activitiesBar.height : 0); anchors.top: parent.top }
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

    // activities bar
    Rectangle {
        id: activitiesBarIndicator
        width: 80
        height: 6
        radius: 3
        color: colors.offwhite
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: activitiesBarEnabled ? (activitiesBar.opened || (currentPage ? currentPage.editMode : false) ? -10 : 0) : -10 }

        Behavior on anchors.bottomMargin {
            NumberAnimation { easing.type: Easing.OutBack; easing.overshoot: 3; duration: 500 }
        }
    }

    MouseArea {
        width: ui.width
        height: activitiesBar.opened ? 120 : 60
        anchors.bottom: parent.bottom
        enabled: activitiesBarEnabled
        propagateComposedEvents: true //!activitiesBar.opened

        property real velocity: 0.0
        property int yStart: 0
        property int yPrev: 0
        property bool tracking: false
        property int treshold: 2

        onPressed: {
            yStart = mouseY;
            yPrev = mouseY;
            velocity = 0;
            tracking = true;
        }

        onCanceled: {
            tracking = false;
            mouse.accepted = false;
        }

        onPositionChanged: {
            let currentVelocity = (mouseY - yPrev);
            velocity = (velocity + currentVelocity) / 2.0
            yPrev = mouseY
        }

        onReleased: {
            tracking = false;

            if (velocity > treshold) {
                activitiesBar.y = mainContainerRoot.height;
            } else if (velocity < -treshold) {
                activitiesBar.y = mainContainerRoot.height - activitiesBar.height;
            } else {
                mouse.accepted = false;
            }
        }
    }

    Item {
        id: activitiesBar
        width: ui.width
        height: 100
        y: parent.height

        property bool opened: activitiesBar.y == mainContainerRoot.height - activitiesBar.height

        Behavior on y {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
        }

        ListView {
            id: activitiesBarListView
            anchors.fill: parent

            orientation: ListView.Horizontal
            snapMode: ListView.SnapOneItem
            highlightRangeMode: ListView.StrictlyEnforceRange
            maximumFlickVelocity: 6000
            flickDeceleration: 1000
            highlightMoveDuration: 200

            model: EntityController.activities
            delegate: activitiesBarDelegate

            onCurrentIndexChanged: {
                EntityController.load(EntityController.activities[activitiesBarListView.currentIndex]);

                connectSignalSlot(EntityController.entityLoaded, function(success, entityId) {
                    if (success) {
                        currentActivity = EntityController.get(EntityController.activities[activitiesBarListView.currentIndex]);
                    }
                });

                //                currentActivity = EntityController.get(EntityController.activities[activitiesBarListView.currentIndex]);
            }
        }

        PageIndicator {
            currentIndex: activitiesBarListView.currentIndex
            count: activitiesBarListView.count
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            visible: EntityController.activities.length > 1
            padding: 0

            delegate: Component {
                Rectangle {
                    width: 6; height: 6
                    radius: 3
                    color: colors.offwhite
                    opacity: index == activitiesBarListView.currentIndex ? 1 : 0.6
                }
            }
        }
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

    Component {
        id: activitiesBarDelegate

        Item {
            width: activitiesBarListView.width
            height: activitiesBarListView.height

            property QtObject entity: EntityController.get(modelData)

            Rectangle {
                id: activityBg
                width: parent.width
                height: parent.height - 10
                color: entity.mediaImageColor ? entity.mediaImageColor : colors.dark
                radius: ui.cornerRadiusSmall
                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top }

                Behavior on color {
                    ColorAnimation { duration: 300 }
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        loadSecondContainer("qrc:/components/entities/" + entity.getTypeAsString() + "/deviceclass/" + entity.getDeviceClass() + ".qml", { "entityId": entity.id, "entityObj": entity });
                    }

                }

                Components.Icon {
                    anchors.centerIn: albumArt
                    icon: entity.icon
                    size: 60
                    color: colors.offwhite
                }

                Text {
                    text: entity.name
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.offwhite
                    anchors { left: albumArt.right; leftMargin: 20; right: playPauseButton.left; rightMargin: 20; verticalCenter: albumArt.verticalCenter }
                    font: fonts.primaryFont(26)
                    visible: entity.type === EntityTypes.Activity
                }

                MediaPlayerComponents.ImageLoader {
                    id: albumArt
                    width: 60; height: 60
                    anchors { left: parent.left; leftMargin: 15; verticalCenter: parent.verticalCenter }
                    url: entity.mediaImage ? entity.mediaImage : ""

                    Behavior on opacity {
                        NumberAnimation { duration: 300 }
                    }
                }

                DropShadow {
                    anchors.fill: albumArt
                    horizontalOffset: 0
                    verticalOffset: 0
                    radius: 8.0
                    samples: 12
                    color: colors.black
                    source: albumArt
                    visible: entity.mediaImage ? true : false
                }

                Text {
                    text: entity.mediaTitle ? entity.mediaTitle : ""
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: Qt.lighter(activityBg.color, 5) //colors.offwhite
                    anchors { left: albumArt.right; leftMargin: 20; right: playPauseButton.left; rightMargin: 20; top: albumArt.top; topMargin: -2 }
                    font: fonts.primaryFont(26)
                    visible: entity.type === EntityTypes.Media_player
                }

                Text {
                    text: entity.name
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: Qt.lighter(activityBg.color, 3) //colors.light
                    anchors { left: albumArt.right; leftMargin: 20; right: playPauseButton.left; rightMargin: 20; bottom: albumArt.bottom; bottomMargin: -2 }
                    font: fonts.secondaryFont(22)
                    visible: entity.type === EntityTypes.Media_player
                }

                EntityComponents.BasePlayPauseButton {
                    id: playPauseButton
                    anchors { verticalCenter: parent.verticalCenter; right: parent.right }
                    checked: entity.state === MediaPlayerStates.Playing ? false : true
                    trigger: function() { entity.playPause() }
                    width: parent.height; height: width
                    visible: entity.type === EntityTypes.Media_player && entity.hasAllFeatures([MediaPlayerFeatures.Media_duration, MediaPlayerFeatures.Media_position]) && entity.mediaDuration !== 0
                }

                Rectangle {
                    id: progressBar
                    height: 4
                    radius: 2
                    anchors { bottom: parent.bottom; left: albumArt.left; right: parent.right; rightMargin: 15 }
                    color: colors.offwhite
                    opacity: 0.5
                    visible: entity.type === EntityTypes.Media_player
                }

                Rectangle {
                    visible: progressBar.visible
                    width: progressBar.width * entity.mediaPosition / entity.mediaDuration
                    height: progressBar.height
                    radius: 2
                    color: colors.offwhite
                    anchors { left: progressBar.left; verticalCenter: progressBar.verticalCenter }
                }
            }
        }
    }
}
