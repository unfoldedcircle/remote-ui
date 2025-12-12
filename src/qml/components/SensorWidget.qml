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

ColumnLayout {
    id: sensorWidget
    width: 80
    height: 80
    spacing: 4

    property string customText
    property string entityId
    property QtObject entityObj
    property bool showLabel: true
    property bool showUnit: true

    Item { Layout.fillHeight: true }

    Text {
        id: sensorLabelText
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter

        text: {
            if (sensorWidget.customText != "") {
                return sensorWidget.customText;
            }

            if (sensorWidget.showLabel) {
                if (entityObj.customLabel !== "") {
                    return entityObj.customLabel;
                } else {
                    return entityObj.getDeviceClass();
                }
            }

            return "";
        }
        maximumLineCount: 1
        elide: Text.ElideRight
        color: colors.light
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        font: fonts.primaryFont(24)
    }

    Text {
        id: sensorValueText
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter

        text: {
            if (!entityObj) {
                return qsTranslate("Abbreviation for not available", "N/A");
            }

            if (entityObj.customUnit !== "") {
                return entityObj.value + " "  + (sensorWidget.showUnit ? entityObj.customUnit : "");
            }

            return entityObj.value + " "  + (sensorWidget.showUnit ? entityObj.unit : "");
        }
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        maximumLineCount: 2
        elide: Text.ElideRight
        color: colors.offwhite
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        font: fonts.primaryFont(36)
    }

    Item { Layout.fillHeight: true }

    Component.onCompleted: {
        entityObj = EntityController.get(entityId);

        if (!entityObj) {
            EntityController.load(entityId);
            connectSignalSlot(EntityController.entityLoaded, function(success, entityId) {
                entityObj = EntityController.get(entityId);
            });
        }
    }

    onEntityIdChanged: {
        entityObj = EntityController.get(entityId);

        if (!entityObj) {
            EntityController.load(entityId);
            connectSignalSlot(EntityController.entityLoaded, function(success, entityId) {
                entityObj = EntityController.get(entityId);
            });
        }
    }
}
