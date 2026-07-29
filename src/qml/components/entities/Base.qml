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
    opacity: currentEntityObj.enabled ? 1 : 0.5
    radius: ui.cornerRadiusSmall
    border {
        color: (isInGroup || isSelected) && !editMode ? colors.medium : colors.transparent
        width: 1
    }

    property string entityId
    property QtObject entityObjDummy: QtObject {
        property string name
        property string icon
        property string stateAsString
        property string stateInfo
        property string integrationId
        property bool enabled
        property int state
        property int type
        property string mediaImage
    }
    property QtObject entityObj: entityObjDummy
    property QtObject currentEntityObj: entityObj ? entityObj : entityObjDummy

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

    property bool hasMediaImage: currentEntityObj.type === EntityTypes.Media_player && currentEntityObj.mediaImage != ""

    function ensureEntityLoaded() {
        const e = EntityController.get(entityBaseContainer.entityId);

        if (!e) {
            entityBaseContainer.entityObj = entityBaseContainer.entityObjDummy;
            entityBaseContainer.integrationObj = entityBaseContainer.integrationObjDummy;
            entityBaseContainer.iconOn = false;
            entityBaseContainer.controlTrigger = function() {}
            button.checked = false;
            return false;
        }

        entityBaseContainer.entityObj = e;
        entityBaseContainer.integrationObj = IntegrationController.getModelItem(currentEntityObj.integrationId);
        if (!entityBaseContainer.integrationObj) {
            entityBaseContainer.integrationObj = entityBaseContainer.integrationObjDummy;
        }

        entityBaseContainer.build();
        return true;
    }

    function handleActivityOpen() {
        if (!entityBaseContainer.entityObj) {
            return false;
        }

        if (currentEntityObj.type !== EntityTypes.Activity) {
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
                                       ui.createActionableNotification(qsTr("Some devices are not ready"), (res.notReadyEntityQty == 1 ? qsTr("%1 is not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities) : qsTr("%1 are not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities)), "uc:link-slash", () => {
                                                                         if (!entityObj) {
                                                                             return;
                                                                         }

                                                                         entityObj.turnOn();
                                                                     }, qsTr("Proceed"));
                                   } else {
                                       if (!entityObj) {
                                           return;
                                       }

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
                                       ui.createActionableNotification(qsTr("Some devices are not ready"), (res.notReadyEntityQty == 1 ? qsTr("%1 is not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities) : qsTr("%1 are not connected yet. Tap Proceed to continue anyway.").arg(res.notReadyEntities)), "uc:link-slash", () => {
                                                                         if (!entityObj) {
                                                                             return;
                                                                         }

                                                                         entityObj.turnOff();
                                                                     }, qsTr("Proceed"));
                                   } else {
                                       if (!entityObj) {
                                           return;
                                       }

                                       entityObj.turnOff();
                                   }
                               }
                           });
            menuItems.push({
                               title: qsTr("Open activity"),
                               icon: "uc:arrow-up-right-and-arrow-down-left-from-center",
                               callback: function() {
                                   if (!entityObj) {
                                       return;
                                   }

                                   loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityId, "entityObj": entityObj, "integrationObj": integrationObj });
                               }
                           });
            popupMenu.menuItems = menuItems;
            popupMenu.open();
        }


        switch (currentEntityObj.state) {
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
                                                    if (!entityObj) {
                                                        return;
                                                    }

                                                    switch (currentEntityObj.state) {
                                                        case ActivityStates.Off:
                                                        entityObj.turnOn();
                                                        break;
                                                        case ActivityStates.Error:
                                                        case ActivityStates.Timeout:
                                                        showActivityPopup();
                                                        break;
                                                        case ActivityStates.On:
                                                        if (!currentEntityObj.enabled) {
                                                            ui.createNotification(currentEntityObj.name + " " + qsTr("is unavailable"), true);
                                                        } else {
                                                            loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityId, "entityObj": entityObj, "integrationObj": integrationObj });
                                                        }
                                                        break;
                                                    }
                                                }, qsTr("Proceed"));
                return false;
            } else {
                if (!entityObj) {
                    return false;
                }

                entityObj.turnOn();
                return false;
            }
        }
        case ActivityStates.Error:
        case ActivityStates.Timeout:
            showActivityPopup();
            return false;
        case ActivityStates.On:
            if (!currentEntityObj.enabled) {
                ui.createNotification(currentEntityObj.name + " " + qsTr("is unavailable"), true);
                return false;
            } else {
                return true;
            }
        default:
            return true;
        }
    }

    function open() {
        if (!entityObj) {
            return;
        }

        if (entityBaseContainer.handleActivityOpen()) {
            loadSecondContainer("qrc:/components/entities/" + entityObj.getTypeAsString() + "/deviceclass/" + entityObj.getDeviceClass() + ".qml", { "entityId": entityId, "entityObj": entityObj, "integrationObj": integrationObj });
        }
    }

    function build() {
        switch (currentEntityObj.type) {
        case EntityTypes.Button:
            entityBaseContainer.iconOn = Qt.binding(function() { return currentEntityObj.state === ButtonStates.Available || currentEntityObj.state === ButtonStates.On; });
            entityBaseContainer.controlTrigger = function() {
                if (!entityObj) {
                    return;
                }

                entityObj.push();
            }
            button.checked = false;
            break;

        case EntityTypes.Switch:
            entityBaseContainer.iconOn = Qt.binding(function() { return currentEntityObj.state === SwitchStates.On; });

            if (entityObj && entityObj.hasAnyFeature([SwitchFeatures.On_off, SwitchFeatures.Toggle])) {
                entityBaseContainer.controlTrigger = function() {
                    if (!entityObj) {
                        return;
                    }

                    entityObj.toggle();
                };
            }

            button.checked = Qt.binding(() => { return currentEntityObj.state === SwitchStates.On; });
            break;

        case EntityTypes.Climate:
            entityBaseContainer.iconOn = Qt.binding(function() { return currentEntityObj.state !== ClimateStates.Off; });
            button.checked = Qt.binding(() => { return currentEntityObj.state !== ClimateStates.Off; });
            break;

        case EntityTypes.Cover:
            entityBaseContainer.iconOn = Qt.binding(function() { return currentEntityObj.state === CoverStates.Closed; });


            if (entityObj && entityObj.hasAllFeatures([CoverFeatures.Open, CoverFeatures.Close])) {
                entityBaseContainer.controlTrigger = function() {
                    if (!entityObj) {
                        return;
                    }

                    if (currentEntityObj.state === CoverStates.Open) {
                        entityObj.close();
                    } else if (currentEntityObj.state === CoverStates.Closed) {
                        entityObj.open();
                    }
                }
            }

            button.checked = Qt.binding(() => { return currentEntityObj.state === CoverStates.Open; });
            break;

        case EntityTypes.Light:
            entityBaseContainer.iconOn = Qt.binding(function() { return currentEntityObj.state === LightStates.On; });
            entityBaseContainer.controlTrigger = function() {
                if (!entityObj) {
                    return;
                }

                entityObj.toggle();
            }
            button.checked = Qt.binding(() => { return currentEntityObj.state === LightStates.On; });
            break;

        case EntityTypes.Media_player:
            entityBaseContainer.iconOn = Qt.binding(function() { return currentEntityObj.state !== MediaPlayerStates.Off; });
            entityBaseContainer.controlTrigger = function() {
                if (!entityObj) {
                    return;
                }

                if (currentEntityObj.state === MediaPlayerStates.Off) {
                    entityObj.turnOn();
                } else {
                    entityObj.playPause();
                }
            }
            button.checked = Qt.binding(() => { return currentEntityObj.state !== MediaPlayerStates.Off; });
            break;

        case EntityTypes.Remote:
            entityBaseContainer.iconOn = Qt.binding(function() { return currentEntityObj.state === RemoteStates.On; });
            entityBaseContainer.controlTrigger = function() {
                if (!entityObj) {
                    return;
                }

                entityObj.toggle();
            }
            button.checked = false;
            break;

        case EntityTypes.Activity:
            entityBaseContainer.iconOn = Qt.binding(function() { return currentEntityObj.state === ActivityStates.On; });
            entityBaseContainer.controlTrigger = function() {
                entityBaseContainer.handleActivityOpen();
            }
            button.checked = Qt.binding(() => { return currentEntityObj.state === ActivityStates.On; });
            break;

        case EntityTypes.Macro:
            entityBaseContainer.iconOn = true;
            entityBaseContainer.controlTrigger = function() {
                if (!entityObj) {
                    return;
                }

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
            entityBaseContainer.controlTrigger = function() {
                if (!entityObj) {
                    return;
                }

                entityObj.selectNext();
            }
            button.checked = false;
            break;
        }
    }

    Behavior on opacity {
        NumberAnimation { duration: 300 }
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
            if (currentEntityObj.enabled) {
                if (!editMode) {
                    entityBaseContainer.open();
                }
            }
        }
        onPressAndHold: {
            if (!entityObj) {
                return;
            }

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
        icon: hasMediaImage ? "" : currentEntityObj.icon
        suffix: currentEntityObj.stateAsString
        anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter; }
        size: 100
        visible: currentEntityObj.enabled
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
        height: width
        anchors.centerIn: icon
        opacity: visible ? 1 : 0
        enabled: visible
        visible: hasMediaImage && currentEntityObj.enabled
        url: hasMediaImage ? currentEntityObj.mediaImage : ""
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
        visible: !editMode && !currentEntityObj.enabled
    }

    // dark disc behind the spinner so it stays legible over album art
    Rectangle {
        width: 56; height: 56
        radius: width / 2
        color: colors.black
        opacity: 0.2
        anchors.centerIn: icon
        visible: commandLoadingIndicator.visible
    }

    Image {
        id: commandLoadingIndicator
        width: 44; height: 44
        anchors.centerIn: icon
        source: "qrc:/images/loader_small.png"
        fillMode: Image.PreserveAspectFit
        visible: !editMode && currentEntityObj.commandInProgress

        RotationAnimation on rotation {
            running: commandLoadingIndicator.visible
            loops: Animation.Infinite
            from: 0; to: 360
            duration: 1200
        }
    }

    ColumnLayout {
        id: titleContainer

        spacing: 0
        anchors { left: icon.right; leftMargin: 20; right: parent.right; rightMargin: editMode ? 100 : 20; verticalCenter: parent.verticalCenter; }

        Text {
            id: titleText

            Layout.fillWidth: true
            text: currentEntityObj.name
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
                text: currentEntityObj.stateInfo
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.light
                verticalAlignment: Text.AlignVCenter
                font: fonts.secondaryFont(24)
                visible: currentEntityObj.stateInfo !== ""
            }
        }
    }

    Components.PopupMenu {
        id: popupMenu
        parent: root
    }

    Component.onCompleted: ensureEntityLoaded()
    onEntityIdChanged: ensureEntityLoaded()

    Connections {
        target: IntegrationController
        ignoreUnknownSignals: true

        function onIntegrationsLoaded() {
            entityBaseContainer.integrationObj = IntegrationController.getModelItem(currentEntityObj.integrationId);
            if (!entityBaseContainer.integrationObj) {
                entityBaseContainer.integrationObj = entityBaseContainer.integrationObjDummy;
            }
        }
    }

    Connections {
        target: EntityController
        ignoreUnknownSignals: true
        enabled: !entityBaseContainer.entityObj || entityBaseContainer.entityObj === entityBaseContainer.entityObjDummy

        function onEntityLoaded(success, loadedId) {
            if (!success || loadedId !== entityBaseContainer.entityId) {
                return;
            }

            entityBaseContainer.ensureEntityLoaded();
        }
    }
}
