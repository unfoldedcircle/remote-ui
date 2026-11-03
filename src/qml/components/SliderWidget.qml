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
 - sliderStyle: "default" | "bar" | "buttons"
 - sliderColor: "auto" | "neutral" | "entity" | "gradient"
 - sliderFill: "auto" | "start" | "center" | "cursor"

 SliderStyle resolves the style, colour and fill and holds the per-feature defaults. Every colour and size is a design
 system token of colors or ui (docs/design-system.md).

 - default: a thin track with the label and value above it. A press moves the value to the finger.
 - bar: the whole item is the track, with label, value and icons inside. A drag moves the value by the dragged
   distance; a tap changes nothing.
 - buttons: minus, value, plus. A press sends the value one step further; increment and decrement for a number.

 The media player's seek feature is a touch slider target only, and an unbounded number entity has no slider range:
 both are shown as unsupported.

 An unavailable entity accepts no command: like a tile (entities/Base.qml), the slider refuses the
 drag where the user pressed, with a notification, instead of sending a command the core would drop.
 EntityController.mayCommandEntity() carries the one exception, the remote coming back from a wakeup,
 see src/ui/entity/entityCommandPolicy.h.
**/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Entity.Controller 1.0
import Entity.Light 1.0
import SliderStyle 1.0

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
    property string sliderStyle: ""
    property string sliderColor: ""
    property string sliderFill: ""

    property QtObject entityObj

    readonly property bool vertical: height > width
    readonly property bool live: updateMode === "live"
    readonly property bool bar: SliderStyle.style(sliderStyle) === SliderStyle.Bar
    readonly property bool buttons: SliderStyle.style(sliderStyle) === SliderStyle.Buttons
    readonly property bool unavailable: entityObj !== null && !entityObj.enabled

    // resolved entity feature binding
    property string resolvedFeature: ""
    property bool supported: false
    property real rangeMin: 0
    property real rangeMax: 100
    property real stepSize: 1
    // the change of a minus or plus button
    property real buttonStepSize: 1

    // resolved look
    readonly property bool entityColorKnown: entityObj !== null && entityObj.type === EntityTypes.Light
                                             && entityObj.hasFeature(LightFeatures.Color)
    readonly property int paint: entityObj ? SliderStyle.color(entityObj.type, resolvedFeature, sliderColor, entityColorKnown)
                                           : SliderStyle.Neutral
    readonly property int fillMode: entityObj ? SliderStyle.fill(entityObj.type, resolvedFeature, sliderFill, rangeMin, rangeMax)
                                              : SliderStyle.Start
    readonly property var gradientRoles: entityObj ? SliderStyle.gradient(entityObj.type, resolvedFeature) : []
    readonly property color paintColor: paint === SliderStyle.Entity ? entityObj.color : colors.textPrimary
    readonly property bool discrete: SliderStyle.discrete(rangeMin, rangeMax, stepSize)
    readonly property int positionCount: SliderStyle.positions(rangeMin, rangeMax, stepSize)

    // value currently shown: the entity value, or the dragged value while the user interacts
    property real displayValue: 0
    // value the track shows: follows the finger between the positions of a discrete slider while dragging
    property real trackValue: 0
    property real lastSentValue: NaN
    property string labelString: ""
    property string valueString: ""

    property bool dragging: false
    // the drag in progress was refused: nothing is sent for it and the track returns on release
    property bool refused: false

    readonly property real valueFraction: fraction(trackValue)
    readonly property real fillFrom: fillMode === SliderStyle.Center ? Math.min(fraction(0), valueFraction) : 0
    readonly property real fillTo: fillMode === SliderStyle.Center ? Math.max(fraction(0), valueFraction) : valueFraction

    function fraction(value) {
        const range = sliderWidget.rangeMax - sliderWidget.rangeMin;
        return range > 0 ? Math.max(0, Math.min(1, (value - sliderWidget.rangeMin) / range)) : 0;
    }

    // the colour of the feature gradient at a fraction of the range
    function gradientAt(at) {
        if (sliderWidget.gradientRoles.length !== 2) {
            return colors.textPrimary;
        }
        return SliderStyle.mix(colors[sliderWidget.gradientRoles[0]], colors[sliderWidget.gradientRoles[1]], at);
    }

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
            sliderWidget.labelString = sliderWidget.customText;
            sliderWidget.valueString = qsTranslate("Abbreviation for not available", "N/A");
            return;
        }

        sliderWidget.resolvedFeature = (sliderWidget.feature !== "" && sliderWidget.feature !== "default")
                ? sliderWidget.feature : defaultFeatureForType(entityObj.type);

        let min = 0;
        let max = 100;
        let step = 1;
        let buttonStep = 0;

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
                // the brightness is shown in percent: a button changes it by one
                buttonStep = 255 / 100;
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
        sliderWidget.buttonStepSize = buttonStep > 0 ? buttonStep : step;

        sliderWidget.labelString = sliderWidget.showName ? entityObj.name : sliderWidget.customText;

        if (!sliderWidget.supported) {
            console.log("Slider widget: unsupported feature '" + sliderWidget.resolvedFeature
                        + "' for entity " + sliderWidget.entityId);
            sliderWidget.valueString = qsTranslate("Abbreviation for not available", "N/A");
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
        if (!entityObj || !sliderWidget.supported || sliderWidget.dragging) {
            return;
        }

        fallbackTimer.stop();
        sliderWidget.lastSentValue = NaN;
        sliderWidget.displayValue = entityValue();
        sliderWidget.trackValue = sliderWidget.displayValue;

        if (sliderWidget.unavailable) {
            sliderWidget.valueString = "--";
        } else if (entityObj.type === EntityTypes.Number && !entityObj.hasValue) {
            // unknown: no value
            sliderWidget.valueString = qsTranslate("Abbreviation for not available", "N/A");
        } else {
            sliderWidget.valueString = formatValue(sliderWidget.displayValue);
        }
    }

    // a discrete slider follows the finger between its positions and snaps on release, any other snaps at once
    function dragTo(value) {
        const snapped = SliderStyle.snap(value, sliderWidget.rangeMin, sliderWidget.rangeMax, sliderWidget.stepSize);
        sliderWidget.trackValue = sliderWidget.discrete ? value : snapped;

        if (snapped !== sliderWidget.displayValue) {
            sliderWidget.displayValue = snapped;
            sliderWidget.valueString = formatValue(snapped);
            Haptic.play(Haptic.Bump);
        }

        if (sliderWidget.live && !liveTimer.running) {
            liveTimer.start();
        }
    }

    function endDrag(changed) {
        liveTimer.stop();
        sliderWidget.dragging = false;

        if (sliderWidget.refused) {
            // nothing was sent: show the entity value again
            sliderWidget.refused = false;
            sliderWidget.syncFromEntity();
            return;
        }

        if (changed || !isNaN(sliderWidget.lastSentValue)) {
            sliderWidget.trackValue = sliderWidget.displayValue;
            sliderWidget.sendValue(sliderWidget.displayValue);
        } else {
            // a tap on a bar, or a drag back to where it started
            sliderWidget.syncFromEntity();
        }
    }

    function stepBy(up) {
        if (!entityObj || !sliderWidget.supported || !commandAllowed()) {
            return;
        }

        if (entityObj.type === EntityTypes.Number) {
            // the core knows the step
            if (up) {
                entityObj.increment();
            } else {
                entityObj.decrement();
            }
            return;
        }

        const value = SliderStyle.snap(sliderWidget.displayValue + (up ? 1 : -1) * sliderWidget.buttonStepSize,
                                       sliderWidget.rangeMin, sliderWidget.rangeMax, sliderWidget.stepSize);
        if (value === sliderWidget.displayValue) {
            return;
        }

        sliderWidget.displayValue = value;
        sliderWidget.trackValue = value;
        sliderWidget.valueString = formatValue(value);
        sliderWidget.sendValue(value);
    }

    Timer {
        // live mode: send while dragging, at most every 200 ms
        id: liveTimer
        interval: 200
        repeat: true
        running: false
        onTriggered: {
            if (!sliderWidget.dragging) {
                liveTimer.stop();
                return;
            }
            sliderWidget.sendValue(sliderWidget.displayValue);
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

    // default and bar style
    GridLayout {
        id: sliderLayout
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 4
        anchors.bottomMargin: 4
        // horizontal default: a column for each icon, an empty one would take a share of the header's width
        columns: sliderWidget.vertical || sliderWidget.bar ? 1
                                                           : 1 + (sliderWidget.iconMin !== "" ? 1 : 0) + (sliderWidget.iconMax !== "" ? 1 : 0)
        rowSpacing: 0
        columnSpacing: 6
        visible: !sliderWidget.buttons
        opacity: sliderWidget.supported ? 1 : ui.disabledOpacity

        // header: label and value
        RowLayout {
            Layout.fillWidth: true
            Layout.columnSpan: sliderLayout.columns
            spacing: 4
            visible: !sliderWidget.bar && (sliderWidget.labelString !== "" || (sliderWidget.showValue && !sliderWidget.vertical)
                                           || sliderWidget.unavailable)

            Components.Icon {
                color: colors.red
                icon: "uc:link-slash"
                size: ui.sliderMarkSize
                visible: sliderWidget.unavailable
            }

            Text {
                Layout.fillWidth: true
                text: sliderWidget.labelString
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.textSecondary
                opacity: sliderWidget.unavailable ? ui.sliderUnavailableLabelOpacity : 1
                verticalAlignment: Text.AlignVCenter
                font: fonts.help()
            }

            Text {
                Layout.alignment: Qt.AlignRight
                text: sliderWidget.valueString
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.textPrimary
                opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignRight
                font: fonts.label()
                visible: sliderWidget.showValue && !sliderWidget.vertical
            }
        }

        Components.Icon {
            color: colors.textSecondary
            opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1
            icon: sliderWidget.iconMin
            size: ui.sliderIconSize
            visible: !sliderWidget.bar && !sliderWidget.vertical && sliderWidget.iconMin !== ""
        }

        Item {
            id: trackArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: sliderWidget.vertical ? 60 : 30

            Item {
                id: track

                readonly property real thickness: interaction.pressed ? ui.sliderTrackThinPressed : ui.sliderTrackThin
                readonly property real radius: sliderWidget.bar ? ui.sliderBarRadius
                                                                : (sliderWidget.vertical ? width : height) / 2

                // the default style is a thin pill that grows while pressed, the bar is the whole item
                width: sliderWidget.bar || !sliderWidget.vertical ? parent.width : thickness
                height: sliderWidget.bar || sliderWidget.vertical ? parent.height : thickness
                anchors.centerIn: parent
                opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1

                Behavior on width {
                    enabled: !sliderWidget.bar && sliderWidget.vertical
                    NumberAnimation { duration: ui.sliderAnimationDuration; easing.type: Easing.OutExpo }
                }
                Behavior on height {
                    enabled: !sliderWidget.bar && !sliderWidget.vertical
                    NumberAnimation { duration: ui.sliderAnimationDuration; easing.type: Easing.OutExpo }
                }

                // the feature gradient over the whole range, for a cursor on a gradient track
                Gradient {
                    id: trackGradient
                    orientation: sliderWidget.vertical ? Gradient.Vertical : Gradient.Horizontal
                    GradientStop { position: 0; color: sliderWidget.gradientAt(sliderWidget.vertical ? 1 : 0) }
                    GradientStop { position: 1; color: sliderWidget.gradientAt(sliderWidget.vertical ? 0 : 1) }
                }

                // the part of the feature gradient under the fill
                Gradient {
                    id: fillGradient
                    orientation: sliderWidget.vertical ? Gradient.Vertical : Gradient.Horizontal
                    GradientStop { position: 0; color: sliderWidget.gradientAt(sliderWidget.vertical ? sliderWidget.fillTo : sliderWidget.fillFrom) }
                    GradientStop { position: 1; color: sliderWidget.gradientAt(sliderWidget.vertical ? sliderWidget.fillFrom : sliderWidget.fillTo) }
                }

                Rectangle {
                    anchors.fill: parent
                    radius: track.radius
                    color: sliderWidget.bar ? colors.surface : colors.surfaceRaised
                    gradient: sliderWidget.fillMode === SliderStyle.Cursor && sliderWidget.paint === SliderStyle.Gradient
                              ? trackGradient : undefined
                }

                Rectangle {
                    // fill from the start or from zero; vertical fills from the bottom
                    id: fillRect
                    x: sliderWidget.vertical ? 0 : sliderWidget.fillFrom * track.width
                    y: sliderWidget.vertical ? (1 - sliderWidget.fillTo) * track.height : 0
                    width: sliderWidget.vertical ? track.width : (sliderWidget.fillTo - sliderWidget.fillFrom) * track.width
                    height: sliderWidget.vertical ? (sliderWidget.fillTo - sliderWidget.fillFrom) * track.height : track.height
                    radius: track.radius
                    color: sliderWidget.paintColor
                    gradient: sliderWidget.paint === SliderStyle.Gradient ? fillGradient : undefined
                    visible: sliderWidget.fillMode !== SliderStyle.Cursor
                }

                Rectangle {
                    // zero line of a center fill
                    x: sliderWidget.vertical ? 0 : sliderWidget.fraction(0) * track.width - width / 2
                    y: sliderWidget.vertical ? (1 - sliderWidget.fraction(0)) * track.height - height / 2 : 0
                    width: sliderWidget.vertical ? track.width : ui.sliderTickSize.width
                    height: sliderWidget.vertical ? ui.sliderTickSize.width : track.height
                    color: colors.textSecondary
                    visible: sliderWidget.fillMode === SliderStyle.Center
                }

                Repeater {
                    // separators between the positions of a discrete bar
                    model: sliderWidget.bar && sliderWidget.discrete ? Math.max(0, sliderWidget.positionCount - 2) : 0

                    Rectangle {
                        readonly property real at: sliderWidget.fraction(Math.min(sliderWidget.rangeMax,
                                                                                  sliderWidget.rangeMin + (index + 1) * sliderWidget.stepSize))
                        x: sliderWidget.vertical ? 0 : at * track.width - width / 2
                        y: sliderWidget.vertical ? (1 - at) * track.height - height / 2 : 0
                        width: sliderWidget.vertical ? track.width : ui.sliderTickSize.width
                        height: sliderWidget.vertical ? ui.sliderTickSize.width : track.height
                        color: colors.bg
                    }
                }

                Rectangle {
                    // cursor: a band over the full thickness with a grip
                    id: cursorBand
                    width: sliderWidget.vertical ? track.width : ui.sliderCursorWidth
                    height: sliderWidget.vertical ? ui.sliderCursorWidth : track.height
                    x: sliderWidget.vertical ? 0 : sliderWidget.valueFraction * (track.width - width)
                    y: sliderWidget.vertical ? (1 - sliderWidget.valueFraction) * (track.height - height) : 0
                    radius: ui.cornerRadiusSmall
                    color: sliderWidget.paint === SliderStyle.Gradient ? colors.textPrimary : sliderWidget.paintColor
                    visible: sliderWidget.fillMode === SliderStyle.Cursor

                    Rectangle {
                        anchors.centerIn: parent
                        width: sliderWidget.vertical ? Math.min(ui.sliderGripSize.height, parent.width / 2) : ui.sliderGripSize.width
                        height: sliderWidget.vertical ? ui.sliderGripSize.width : Math.min(ui.sliderGripSize.height, parent.height / 2)
                        radius: ui.sliderGripSize.width / 2
                        color: colors.sliderGrip
                    }
                }
            }

            Repeater {
                // tick marks of a discrete default slider, beside the track
                model: !sliderWidget.bar && sliderWidget.discrete ? sliderWidget.positionCount : 0

                Rectangle {
                    readonly property real at: sliderWidget.fraction(Math.min(sliderWidget.rangeMax,
                                                                              sliderWidget.rangeMin + index * sliderWidget.stepSize))
                    readonly property real offset: ui.sliderTrackThinPressed / 2 + ui.sliderTickSize.width
                    x: sliderWidget.vertical ? trackArea.width / 2 + offset : at * (trackArea.width - width)
                    y: sliderWidget.vertical ? (1 - at) * (trackArea.height - height) : trackArea.height / 2 + offset
                    width: sliderWidget.vertical ? ui.sliderTickSize.height : ui.sliderTickSize.width
                    height: sliderWidget.vertical ? ui.sliderTickSize.width : ui.sliderTickSize.height
                    color: colors.textSecondary
                }
            }

            Repeater {
                // bar contents, twice: as they are, and in the colour over the fill, clipped to it
                model: sliderWidget.bar ? 2 : 0

                Item {
                    id: barLayer

                    readonly property bool overFill: index === 1
                    readonly property bool cursor: sliderWidget.fillMode === SliderStyle.Cursor
                    readonly property bool gradientTrack: cursor && sliderWidget.paint === SliderStyle.Gradient
                    // the colour of the fill under the contents
                    readonly property color fillColor: {
                        if (sliderWidget.paint === SliderStyle.Gradient) {
                            return gradientTrack ? sliderWidget.gradientAt(0.5)
                                                 : sliderWidget.gradientAt((sliderWidget.fillFrom + sliderWidget.fillTo) / 2);
                        }
                        return cursor ? cursorBand.color : sliderWidget.paintColor;
                    }
                    readonly property color tint: SliderStyle.contrastColor(fillColor, colors.sliderOnFill, colors.textPrimary)

                    // the layer over the fill shows only where the fill is: the whole gradient track for a cursor
                    x: !overFill || gradientTrack ? 0 : (cursor ? cursorBand.x : fillRect.x)
                    y: !overFill || gradientTrack ? 0 : (cursor ? cursorBand.y : fillRect.y)
                    width: !overFill || gradientTrack ? trackArea.width : (cursor ? cursorBand.width : fillRect.width)
                    height: !overFill || gradientTrack ? trackArea.height : (cursor ? cursorBand.height : fillRect.height)
                    clip: overFill
                    visible: !overFill || !sliderWidget.unavailable

                    GridLayout {
                        x: -barLayer.x
                        y: -barLayer.y
                        width: trackArea.width
                        height: trackArea.height
                        columns: sliderWidget.vertical ? 1 : 5
                        rowSpacing: 0
                        columnSpacing: 6

                        // horizontal: [mark] [icon min] label value [icon max]
                        // vertical: [mark] label, value, [icon max], [icon min] at the bottom
                        Components.Icon {
                            Layout.alignment: Qt.AlignCenter
                            Layout.leftMargin: sliderWidget.vertical ? 0 : ui.sliderBarRadius
                            Layout.topMargin: sliderWidget.vertical ? ui.sliderBarRadius : 0
                            color: colors.red
                            icon: "uc:link-slash"
                            size: ui.sliderMarkSize
                            visible: sliderWidget.unavailable
                        }

                        Components.Icon {
                            Layout.alignment: Qt.AlignCenter
                            Layout.leftMargin: sliderWidget.unavailable ? 0 : ui.sliderBarRadius
                            color: barLayer.overFill ? barLayer.tint : colors.textSecondary
                            opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1
                            icon: sliderWidget.iconMin
                            size: ui.sliderIconSize
                            visible: !sliderWidget.vertical && sliderWidget.iconMin !== ""
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.leftMargin: sliderWidget.vertical || sliderWidget.unavailable || sliderWidget.iconMin !== ""
                                               ? 0 : ui.sliderBarRadius
                            Layout.topMargin: sliderWidget.vertical && !sliderWidget.unavailable ? ui.sliderBarRadius : 0
                            text: sliderWidget.labelString
                            maximumLineCount: 1
                            elide: Text.ElideRight
                            color: barLayer.overFill ? barLayer.tint : colors.textSecondary
                            opacity: sliderWidget.unavailable ? ui.sliderUnavailableLabelOpacity : 1
                            horizontalAlignment: sliderWidget.vertical ? Text.AlignHCenter : Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            font: fonts.help()
                            visible: text !== ""
                        }

                        Text {
                            Layout.alignment: sliderWidget.vertical ? Qt.AlignHCenter : (Qt.AlignRight | Qt.AlignVCenter)
                            Layout.rightMargin: sliderWidget.vertical || sliderWidget.iconMax !== "" ? 0 : ui.sliderBarRadius
                            text: sliderWidget.valueString
                            maximumLineCount: 1
                            color: barLayer.overFill ? barLayer.tint : colors.textPrimary
                            opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1
                            verticalAlignment: Text.AlignVCenter
                            font: fonts.label()
                            visible: sliderWidget.showValue
                        }

                        Components.Icon {
                            Layout.alignment: Qt.AlignCenter
                            Layout.rightMargin: sliderWidget.vertical ? 0 : ui.sliderBarRadius
                            color: barLayer.overFill ? barLayer.tint : colors.textSecondary
                            opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1
                            icon: sliderWidget.iconMax
                            size: ui.sliderIconSize
                            visible: sliderWidget.iconMax !== ""
                        }

                        Item {
                            // vertical: pushes the minimum icon to the bottom
                            Layout.fillHeight: true
                            visible: sliderWidget.vertical
                        }

                        Components.Icon {
                            Layout.alignment: Qt.AlignCenter
                            Layout.bottomMargin: ui.sliderBarRadius
                            color: barLayer.overFill ? barLayer.tint : colors.textSecondary
                            opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1
                            icon: sliderWidget.iconMin
                            size: ui.sliderIconSize
                            visible: sliderWidget.vertical && sliderWidget.iconMin !== ""
                        }
                    }
                }
            }

            MouseArea {
                // A touch that starts on the slider belongs to it, not to the page swipe. The default style moves
                // the value to the finger, the bar by the dragged distance.
                id: interaction
                anchors.fill: parent
                // stays pressable while the entity is unavailable, so the press is refused with a
                // notification like on a tile - and gets through while the remote is waking up
                enabled: sliderWidget.supported && entityObj !== null
                preventStealing: true

                property real startValue: 0
                property real startPosition: 0
                property bool moved: false

                // the position along the track, growing towards the maximum
                function along(mouse) {
                    return sliderWidget.vertical ? -mouse.y : mouse.x;
                }

                function valueAt(mouse) {
                    const at = sliderWidget.vertical ? 1 - (mouse.y - track.y) / track.height
                                                     : (mouse.x - track.x) / track.width;
                    return sliderWidget.rangeMin + Math.max(0, Math.min(1, at)) * (sliderWidget.rangeMax - sliderWidget.rangeMin);
                }

                onPressed: {
                    sliderWidget.refused = !sliderWidget.commandAllowed();
                    interaction.startValue = sliderWidget.displayValue;
                    interaction.startPosition = along(mouse);
                    interaction.moved = false;
                    if (sliderWidget.refused) {
                        return;
                    }

                    sliderWidget.dragging = true;
                    if (!sliderWidget.bar) {
                        sliderWidget.dragTo(valueAt(mouse));
                    }
                }

                onPositionChanged: {
                    if (sliderWidget.refused) {
                        return;
                    }

                    if (!sliderWidget.bar) {
                        sliderWidget.dragTo(valueAt(mouse));
                        return;
                    }

                    const distance = along(mouse) - interaction.startPosition;
                    if (!interaction.moved && Math.abs(distance) < Qt.styleHints.startDragDistance) {
                        return;
                    }
                    interaction.moved = true;
                    sliderWidget.dragTo(SliderStyle.dragValue(interaction.startValue, distance,
                                                              sliderWidget.vertical ? track.height : track.width,
                                                              sliderWidget.rangeMin, sliderWidget.rangeMax));
                }

                onReleased: sliderWidget.endDrag(sliderWidget.displayValue !== interaction.startValue)
                onCanceled: sliderWidget.endDrag(sliderWidget.displayValue !== interaction.startValue)
            }
        }

        Components.Icon {
            color: colors.textSecondary
            opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1
            icon: sliderWidget.iconMax
            size: ui.sliderIconSize
            visible: !sliderWidget.bar && !sliderWidget.vertical && sliderWidget.iconMax !== ""
        }

        Text {
            // vertical layout: value below the slider
            Layout.fillWidth: true
            text: sliderWidget.valueString
            maximumLineCount: 1
            elide: Text.ElideRight
            color: colors.textPrimary
            opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1
            horizontalAlignment: Text.AlignHCenter
            font: fonts.help()
            visible: !sliderWidget.bar && sliderWidget.showValue && sliderWidget.vertical
        }
    }

    // buttons style: minus, value, plus; vertically plus on top
    ColumnLayout {
        anchors { fill: parent; leftMargin: 10; rightMargin: 10; topMargin: 4; bottomMargin: 4 }
        spacing: 0
        visible: sliderWidget.buttons
        opacity: sliderWidget.supported ? 1 : ui.disabledOpacity

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: sliderWidget.labelString !== "" || sliderWidget.unavailable

            Components.Icon {
                color: colors.red
                icon: "uc:link-slash"
                size: ui.sliderMarkSize
                visible: sliderWidget.unavailable
            }

            Text {
                Layout.fillWidth: true
                text: sliderWidget.labelString
                maximumLineCount: 1
                elide: Text.ElideRight
                color: colors.textSecondary
                opacity: sliderWidget.unavailable ? ui.sliderUnavailableLabelOpacity : 1
                verticalAlignment: Text.AlignVCenter
                font: fonts.help()
            }
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: sliderWidget.vertical ? 1 : 3
            rowSpacing: 4
            columnSpacing: 4

            Repeater {
                model: sliderWidget.buttons ? (sliderWidget.vertical ? ["up", "value", "down"] : ["down", "value", "up"]) : []

                Item {
                    Layout.fillWidth: modelData === "value"
                    Layout.fillHeight: true
                    Layout.minimumWidth: ui.sliderButtonSize
                    Layout.minimumHeight: ui.sliderButtonSize
                    Layout.preferredWidth: modelData === "value" ? -1 : ui.sliderButtonSize

                    Text {
                        anchors.fill: parent
                        visible: modelData === "value"
                        text: sliderWidget.showValue ? sliderWidget.valueString : ""
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        color: colors.textPrimary
                        opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: fonts.label()
                    }

                    Rectangle {
                        width: ui.sliderButtonSize
                        height: ui.sliderButtonSize
                        anchors.centerIn: parent
                        visible: modelData !== "value"
                        radius: width / 2
                        color: stepMouseArea.pressed ? colors.textPrimary : colors.surfaceRaised
                        opacity: sliderWidget.unavailable ? ui.sliderUnavailableOpacity : 1

                        Components.Icon {
                            anchors.centerIn: parent
                            color: stepMouseArea.pressed ? colors.bg : colors.textPrimary
                            icon: modelData === "up" ? "uc:plus" : "uc:minus"
                            size: ui.sliderIconSize
                        }

                        Components.HapticMouseArea {
                            id: stepMouseArea
                            anchors.fill: parent
                            enabled: sliderWidget.supported && entityObj !== null
                            onClicked: sliderWidget.stepBy(modelData === "up")
                        }
                    }
                }
            }
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
