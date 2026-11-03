// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 NUMBER COMPONENT

 A user interface page widget showing a number entity with its effective display mode: a slider (the slider widget
 bound to the value feature, sent on release) or the value with decrement and increment buttons (stepper). The
 widget's own display mode slider or stepper wins over the entity's, auto follows the entity. An unbounded number is
 always a stepper. With its own step the stepper sends increment and decrement with that step; a slider ignores it. A number without a value, an unavailable one and a missing entity read "—"
 without a unit; an unavailable entity and an entity that is not a number get the red mark.
 See https://github.com/unfoldedcircle/core-api/blob/main/doc/ui-widgets.md#number

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width / height: a slider or stepper is vertical if height > width
 - entityId
 - customText (label), showName
 - displayMode: auto (default), slider or stepper
 - step: the step of the stepper buttons, 0 for the entity's step
**/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Entity.Controller 1.0

import "qrc:/components" as Components

Item {
    id: numberWidget
    width: 160
    height: 80
    clip: true

    property string customText
    property string entityId
    property bool showName: false
    property string displayMode: "auto"
    property real step: 0

    property QtObject entityObj

    readonly property bool vertical: height > width
    readonly property bool isNumber: entityObj !== null && entityObj.type === EntityTypes.Number
    // resolved by the entity from the widget's display mode and the entity options
    property bool slider: false
    readonly property bool unavailable: entityObj !== null && !entityObj.enabled
    // the red mark: an unavailable entity or one that is not a number
    readonly property bool marked: unavailable || (entityObj !== null && !isNumber)

    function ensureEntityLoaded() {
        entityObj = EntityController.get(entityId);
    }

    function resolveRepresentation() {
        numberWidget.slider = entityObj !== null && entityObj.type === EntityTypes.Number
                && entityObj.isSliderRepresentationFor(numberWidget.displayMode);
    }

    // like a tile, an unavailable entity refuses the press with a notification
    function step(up) {
        if (!numberWidget.isNumber) {
            return;
        }

        if (!EntityController.mayCommandEntity(entityObj.enabled)) {
            //: Notification when a number was changed but the command was not sent because the device is unavailable. %1 is the entity name
            ui.createNotification(qsTr("%1 is unavailable").arg(entityObj.name), true);
            return;
        }

        if (up) {
            entityObj.increment(numberWidget.step);
        } else {
            entityObj.decrement(numberWidget.step);
        }
    }

    Loader {
        anchors.fill: parent
        active: numberWidget.slider
        sourceComponent: Components.SliderWidget {
            entityId: numberWidget.entityId
            feature: "value"
            customText: numberWidget.customText
            showName: numberWidget.showName
        }
    }

    ColumnLayout {
        anchors { fill: parent; leftMargin: 10; rightMargin: 10; topMargin: 4; bottomMargin: 4 }
        spacing: 0
        visible: !numberWidget.slider

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: labelText.text !== "" || numberWidget.marked

            Components.Icon {
                color: colors.red
                icon: "uc:link-slash"
                size: ui.unavailableMarkSize
                visible: numberWidget.marked
            }

            Text {
                id: labelText
                Layout.fillWidth: true
                text: numberWidget.showName && entityObj ? entityObj.name : numberWidget.customText
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.textSecondary
                opacity: numberWidget.unavailable ? ui.unavailableLabelOpacity : 1
                verticalAlignment: Text.AlignVCenter
                font: fonts.help()
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: numberWidget.vertical ? 1 : 3
            rowSpacing: 4
            columnSpacing: 4

            // vertical: increment on top, horizontal: decrement on the left
            Repeater {
                model: numberWidget.vertical ? ["up", "value", "down"] : ["down", "value", "up"]

                Item {
                    Layout.fillWidth: modelData === "value"
                    Layout.fillHeight: true
                    Layout.minimumWidth: 60
                    Layout.minimumHeight: 60
                    Layout.preferredWidth: modelData === "value" ? -1 : 60

                    Text {
                        anchors.fill: parent
                        visible: modelData === "value"
                        text: {
                            if (!numberWidget.isNumber || numberWidget.unavailable || !entityObj.hasValue) {
                                return "\u2014";
                            }
                            return entityObj.formattedValue + (entityObj.unit !== "" ? " " + entityObj.unit : "");
                        }
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        color: colors.textPrimary
                        opacity: numberWidget.unavailable ? ui.unavailableOpacity : 1
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: fonts.label()
                    }

                    Rectangle {
                        anchors.fill: parent
                        visible: modelData !== "value"
                        radius: ui.cornerRadiusSmall
                        color: stepMouseArea.pressed ? colors.textPrimary : colors.surface
                        opacity: numberWidget.unavailable ? ui.unavailableOpacity : 1

                        Components.Icon {
                            anchors.centerIn: parent
                            color: stepMouseArea.pressed ? colors.bg : colors.textPrimary
                            icon: modelData === "up" ? "uc:plus" : "uc:minus"
                            size: 60
                        }

                        Components.HapticMouseArea {
                            id: stepMouseArea
                            anchors.fill: parent
                            enabled: numberWidget.isNumber
                            onClicked: numberWidget.step(modelData === "up")
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: ensureEntityLoaded()
    onEntityIdChanged: ensureEntityLoaded()
    onEntityObjChanged: resolveRepresentation()
    onDisplayModeChanged: resolveRepresentation()

    Connections {
        target: EntityController
        ignoreUnknownSignals: true

        function onEntityLoaded(success, loadedId) {
            if (!success || loadedId !== numberWidget.entityId) {
                return;
            }

            numberWidget.entityObj = EntityController.get(loadedId);
        }
    }

    Connections {
        target: numberWidget.entityObj
        ignoreUnknownSignals: true

        function onOptionsChanged() {
            numberWidget.resolveRepresentation();
        }
    }
}
