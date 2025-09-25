// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Entity.Controller 1.0
import Entity.Button 1.0
import Entity.Light 1.0
import Entity.Switch 1.0
import Entity.Climate 1.0
import Entity.Cover 1.0
import Entity.MediaPlayer 1.0
import Entity.Remote 1.0
import Entity.Activity 1.0

import Haptic 1.0

import "qrc:/components" as Components
import "qrc:/components/entities/media_player" as MediaPlayerComponents

Rectangle {
    id: entityBaseContainer
    width: isInGroup ? ui.width - 40 : ui.width - 20; height: 130
    color: isSelected && !editMode ? Qt.darker(colors.medium) : colors.black
    opacity: entityObj.enabled ? 1 : 0.5
    radius: ui.cornerRadiusSmall
    border {
        color: (isInGroup || isSelected) && !editMode ? colors.medium : colors.transparent
        width: 1
    }

    property string entityId
    property QtObject entityObj: QtObject {
        property string name
        property string icon
        property string stateAsString
        property string stateInfo
        property bool enabled
        property int state
        property int type
        property string mediaImage
    }

    property bool isHighLightEnabled: true
    property bool isSelected: isHighLightEnabled ? parent.isCurrentItem : false
    property bool isInGroup: false
    property string parentGroupId
    property bool editMode
    property bool iconOn: false
    property var controlTrigger: function() {}

    property alias button: button

    function handleActivityOpen() {
        if (entityBaseContainer.entityObj.type === EntityTypes.Activity) {
            switch (entityObj.state) {
            case ActivityStates.Off:
                entityObj.turnOn();
                return false;
            case ActivityStates.Error:
                popupMenu.title = qsTr("Activity error. Select option below.");
                let menuItems = [];
                menuItems.push({
                                   title: qsTr("Turn activity on"),
                                   icon: "uc:arrow-right",
                                   callback: function() {
                                       entityObj.turnOn();
                                   }
                               });
                menuItems.push({
                                   title: qsTr("Turn activity off"),
                                   icon: "uc:arrow-left",
                                   callback: function() {
                                       entityObj.turnOff();
                                   }
                               });
                popupMenu.menuItems = menuItems;
                popupMenu.open();
                return false;
            case ActivityStates.On:
                if (!entityObj.enabled) {
                    ui.createNotification(entityObj.name + " " + qsTr("is unavailable"), true);
                    return false;
                }
                break;
            }

            return true;
        }

        return true;
    }

    function open() {
        if (entityBaseContainer.handleActivityOpen()) {
            loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityId, "entityObj": entityObj });
        }
    }

    function build() {
        switch (entityBaseContainer.entityObj.type) {
        case EntityTypes.Button:
            entityBaseContainer.iconOn = Qt.binding( function() { return (entityObj.state === ButtonStates.Available || entityObj.state === ButtonStates.On) ? true : false });
            entityBaseContainer.controlTrigger = function() { entityObj.push(); }
            button.checked = false;
            break;

        case EntityTypes.Switch:
            entityBaseContainer.iconOn = Qt.binding( function() { return entityObj.state === SwitchStates.On });

            if (entityObj.hasAnyFeature([SwitchFeatures.On_off, SwitchFeatures.Toggle])) {
                entityBaseContainer.controlTrigger = function() { entityObj.toggle(); };
            }

            button.checked = Qt.binding(()=>{ return entityObj.state === SwitchStates.On; });
            break;

        case EntityTypes.Climate:
            entityBaseContainer.iconOn = Qt.binding( function() { return entityObj.state !== ClimateStates.Off });
            button.checked = Qt.binding(()=>{ return entityObj.state !== ClimateStates.Off; });
            break;

        case EntityTypes.Cover:
            entityBaseContainer.iconOn = Qt.binding( function() { return entityObj.state === CoverStates.Closed });


            if (entityObj.hasAllFeatures([CoverFeatures.Open, CoverFeatures.Close])) {
                entityBaseContainer.controlTrigger = function() {
                    if (entityObj.state === CoverStates.Open) {
                        entityObj.close();
                    } else if (entityObj.state === CoverStates.Closed) {
                        entityObj.open();
                    }
                }
            }

            button.checked = Qt.binding(()=>{ return entityObj.state === CoverStates.Open; });
            break;

        case EntityTypes.Light:
            entityBaseContainer.iconOn = Qt.binding( function() { return entityObj.state === LightStates.On ? true : false });
            entityBaseContainer.controlTrigger = function() { entityObj.toggle(); }
            button.checked = Qt.binding(()=>{ return entityObj.state === LightStates.On; });
            break;

        case EntityTypes.Media_player:
            entityBaseContainer.iconOn = Qt.binding( function() { return entityObj.state !== MediaPlayerStates.Off });
            entityBaseContainer.controlTrigger = function() {
                if (entityObj.state === MediaPlayerStates.Off) {
                    entityObj.turnOn();
                } else {
                    entityObj.playPause();
                }
            }
            button.checked = Qt.binding(()=>{ return entityObj.state !== MediaPlayerStates.Off; });
            break;

        case EntityTypes.Remote:
            entityBaseContainer.iconOn = Qt.binding( function() { return entityObj.state === RemoteStates.On ? true : false });
            entityBaseContainer.controlTrigger = function() { entityObj.toggle(); }
            button.checked = false;
            break;

        case EntityTypes.Activity:
            entityBaseContainer.iconOn = Qt.binding( function() { return entityObj.state === ActivityStates.On ? true : false });
            entityBaseContainer.controlTrigger = function() {
                entityBaseContainer.handleActivityOpen();
            }
            button.checked = Qt.binding(()=>{ return entityObj.state === ActivityStates.On; });
            break;

        case EntityTypes.Macro:
            entityBaseContainer.iconOn = true;
            entityBaseContainer.controlTrigger = function() {
                activityLoading.start(entityId, EntityTypes.Macro);
                entityObj.run();
            }
            button.checked = false;
            break;

        case EntityTypes.Sensor:
            entityBaseContainer.iconOn = true;
            button.checked = false;
            break;
        }
    }

    Behavior on opacity {
        NumberAnimation { duration: 300 }
    }

    Connections {
        id: entityControllerConnection
        target: EntityController
        ignoreUnknownSignals: true

        function onEntityLoaded(success, entityId) {
            if (success && entityBaseContainer.entityId === entityId) {
                console.debug("ENTITY LOADED: " + entityId);
                entityControllerConnection.enabled = false;
                entityBaseContainer.entityObj = EntityController.get(entityBaseContainer.entityId);
                entityBaseContainer.build();
            }
        }
    }

    Connections {
        target: ui
        ignoreUnknownSignals: true

        function onEditModeChanged(value) {
            editMode = value;
        }
    }

    AbstractButton {
        id: button
    }

    Components.HapticMouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {           
            if (entityObj.enabled) {
                if (!editMode) {
                    entityBaseContainer.open();
                }
            }
        }
        onPressAndHold: {
            if (ui.profile.restricted) {
                ui.createNotification(qsTr("Profile is restricted"), true);
            } else {
                root.containerMainItem.openEntityEditMenu(entityObj, entityBaseContainer.parentGroupId);
            }
        }
    }

    Components.Icon {
        id: icon
        color: colors.offwhite
        icon: entityObj.mediaImage && entityObj.mediaImage !== "" ? "" : entityObj.icon
        suffix: entityObj.stateAsString
        anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter; }
        size: 100
        visible: entityObj.enabled
        opacity: entityBaseContainer.iconOn ? 1 : 0.4

        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }

        MediaPlayerComponents.ImageLoader {
            id: imageClosed
            width: 100; height: 100
            anchors.centerIn: icon
            opacity: entityObj.mediaImage !== "" ? 1 : 0
            enabled: visible
            visible: entityObj.type === EntityTypes.Media_player && entityObj.mediaImage != ""
            url: entityObj.mediaImage ? entityObj.mediaImage : ""
            aspectFit: true

            Behavior on opacity {
                NumberAnimation { duration: 300 }
            }
        }

        Components.HapticMouseArea {
            anchors.fill: parent
            onClicked: {
                entityBaseContainer.controlTrigger();
            }
        }
    }

    Components.Icon {
        color: colors.offwhite
        icon: "uc:ban"
        anchors.centerIn: icon
        size: 100
        visible: !editMode && !entityObj.enabled
    }

    ColumnLayout {
        id: titleContainer

        spacing: 0
        anchors { left: icon.right; leftMargin: 20; right: parent.right; rightMargin: editMode ? 100 : 20; verticalCenter: parent.verticalCenter; }

        Text {
            id: titleText

            Layout.fillWidth: true
            text: entityObj.name
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: 2
            elide: Text.ElideRight
            color: colors.offwhite
            font: fonts.primaryFont(30)
            lineHeight: 0.8
        }

        Text {
            id: statusText

            Layout.fillWidth: true
            text: entityObj.stateInfo
            maximumLineCount: 1
            elide: Text.ElideRight
            color: colors.light
            verticalAlignment: Text.AlignVCenter
            font: fonts.secondaryFont(24)
            visible: entityObj.stateInfo !== ""
        }
    }

    Components.PopupMenu {
        id: popupMenu
        parent: root
    }

    Component.onCompleted: {
        let e = EntityController.get(entityBaseContainer.entityId);

        if (e) {
            entityBaseContainer.entityObj = e;
            entityBaseContainer.build();
        }
    }
}
