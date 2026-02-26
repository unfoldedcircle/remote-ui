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
import Entity.Select 1.0

import Integration.Controller 1.0

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

    property QtObject integrationObjDummy: QtObject {
        property string state
    }

    property QtObject integrationObj: integrationObjDummy


    property bool isHighLightEnabled: true
    property bool isSelected: isHighLightEnabled ? parent.isCurrentItem : false
    property bool isInGroup: false
    property string parentGroupId
    property bool editMode
    property bool iconOn: false
    property var controlTrigger: function() {}

    property alias button: button

    property bool hasMediaImage: entityObj && entityObj.type === EntityTypes.Media_player && entityObj.mediaImage != ""

    function handleActivityOpen() {
        if (entityBaseContainer.entityObj.type !== EntityTypes.Activity) {
            return true;
        }

        function showActivityPopup() {
            popupMenu.title = qsTr("Activity error. Select option below.");
            let menuItems = [];
            menuItems.push({
                               title: qsTr("Turn activity on"),
                               icon: "uc:arrow-right",
                               callback: function() {
                                   const res = checkActivityIncludedEntities(entityBaseContainer.entityObj);

                                   if (!res.allIncludedEntitiesConnected && entityBaseContainer.entityObj.readyCheck) {
                                       ui.createActionableNotification(qsTr("Some devices are not ready"), (res.notReadyEntityQty == 1 ? qsTr("%1 is not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities) : qsTr("%1 are not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities)), "uc:link-slash", () => { entityObj.turnOn(); }, qsTr("Proceed"));
                                   } else {
                                       entityObj.turnOn();
                                   }
                               }
                           });
            menuItems.push({
                               title: qsTr("Turn activity off"),
                               icon: "uc:arrow-left",
                               callback: function() {
                                   const res = checkActivityIncludedEntities(entityBaseContainer.entityObj, false);

                                   if (!res.allIncludedEntitiesConnected && entityBaseContainer.entityObj.readyCheck) {
                                       ui.createActionableNotification(qsTr("Some devices are not ready"), (res.notReadyEntityQty == 1 ? qsTr("%1 is not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities) : qsTr("%1 are not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities)), "uc:link-slash", () => { entityObj.turnOff(); }, qsTr("Proceed"));
                                   } else {
                                       entityObj.turnOff();
                                   }
                               }
                           });
            menuItems.push({
                               title: qsTr("Open activity"),
                               icon: "uc:arrow-up-right-and-arrow-down-left-from-center",
                               callback: function() {
                                   loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityId, "entityObj": entityObj, "integrationObj": integrationObj });
                               }
                           });
            popupMenu.menuItems = menuItems;
            popupMenu.open();
        }


        switch (entityObj.state) {
        case ActivityStates.Running:
            return true;
        case ActivityStates.Off: {
            const res = checkActivityIncludedEntities(entityBaseContainer.entityObj);

            if (EntityController.resumeWindow && !res.allIncludedEntitiesConnected) {
                ui.setTimeOut(500, () => {
                                  entityBaseContainer.handleActivityOpen();
                              });
                return false;
            } else if (!EntityController.resumeWindow && !res.allIncludedEntitiesConnected && entityBaseContainer.entityObj.readyCheck) {
                ui.createActionableNotification(qsTr("Some devices are not ready"), (res.notReadyEntityQty == 1 ? qsTr("%1 is not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities) : qsTr("%1 are not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities)), "uc:link-slash", () => {
                                                    switch (entityObj.state) {
                                                        case ActivityStates.Off:
                                                        entityObj.turnOn();
                                                        break;
                                                        case ActivityStates.Error:
                                                        case ActivityStates.Timeout:
                                                        showActivityPopup();
                                                        break;
                                                        case ActivityStates.On:
                                                        if (!entityObj.enabled) {
                                                            ui.createNotification(entityObj.name + " " + qsTr("is unavailable"), true);
                                                        } else {
                                                            loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityId, "entityObj": entityObj, "integrationObj": integrationObj });
                                                        }
                                                        break;
                                                    }
                                                }, qsTr("Proceed"));
                return false;
            } else {
                entityObj.turnOn();
                return false;
            }
        }
        case ActivityStates.Error:
        case ActivityStates.Timeout:
            showActivityPopup();
            return false;
        case ActivityStates.On:
            if (!entityObj.enabled) {
                ui.createNotification(entityObj.name + " " + qsTr("is unavailable"), true);
                return false;
            } else {
                return true;
            }
        default:
            return true;
        }
    }

    function open() {
        if (entityBaseContainer.handleActivityOpen()) {
            loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityId, "entityObj": entityObj, "integrationObj": integrationObj });
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
        case EntityTypes.Select:
            entityBaseContainer.iconOn = true;
            entityBaseContainer.controlTrigger = function() { entityObj.selectNext(); }
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
        icon: hasMediaImage ? "" : entityObj.icon
        suffix: entityObj.stateAsString
        anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter; }
        size: 100
        visible: entityObj.enabled
        opacity: entityBaseContainer.iconOn ? 1 : 0.4

        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }

        Components.HapticMouseArea {
            anchors.fill: parent
            onClicked: {
                entityBaseContainer.controlTrigger();
            }
        }
    }

    MediaPlayerComponents.ImageLoader {
        id: imageClosed
        width: 100
        anchors.centerIn: icon
        opacity: visible ? 1 : 0
        enabled: visible
        visible: hasMediaImage && entityObj.enabled
        url: entityObj.mediaImage
        aspectFit: true
        alignCentered: true

        Behavior on opacity {
            NumberAnimation { duration: 300 }
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

        RowLayout {
            spacing: 4

            Components.Icon {
                color: colors.red
                icon: "uc:link-slash"
                size: 40
                visible: integrationObj.state != "connected" && integrationObj.state != ""
            }

            Text {
                id: statusText

                Layout.fillWidth: true
                text: entityObj.stateInfo;
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.light
                verticalAlignment: Text.AlignVCenter
                font: fonts.secondaryFont(24)
                visible: entityObj.stateInfo !== ""
            }
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
            entityBaseContainer.integrationObj = IntegrationController.getModelItem(entityObj.integrationId);
            if (!entityBaseContainer.integrationObj) {
                entityBaseContainer.integrationObj = entityBaseContainer.integrationObjDummy;
            }

            entityBaseContainer.build();
        }
    }

    Connections {
        target: IntegrationController
        ignoreUnknownSignals: true

        function onIntegrationsLoaded() {
            entityBaseContainer.integrationObj = IntegrationController.getModelItem(entityObj.integrationId);
            if (!entityBaseContainer.integrationObj) {
                entityBaseContainer.integrationObj = entityBaseContainer.integrationObjDummy;
            }
        }
    }
}
