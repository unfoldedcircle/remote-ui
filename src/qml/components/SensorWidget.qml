// Copyright (c) 2022-2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SENSOR COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width
 - height
 - text
 - textColor
 - show label
 - show unit
**/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Entity.Controller 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: sensorWidget
    width: 80
    height: 80
    spacing: 4
    clip: true

    property string customText
    property string entityId
    property QtObject entityObj
    property bool showLabel: true
    property bool showUnit: true

    // the unavailable look of every page widget: the red mark, a dimmed label and a dimmed value (docs/design-system.md)
    readonly property bool unavailable: entityObj !== null && !entityObj.enabled

    function ensureEntityLoaded() {
        entityObj = EntityController.get(entityId);
        evaluateSensor();
    }

    function evaluateSensor() {
        // sensor label: the custom label may be removed by an option update
        if (sensorWidget.showLabel && entityObj && entityObj.customLabel && entityObj.customLabel !== "") {
            sensorLabelText.text =  entityObj.customLabel;
        } else {
            sensorLabelText.text = sensorWidget.customText;
        }

        // sensor value: "—" without a unit for a missing or unavailable entity and a sensor without a value
        if (!entityObj || !entityObj.enabled || entityObj.value === "") {
            sensorValueText.text = "\u2014";
            return;
        }

        if (entityObj.customUnit !== "") {
            sensorValueText.text = entityObj.value + " "  + (sensorWidget.showUnit ? entityObj.customUnit : "");
        } else {
            sensorValueText.text = entityObj.value + " "  + (sensorWidget.showUnit ? entityObj.unit : "");
        }
    }

    Item { Layout.fillHeight: true }

    RowLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        spacing: 0

        Item { Layout.fillWidth: true }

        Components.Icon {
            color: colors.red
            icon: "uc:link-slash"
            size: ui.unavailableMarkSize
            visible: sensorWidget.unavailable
        }

        Item { Layout.preferredWidth: sensorWidget.unavailable ? 2 : 0 }

        Text {
            id: sensorLabelText
            Layout.maximumWidth: sensorWidget.width
            text: ""
            maximumLineCount: 1
            elide: Text.ElideRight
            color: colors.textSecondary
            opacity: sensorWidget.unavailable ? ui.unavailableLabelOpacity : 1
            verticalAlignment: Text.AlignVCenter
            font: fonts.help()
        }

        Item { Layout.fillWidth: true }
    }

    Text {
        id: sensorValueText
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter

        text: ""
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        maximumLineCount: 2
        elide: Text.ElideRight
        color: colors.offwhite
        opacity: sensorWidget.unavailable ? ui.unavailableOpacity : 1
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        font: fonts.primaryFont(36)
    }

    Item { Layout.fillHeight: true }

    Component.onCompleted: ensureEntityLoaded()
    onEntityIdChanged: ensureEntityLoaded()

    Connections {
        target: EntityController
        ignoreUnknownSignals: true

        function onEntityLoaded(success, loadedId) {
            if (!success || loadedId !== sensorWidget.entityId) {
                return;
            }

            sensorWidget.entityObj = EntityController.get(loadedId);
            sensorWidget.evaluateSensor();
        }
    }

    Connections {
        target: sensorWidget.entityObj
        ignoreUnknownSignals: true

        function onValueChanged() {
            sensorWidget.evaluateSensor();
        }

        function onUnitChanged() {
            sensorWidget.evaluateSensor();
        }

        function onEntityEnabledChanged() {
            sensorWidget.evaluateSensor();
        }
    }

}
