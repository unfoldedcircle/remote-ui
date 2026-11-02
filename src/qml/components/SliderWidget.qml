// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SLIDER COMPONENT

 A user interface page widget bound to a numeric entity feature ("entity feature target").
 See https://github.com/unfoldedcircle/core-api/blob/main/doc/ui-widgets.md#slider

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - width / height: the orientation is derived from the size (height > width: vertical)
 - entityId, feature (optional, defaults to the entity type's default slider feature)
 - customText (label), showName, showValue
 - iconMin, iconMax
 - minOverride, maxOverride (clamped to the feature range)
 - updateMode: "on_release" | "live"

 The media player's seek feature is a touch slider target only, and an unbounded number entity has no slider range:
 both are shown as unsupported.

 An unavailable entity accepts no command: like a tile (entities/Base.qml), the slider refuses the
 drag where the user pressed, with a notification, instead of sending a command the core would drop.
 EntityController.mayCommandEntity() carries the one exception, the remote coming back from a wakeup,
 see src/ui/entity/entityCommandPolicy.h.
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Entity.Controller 1.0

import "qrc:/components" as Components

Item {
    id: sliderWidget
    width: 160
    height: 80
    clip: true

    property string customText
    property string entityId
    property string feature: ""
    property bool showName: false
    property bool showValue: true
    property string iconMin: ""
    property string iconMax: ""
    property var minOverride: undefined
    property var maxOverride: undefined
    property string updateMode: "on_release"

    property QtObject entityObj

    readonly property bool vertical: height > width
    readonly property bool live: updateMode === "live"

    // resolved entity feature binding
    property string resolvedFeature: ""
    property bool supported: false
    property real rangeMin: 0
    property real rangeMax: 100
    property real stepSize: 1

    // value currently shown: the entity value, or the dragged value while the user interacts
    property real displayValue: 0
    property real lastSentValue: NaN

    // the drag in progress was refused: nothing is sent for it and the handle snaps back on release
    property bool refused: false

    // Decided once, when the drag starts: a refusal per live update would raise a notification every
    // 200 ms while the finger is down.
    function commandAllowed() {
        if (!entityObj || !sliderWidget.supported) {
            return false;
        }

        if (EntityController.mayCommandEntity(entityObj.enabled)) {
            return true;
        }

        //: Notification when a slider was moved but the command was not sent because the device is unavailable. %1 is the entity name
        ui.createNotification(qsTr("%1 is unavailable").arg(entityObj.name), true);
        return false;
    }

    function ensureEntityLoaded() {
        entityObj = EntityController.get(entityId);
        resolveBinding();
    }

    function defaultFeatureForType(type) {
        switch (type) {
        case EntityTypes.Number:
            return "value";
        case EntityTypes.Light:
            return "dim";
        case EntityTypes.Media_player:
            return "volume";
        case EntityTypes.Cover:
            return "position";
        case EntityTypes.Climate:
            return "target_temperature";
        default:
            return "";
        }
    }

    function resolveBinding() {
        sliderWidget.supported = false;

        if (!entityObj) {
            labelText.text = sliderWidget.customText;
            valueText.text = qsTranslate("Abbreviation for not available", "N/A");
            return;
        }

        sliderWidget.resolvedFeature = (sliderWidget.feature !== "" && sliderWidget.feature !== "default")
                ? sliderWidget.feature : defaultFeatureForType(entityObj.type);

        let min = 0;
        let max = 100;
        let step = 1;

        switch (entityObj.type) {
        case EntityTypes.Number:
            if (sliderWidget.resolvedFeature === "value" && entityObj.bounded) {
                min = entityObj.min; max = entityObj.max; step = entityObj.step;
                sliderWidget.supported = true;
            }
            break;
        case EntityTypes.Light:
            if (sliderWidget.resolvedFeature === "dim") {
                min = 0; max = 255; step = 1;
                sliderWidget.supported = true;
            } else if (sliderWidget.resolvedFeature === "color_temperature") {
                // slider positions: the color_temperature_steps option spread over 0..100, see Light
                min = 0; max = entityObj.colorTempPositionMax; step = 1;
                sliderWidget.supported = true;
            }
            break;
        case EntityTypes.Media_player:
            if (sliderWidget.resolvedFeature === "volume") {
                min = 0; max = 100; step = 1;
                sliderWidget.supported = true;
            }
            break;
        case EntityTypes.Cover:
            if (sliderWidget.resolvedFeature === "position") {
                min = 0; max = 100; step = 1;
                sliderWidget.supported = true;
            }
            break;
        case EntityTypes.Climate:
            if (sliderWidget.resolvedFeature === "target_temperature") {
                min = entityObj.minTemperature; max = entityObj.maxTemperature;
                step = entityObj.targetTemperatureStep > 0 ? entityObj.targetTemperatureStep : 0.5;
                sliderWidget.supported = true;
            }
            break;
        }

        // optional user limits in the unit of the feature, clamped to the feature range
        const colorTemp = entityObj.type === EntityTypes.Light && sliderWidget.resolvedFeature === "color_temperature";
        const toRange = function(limit) {
            return colorTemp ? entityObj.colorTempToPosition(Number(limit)) : Number(limit);
        }
        if (sliderWidget.minOverride !== undefined && sliderWidget.minOverride !== null) {
            min = Math.max(min, Math.min(max, toRange(sliderWidget.minOverride)));
        }
        if (sliderWidget.maxOverride !== undefined && sliderWidget.maxOverride !== null) {
            max = Math.min(max, Math.max(min, toRange(sliderWidget.maxOverride)));
        }

        sliderWidget.rangeMin = min;
        sliderWidget.rangeMax = max;
        sliderWidget.stepSize = step;

        labelText.text = sliderWidget.showName ? entityObj.name : sliderWidget.customText;

        if (!sliderWidget.supported) {
            console.log("Slider widget: unsupported feature '" + sliderWidget.resolvedFeature
                        + "' for entity " + sliderWidget.entityId);
            valueText.text = qsTranslate("Abbreviation for not available", "N/A");
            return;
        }

        syncFromEntity();
    }

    function entityValue() {
        switch (entityObj.type) {
        case EntityTypes.Number:
            return entityObj.value;
        case EntityTypes.Light:
            return sliderWidget.resolvedFeature === "dim" ? entityObj.brightness
                                                          : entityObj.colorTempToPosition(entityObj.colorTemp);
        case EntityTypes.Media_player:
            return entityObj.volume;
        case EntityTypes.Cover:
            return entityObj.position;
        case EntityTypes.Climate:
            return entityObj.targetTemperature;
        }
        return 0;
    }

    function sendValue(value) {
        if (!entityObj || !sliderWidget.supported || sliderWidget.refused) {
            return;
        }

        if (!isNaN(sliderWidget.lastSentValue) && sliderWidget.lastSentValue === value) {
            return;
        }
        sliderWidget.lastSentValue = value;

        switch (entityObj.type) {
        case EntityTypes.Number:
            entityObj.setValue(value);
            break;
        case EntityTypes.Light:
            if (sliderWidget.resolvedFeature === "dim") {
                entityObj.setBrightness(value);
            } else {
                entityObj.setColorTemperature(entityObj.colorTempFromPosition(value));
            }
            break;
        case EntityTypes.Media_player:
            entityObj.setVolume(value);
            break;
        case EntityTypes.Cover:
            entityObj.setPosition(value);
            break;
        case EntityTypes.Climate:
            entityObj.setTargetTemperature(value);
            break;
        }

        // optimistic display: fall back to the entity value if the driver doesn't confirm
        fallbackTimer.restart();
    }

    function formatValue(value) {
        switch (entityObj.type) {
        case EntityTypes.Number:
            return entityObj.formatValue(value) + (entityObj.unit !== "" ? " " + entityObj.unit : "");
        case EntityTypes.Light:
            return (sliderWidget.resolvedFeature === "dim" ? Math.round(value / 255 * 100)
                                                           : entityObj.colorTempFromPosition(value)) + " %";
        case EntityTypes.Media_player:
            return Math.round(value).toString();
        case EntityTypes.Cover:
            return Math.round(value) + " %";
        case EntityTypes.Climate:
            return value.toFixed(sliderWidget.stepSize < 1 ? 1 : 0) + " " + entityObj.temperatureLabel;
        }
        return value.toString();
    }

    function syncFromEntity() {
        if (!entityObj || !sliderWidget.supported || valueSlider.pressed) {
            return;
        }

        fallbackTimer.stop();
        sliderWidget.lastSentValue = NaN;
        sliderWidget.displayValue = entityValue();
        valueSlider.value = sliderWidget.displayValue;

        if (entityObj.type === EntityTypes.Number && !entityObj.hasValue) {
            // unavailable or unknown: no value
            valueText.text = qsTranslate("Abbreviation for not available", "N/A");
        } else {
            valueText.text = formatValue(sliderWidget.displayValue);
        }
    }

    function onDragged(value) {
        if (sliderWidget.refused) {
            return;
        }

        sliderWidget.displayValue = value;
        valueText.text = formatValue(value);
        Haptic.play(Haptic.Bump);

        if (sliderWidget.live && !liveTimer.running) {
            liveTimer.start();
        }
    }

    Timer {
        // live mode: send while dragging, at most every 200 ms
        id: liveTimer
        interval: 200
        repeat: true
        running: false
        onTriggered: {
            if (!valueSlider.pressed) {
                liveTimer.stop();
                return;
            }
            sliderWidget.sendValue(valueSlider.value);
        }
    }

    Timer {
        // no confirmation from the driver: show the last known entity value again
        id: fallbackTimer
        interval: 3000
        repeat: false
        running: false
        onTriggered: sliderWidget.syncFromEntity()
    }

    GridLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 4
        anchors.bottomMargin: 4
        columns: sliderWidget.vertical ? 1 : 3
        rowSpacing: 0
        columnSpacing: 6

        // header: label and value
        RowLayout {
            Layout.fillWidth: true
            Layout.columnSpan: sliderWidget.vertical ? 1 : 3
            spacing: 4
            visible: labelText.text !== "" || (sliderWidget.showValue && !sliderWidget.vertical) || (entityObj && !entityObj.enabled)

            Components.Icon {
                color: colors.red
                icon: "uc:link-slash"
                size: 30
                visible: entityObj && !entityObj.enabled
            }

            Text {
                id: labelText
                Layout.fillWidth: true
                text: ""
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.textSecondary
                verticalAlignment: Text.AlignVCenter
                font: fonts.help()
            }

            Text {
                id: valueText
                Layout.alignment: Qt.AlignRight
                text: ""
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.textPrimary
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                font: fonts.label()
                visible: sliderWidget.showValue && !sliderWidget.vertical
            }
        }

        Components.Icon {
            color: colors.textSecondary
            icon: sliderWidget.iconMin
            size: 40
            visible: !sliderWidget.vertical && sliderWidget.iconMin !== ""
        }

        Slider {
            id: valueSlider
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: sliderWidget.vertical ? 60 : 30
            live: true
            // stays pressable while the entity is unavailable, so the press is refused with a
            // notification like on a tile - and gets through while the remote is waking up
            enabled: sliderWidget.supported && entityObj !== null
            opacity: entityObj && !entityObj.enabled ? 0.4 : 1
            padding: 0
            snapMode: Slider.SnapAlways
            orientation: sliderWidget.vertical ? Qt.Vertical : Qt.Horizontal
            from: sliderWidget.rangeMin
            to: sliderWidget.rangeMax
            stepSize: sliderWidget.stepSize

            background: Rectangle {
                implicitWidth: valueSlider.width; implicitHeight: valueSlider.height
                width: valueSlider.availableWidth; height: valueSlider.availableHeight
                radius: ui.cornerRadiusSmall
                color: colors.surfaceRaised

                Rectangle {
                    // filled part of the track
                    width: sliderWidget.vertical ? parent.width : valueSlider.visualPosition * parent.width
                    height: sliderWidget.vertical ? (1 - valueSlider.visualPosition) * parent.height : parent.height
                    anchors.bottom: parent.bottom
                    color: colors.textPrimary
                    radius: ui.cornerRadiusSmall
                }
            }

            handle: Item {
                implicitWidth: 40; implicitHeight: 40
                x: sliderWidget.vertical ? (valueSlider.availableWidth - width) / 2
                                         : valueSlider.visualPosition * (valueSlider.availableWidth - width)
                y: sliderWidget.vertical ? valueSlider.visualPosition * (valueSlider.availableHeight - height)
                                         : (valueSlider.availableHeight - height) / 2
            }

            onMoved: sliderWidget.onDragged(value)

            onPressedChanged: {
                if (valueSlider.pressed) {
                    sliderWidget.refused = !sliderWidget.commandAllowed();
                    return;
                }

                liveTimer.stop();

                if (sliderWidget.refused) {
                    // nothing was sent: put the handle back on the entity value
                    sliderWidget.refused = false;
                    sliderWidget.syncFromEntity();
                    return;
                }

                sliderWidget.sendValue(valueSlider.value);
            }
        }

        Components.Icon {
            color: colors.textSecondary
            icon: sliderWidget.iconMax
            size: 40
            visible: !sliderWidget.vertical && sliderWidget.iconMax !== ""
        }

        Text {
            // vertical layout: value below the slider
            Layout.fillWidth: true
            text: valueText.text
            maximumLineCount: 1
            elide: Text.ElideRight
            color: colors.textPrimary
            horizontalAlignment: Text.AlignHCenter
            font: fonts.help()
            visible: sliderWidget.showValue && sliderWidget.vertical
        }
    }

    Component.onCompleted: ensureEntityLoaded()
    onEntityIdChanged: ensureEntityLoaded()
    onFeatureChanged: resolveBinding()

    Connections {
        target: EntityController
        ignoreUnknownSignals: true

        function onEntityLoaded(success, loadedId) {
            if (!success || loadedId !== sliderWidget.entityId) {
                return;
            }

            sliderWidget.entityObj = EntityController.get(loadedId);
            sliderWidget.resolveBinding();
        }
    }

    Connections {
        target: sliderWidget.entityObj
        ignoreUnknownSignals: true

        // number
        function onValueChanged() { sliderWidget.syncFromEntity(); }
        function onOptionsChanged() { sliderWidget.resolveBinding(); }
        // light
        function onBrightnessChanged() { sliderWidget.syncFromEntity(); }
        function onColorTempChanged() { sliderWidget.syncFromEntity(); }
        // media player
        function onVolumeChanged() { sliderWidget.syncFromEntity(); }
        // cover
        function onPositionChanged() { sliderWidget.syncFromEntity(); }
        // climate
        function onTargetTemperatureChanged() { sliderWidget.syncFromEntity(); }
        function onModelChanged() { sliderWidget.resolveBinding(); }

        function onEntityEnabledChanged() { sliderWidget.syncFromEntity(); }
    }
}
