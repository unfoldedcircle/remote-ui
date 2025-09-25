// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0
import Entity.Activity 1.0
import Entity.MediaPlayer 1.0
import Entity.Controller 1.0
import Config 1.0
import Wifi 1.0
import Wifi.SignalStrength 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: activityBase

    property var pages: entityObj.ui.pages
    property QtObject mediaWidgetEntityObj

    function triggerCommand(entityId, cmdId, params) {
        let e = EntityController.get(entityId);

        if (e.type === EntityTypes.Macro) {
            activityLoading.start(entityId, EntityTypes.Macro);
        }

        EntityController.onEntityCommand(
                    entityId,
                    cmdId,
                    (params ? params : {}))

        if (e.type === EntityTypes.Activity && cmdId === "activity.start") {
            loadActivityToSecondContainer(e);
            activityBase.close();
        }
    }

    function parsePageItems(page, index, container) {
        const items = page.items
        const gridWidth = page.grid ? page.grid.width : 4;
        const gridHeight = page.grid ? page.grid.height : 6;
        const gridSizeW = uiPages.width / gridWidth
        const gridSizeH = uiPages.height / gridHeight

        items.forEach(function(item) {
            switch (item.type) {
            case "text":
            case "icon":
                let component = Qt.createComponent("qrc:/components/ButtonIcon.qml");
                component.createObject(container, {
                                           "x": gridSizeW * item.location.x,
                                           "y": gridSizeH * item.location.y,
                                           "width": gridSizeW * (item.size ? (item.size.width ? item.size.width : 1) : 1),
                                           "height": gridSizeH * (item.size ? (item.size.height ? item.size.height : 1) : 1),
                                           "icon": item.icon ? item.icon : "",
                                           "text": item.text ? item.text : "",
                                           "trigger": function() {
                                               if (item.command.entity_id) {
                                                   if (!EntityController.get(item.command.entity_id)) {
                                                       EntityController.load(item.command.entity_id);
                                                       connectSignalSlot(EntityController.entityLoaded, function(success, entityId) {
                                                           activityBase.triggerCommand(item.command.entity_id, item.command.cmd_id, item.command.params);
                                                       });
                                                   } else {
                                                       activityBase.triggerCommand(item.command.entity_id, item.command.cmd_id, item.command.params);
                                                   }
                                               }
                                           }
                                       });

                break;
            case "media_player":
                let mediaComponent = Qt.createComponent("qrc:/components/entities/activity/MediaComponent.qml");
                mediaComponent.createObject(container, {
                                                "x": gridSizeW * item.location.x,
                                                "y": gridSizeH * item.location.y,
                                                "width": gridSizeW * (item.size ? (item.size.width ? item.size.width : 1) : 1),
                                                "height": gridSizeH * (item.size ? (item.size.height ? item.size.height : 1) : 1),
                                                "entityId": item.media_player_id,
                                                "gridWidth": gridWidth,
                                                "gridHeight": gridHeight,
                                                "aspectFit": !Config.fillMediaArtwork
                                            });
                activityBase.mediaWidgetEntityObj = EntityController.get(item.media_player_id);
                break;
            default:
                console.log("Not implemented item type: " + item.type);
                break;
            }
        });
    }

    function updateButtonMapping() {
        let overrideConfig = overrideConfigDefault;

        console.info("Setting up button mappings for activity: " + entityObj.name);
        entityObj.buttonMapping.forEach((buttonMap) => {
                                            if (!overrideConfig[buttonMap.button]) {
                                                overrideConfig[buttonMap.button] =  ({});
                                            }

                                            if (buttonMap.short_press) {
                                                EntityController.load(buttonMap.short_press.entity_id);
                                            }

                                            if (buttonMap.long_press) {
                                                EntityController.load(buttonMap.long_press.entity_id);
                                            }

                                            connectSignalSlot(EntityController.entityLoaded, function(success, entityId) {
                                                if (success) {
                                                    if (buttonMap.short_press) {
                                                        // let's setup short press first
                                                        let e = EntityController.get(buttonMap.short_press.entity_id);

                                                        const cmdString = String(buttonMap.short_press.cmd_id);
                                                        const canRepeat = !cmdString.includes("remote.");

                                                        console.info(entityObj.name + " button mapping short press: " + buttonMap.button + " -> " + JSON.stringify(buttonMap.short_press));

                                                        overrideConfig[buttonMap.button]["pressed"] = function() {
                                                            if (e) {
                                                                switch (e.type) {
                                                                case EntityTypes.Media_player:
                                                                    if (e.hasFeature(MediaPlayerFeatures.Volume_up_down)){
                                                                        if (buttonMap.button === "VOLUME_UP" || buttonMap.button === "VOLUME_DOWN") {
                                                                            volume.start(e, buttonMap.button === "VOLUME_UP");
                                                                        }
                                                                    }
                                                                    activityBase.triggerCommand(buttonMap.short_press.entity_id, buttonMap.short_press.cmd_id, buttonMap.short_press.params);
                                                                    break;
                                                                case EntityTypes.Remote: {
                                                                    EntityController.onEntityCommand(
                                                                                e.id,
                                                                                buttonMap.short_press.cmd_id,
                                                                                buttonMap.short_press.params ? buttonMap.short_press.params : {});
                                                                    break;
                                                                }
                                                                default:
                                                                    activityBase.triggerCommand(buttonMap.short_press.entity_id, buttonMap.short_press.cmd_id, buttonMap.short_press.params);
                                                                    break;
                                                                }
                                                            } else {
                                                                console.warn("Entity " + entityId + " is not loaded. Button mapping failed for press: " + buttonMap.button);
                                                            }
                                                        }

                                                        overrideConfig[buttonMap.button]["pressed_repeat"] = function() {
                                                            if (e) {
                                                                switch (e.type) {
                                                                case EntityTypes.Media_player:
                                                                    if (e.hasFeature(MediaPlayerFeatures.Volume_up_down)){
                                                                        if (buttonMap.button === "VOLUME_UP" || buttonMap.button === "VOLUME_DOWN") {
                                                                            volume.start(e, buttonMap.button === "VOLUME_UP");
                                                                        }
                                                                    }
                                                                    activityBase.triggerCommand(buttonMap.short_press.entity_id, buttonMap.short_press.cmd_id, buttonMap.short_press.params);
                                                                    break;
                                                                case EntityTypes.Remote: {
                                                                    if (canRepeat) {
                                                                        EntityController.onEntityCommand(
                                                                                    e.id,
                                                                                    "remote.send",
                                                                                    {
                                                                                        "command": buttonMap.short_press.cmd_id,
                                                                                        "repeat": ui.inputController.repeatCount
                                                                                    });
                                                                    } else {
                                                                        EntityController.onEntityCommand(
                                                                                    e.id,
                                                                                    buttonMap.short_press.cmd_id,
                                                                                    buttonMap.short_press.params ? buttonMap.short_press.params : {});
                                                                    }
                                                                    break;
                                                                }
                                                                default:
                                                                    activityBase.triggerCommand(buttonMap.short_press.entity_id, buttonMap.short_press.cmd_id, buttonMap.short_press.params);
                                                                    break;
                                                                }
                                                            }  else {
                                                                console.warn("Entity " + entityId + " is not loaded. Button mapping failed for press repeat: " + buttonMap.button);
                                                            }
                                                        }

                                                        // for the remote entity we need to define a release event as well
                                                        overrideConfig[buttonMap.button]["released"] = function() {
                                                            if (e) {
                                                                if (e.type === EntityTypes.Remote && canRepeat) {
                                                                    EntityController.onEntityCommand(
                                                                                e.id,
                                                                                "remote.stop_send",
                                                                                {});
                                                                }
                                                            } else {
                                                                console.warn("Entity " + entityId + " is not loaded. Button mapping failed for release: " + buttonMap.button);
                                                            }
                                                        }
                                                    }

                                                    if (buttonMap.long_press) {
                                                        // now tackle the long press
                                                        let eL = EntityController.get(buttonMap.long_press.entity_id);

                                                        console.info(entityObj.name + " button mapping long press: " + buttonMap.button + " -> " + JSON.stringify(buttonMap.long_press));

                                                        overrideConfig[buttonMap.button]["long_press"] = function() {
                                                            if (eL) {
                                                                activityBase.triggerCommand(buttonMap.long_press.entity_id, buttonMap.long_press.cmd_id, buttonMap.long_press.params);
                                                            }  else {
                                                                console.warn("Entity " + entityId + " is not loaded. Button mapping failed for long press: " + buttonMap.button);
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    console.warn("Entity " + entityId + " is not loaded. Button mapping failed.");
                                                }
                                            })
                                        });

        buttonNavigation.overrideConfig = overrideConfig;
    }

    Component.onCompleted: updateButtonMapping()

    Connections {
        target: entityObj
        ignoreUnknownSignals: true

        function onButtonMappingChanged() {
            updateButtonMapping();
        }

        function onUiConfigChanged() {
            activityBase.pages = entityObj.ui.pages;
        }
    }

    property var overrideConfigDefault: {
        "DPAD_LEFT": {
            "pressed": function() {
                uiPages.decrementCurrentIndex();
            }
        },
        "DPAD_RIGHT": {
            "pressed": function() {
                uiPages.incrementCurrentIndex();
            }
        },
        "POWER": {
            "pressed": function() {
                entityObj.turnOff();
                activityBase.close();
            }
        }
    }

    Rectangle {
        id: title
        width: parent.width
        height: 80
        color: entityObj.state === ActivityStates.Error ? colors.red : colors.transparent

        Components.Icon {
            id: iconOpen
            icon: entityObj.icon
            color: colors.offwhite
            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
            size: 70
        }

        Item {
            width: parent.width - 200
            height: childrenRect.height
            anchors { left: iconOpen.right; leftMargin: 10; verticalCenter: parent.verticalCenter; }

            Text {
                id: titleOpen
                text: entityObj.name
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                elide: Text.ElideRight
                maximumLineCount: 1
                color: colors.offwhite
                opacity: iconOpen.opacity
                font: fonts.primaryFont(24, "Medium")
            }

            Text {
                id: titleDesc
                //: Tap to close menu or tap to see more
                text: activityMenu.opened ? qsTr("Tap to close") : qsTr("Tap for more")
                height: visible ? implicitHeight : 0
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                elide: Text.ElideRight
                maximumLineCount: 1
                color: colors.light
                opacity: iconOpen.opacity
                anchors { left: parent.left; top: titleOpen.bottom; topMargin: -5 }
                font: fonts.secondaryFont(20, "Medium")
            }
        }

        Components.HapticMouseArea {
            anchors.fill: parent
            onClicked: {
                if (activityMenu.opened) {
                    activityMenu.close();
                    extraContent.decrementCurrentIndex();
                    activityBase.buttonNavigation.takeControl();
                } else {

                    activityMenu.open();
                }
            }
        }

        Components.Icon {
            icon: "uc:wifi"
            color: colors.offwhite
            opacity: 0.5
            size: 60
            anchors { right: parent.right; rightMargin: 60; verticalCenter: parent.verticalCenter }
            visible: !Wifi.isConnected || Wifi.currentNetwork.signalStrength === SignalStrength.NONE ||  Wifi.currentNetwork.signalStrength === SignalStrength.WEAK

            Components.Icon {
                size: 60
                icon: {
                    switch (Wifi.currentNetwork.signalStrength) {
                    case SignalStrength.NONE:
                        return "";
                    case SignalStrength.WEAK:
                        return "uc:wifi-weak";
                    default:
                        return "";
                    }
                }
                opacity: icon === "" ? 0 : 1
                anchors.centerIn: parent
            }

            Rectangle {
                width: 30
                height: 2
                color: colors.red
                rotation: -45
                transformOrigin: Item.Center
                anchors.centerIn: parent
                visible: !Wifi.isConnected
            }
        }
    }

    PathView {
        id: uiPages
        width: parent.width
        height: parent.height - title.height
        anchors { horizontalCenter: parent.horizontalCenter; top: title.bottom }
        interactive: uiPages.count > 1

        snapMode: PathView.SnapToItem
        highlightRangeMode: PathView.StrictlyEnforceRange
        highlightMoveDuration: 200

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        model: pages
        cacheItemCount: 5
        delegate: uiPage

        path: Path {
            startX: -uiPages.width / 2 * (uiPages.count - 1)
            startY: uiPages.height / 2

            PathLine { x: -(uiPages.width / 2 * (uiPages.count - 1)) + (uiPages.width * uiPages.count); y: uiPages.height / 2 }
        }

        preferredHighlightEnd: 0.5
        preferredHighlightBegin: 0.5
    }

    PageIndicator {
        currentIndex: uiPages.currentIndex
        count: uiPages.count
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        visible: uiPages.count > 1
        padding: 0

        delegate: Component {
            Rectangle {
                width: 12; height: 12
                radius: 6
                color: colors.offwhite
                opacity: index == uiPages.currentIndex ? 1 : 0.6
            }
        }
    }

    EntityComponents.DropDownMenu {
        id: activityMenu

        SwipeView {
            id: extraContent
            anchors.fill: parent
            interactive: false

            Item {
                Components.HapticMouseArea {
                    id: fixStatesButton
                    width: parent.width
                    height: 100

                    onClicked: {
                        extraContent.incrementCurrentIndex();
                    }

                    Components.Icon {
                        id: iconIssue
                        color: colors.offwhite
                        icon: "uc:triangle-exclamation"
                        anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                        size: 80
                    }

                    Text {
                        //: Title referring to fixing device states that might out of sync
                        text: qsTr("Fix states")
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        color: colors.offwhite
                        anchors { left: iconIssue.right; leftMargin: 20; verticalCenter: parent.verticalCenter }
                        font: fonts.primaryFont(30)
                    }

                    Components.Icon {
                        color: colors.offwhite
                        icon: "uc:arrow-right"
                        anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                        size: 80
                    }
                }

                Text {
                    id: entityListTitle
                    wrapMode: Text.WordWrap
                    color: colors.light
                    text: qsTr("Quickly access entities included in this activity:")
                    anchors { left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20; top: fixStatesButton.bottom; topMargin: 20 }
                    font: fonts.secondaryFont(24)
                    lineHeight: 0.8
                }

                ListView {
                    id: includedEntitiesList
                    anchors { left: parent.left; right: parent.right; top: entityListTitle.bottom; topMargin: 30; bottom: parent.bottom }

                    highlightMoveDuration: 200
                    maximumFlickVelocity: 6000
                    flickDeceleration: 1000

                    model: entityObj.includedEntities
                    delegate: includedEntityItem
                    clip: true
                }
            }

            Item {
                Components.HapticMouseArea {
                    id: backButton
                    width: parent.width
                    height: 100
                    anchors.top: parent.top

                    onClicked: {
                        extraContent.decrementCurrentIndex();
                    }

                    Components.Icon {
                        id: iconIssueBack
                        color: colors.offwhite
                        icon: "uc:arrow-left"
                        anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                        size: 80
                    }

                    Text {
                        //: Caption to go back
                        text: qsTr("Back")
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        color: colors.offwhite
                        anchors { left: iconIssueBack.right; leftMargin: 20; verticalCenter: parent.verticalCenter }
                        font: fonts.primaryFont(30)
                    }
                }

                ListView {
                    id: fixedEntitiesList
                    anchors { top: backButton.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }

                    highlightMoveDuration: 200
                    maximumFlickVelocity: 6000
                    flickDeceleration: 1000
                    clip: true

                    model: entityObj.includedEntities
                    delegate: fixedEntityItem
                }
            }
        }
    }

    Component {
        id: uiPage

        Item {
            id: gridContainer
            width: uiPages.width
            height: uiPages.height

            Component.onCompleted: {
                parsePageItems(pages[index], index, gridContainer)
            }

            Text {
                id: noComponentstitle
                text: qsTr("Empty page")
                color: colors.offwhite
                font: fonts.secondaryFont(26)
                width: parent.width - 40
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -100
                visible: pages[index].items.length === 0
            }

            Text {
                text: qsTr("You can add UI elements via the Web Configurator")
                color: colors.light
                font: fonts.secondaryFont(26)
                width: parent.width - 40
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                anchors { top: noComponentstitle.bottom; topMargin: 10; horizontalCenter: parent.horizontalCenter }
                visible: pages[index].items.length === 0
            }
        }
    }

    Component {
        id: includedEntityItem

        Components.HapticMouseArea {
            width: includedEntitiesList.width
            height: 100

            Component.onCompleted: {
                entity = EntityController.get(modelData)

                if (!entity) {
                    connectSignalSlot(EntityController.entityLoaded, function(success, entityId) {
                        if (success) {
                            entity = EntityController.get(entityId);
                        }
                    });
                    EntityController.load(modelData);
                }
            }

            property QtObject entity

            onClicked: {
                loadThirdContainer("qrc:/components/entities/" + entity.getTypeAsString() + "/deviceclass/" + entity.getDeviceClass() + ".qml", { "entityId": entity.id, "entityObj": entity });
            }

            Components.Icon {
                id: includedEntityItemIcon
                color: colors.offwhite
                icon: entity.icon
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                size: 80
            }

            Text {
                text: entity.name
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                elide: Text.ElideRight
                maximumLineCount: 2
                color: colors.offwhite
                anchors { left: includedEntityItemIcon.right; leftMargin: 20; right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
                font: fonts.primaryFont(30)
            }
        }
    }

    Component {
        id: fixedEntityItem

        Components.HapticMouseArea {
            width: fixedEntitiesList.width
            height: fixedEntityItemIcon.size / 2 + fixedEntityItemData.height

            Component.onCompleted: {
                entity = EntityController.get(modelData)

                if (!entity) {
                    connectSignalSlot(EntityController.entityLoaded, function(success, entityId) {
                        if (success) {
                            entity = EntityController.get(entityId);
                        }
                    });
                    EntityController.load(modelData);
                }
            }

            property QtObject entity

            onClicked: {
                ui.createNotification("Not yet implemented");
            }

            Components.Icon {
                id: fixedEntityItemIcon
                color: colors.offwhite
                icon: entity.icon
                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                size: 80
            }

            Item {
                id: fixedEntityItemData
                height: childrenRect.height
                anchors { left: fixedEntityItemIcon.right; leftMargin: 20; right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }

                Text {
                    id: fixedEntityItemName
                    text: entity.name
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    color: colors.offwhite
                    anchors { left: parent.left; right: parent.right }
                    font: fonts.primaryFont(30)
                }

                Text {
                    id: fixedEntityItemState
                    //: Device state
                    text: qsTr("State: %1").arg(entity ? entity.stateAsString : "")
                    width: parent.width
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: colors.light
                    anchors { left: parent.left; top: fixedEntityItemName.bottom }
                    font: fonts.secondaryFont(22)
                }
            }
        }
    }

    Components.TouchSlider {
        id: touchSlider
        entityObj: activityBase.mediaWidgetEntityObj
        active: activityBase.mediaWidgetEntityObj.hasFeature(MediaPlayerFeatures.Volume)
        parent: Overlay.overlay
    }
}
