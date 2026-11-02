// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 NUMBER ENTITY SCREEN

 The representation follows the effective display mode of the entity: a vertical slider, or the value with decrement
 and increment buttons (stepper). An unbounded number is always a stepper.
 See https://github.com/unfoldedcircle/core-api/blob/main/doc/entities/entity_number.md#display-mode
**/

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0
import Entity.Number 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: numberBase

    function displayText() {
        let value;
        if (entityObj.sliderRepresentation && valueSlider.pressed) {
            value = entityObj.formatValue(valueSlider.value);
        } else if (entityObj.hasValue) {
            value = entityObj.formattedValue;
        } else {
            value = qsTranslate("Abbreviation for not available", "N/A");
        }
        return value + (entityObj.unit !== "" ? " " + entityObj.unit : "");
    }

    overrideConfig: {
        "DPAD_UP": {
            "pressed": function() {
                entityObj.increment();
            }
        },
        "DPAD_DOWN": {
            "pressed": function() {
                entityObj.decrement();
            }
        },
        "VOLUME_UP": {
            "pressed": function() {
                entityObj.increment();
            }
        },
        "VOLUME_DOWN": {
            "pressed": function() {
                entityObj.decrement();
            }
        },
        "PREV": {
            "pressed": function() {
                if (entityObj.hasMin) {
                    entityObj.setValue(entityObj.min);
                }
            }
        },
        "NEXT": {
            "pressed": function() {
                if (entityObj.hasMax) {
                    entityObj.setValue(entityObj.max);
                }
            }
        }
    }

    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        title: entityObj.name
    }

    Text {
        id: valueText
        text: numberBase.displayText()
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        elide: Text.ElideRight
        maximumLineCount: 2
        horizontalAlignment: Text.AlignHCenter
        color: colors.textPrimary
        anchors {
            left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20
            top: entityObj.sliderRepresentation ? undefined : title.bottom
            topMargin: 80
            bottom: entityObj.sliderRepresentation ? valueSlider.top : undefined
            bottomMargin: 10
        }
        font: fonts.display(entityObj.unit !== "" ? 90 : 140)
    }

    Slider {
        id: valueSlider
        visible: entityObj.sliderRepresentation
        enabled: visible
        live: true
        width: parent.width - 60
        height: 500
        snapMode: Slider.SnapAlways
        padding: 0
        orientation: Qt.Vertical

        transformOrigin: Item.Center
        rotation: 180
        anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 30 }

        // rotated by 180 degrees: from is the top, to the bottom
        from: entityObj.max
        to: entityObj.min
        stepSize: entityObj.step
        value: entityObj.value

        background: Item {
            implicitWidth: valueSlider.width; implicitHeight: valueSlider.height
            width: valueSlider.availableWidth; height: implicitHeight

            Rectangle {
                width: parent.width; height: valueSlider.height
                anchors.centerIn: parent
                radius: ui.cornerRadiusSmall
                color: colors.surfaceRaised

                Rectangle {
                    height: valueSlider.visualPosition * parent.height; width: parent.width
                    color: colors.textPrimary
                    radius: ui.cornerRadiusSmall
                    visible: entityObj.hasValue || valueSlider.pressed
                }
            }
        }

        handle: Item {
            x: (valueSlider.visualPosition * valueSlider.availableWidth) - width/2
            y: valueSlider.topPadding + valueSlider.availableHeight / 2 - height / 2
            implicitWidth: 40; implicitHeight: 40
        }

        onMoved: Haptic.play(Haptic.Bump)

        onPressedChanged: {
            if (!valueSlider.pressed) {
                entityObj.setValue(value);
            }
        }

        Components.Icon {
            color: colors.textSecondary
            icon: "uc:plus"
            size: 60
            rotation: 180
            anchors { right: parent.right; rightMargin: 10; bottom: parent.bottom; bottomMargin: 20 }
        }

        Components.Icon {
            color: colors.textSecondary
            icon: "uc:minus"
            size: 60
            rotation: 180
            anchors { right: parent.right; rightMargin: 10; top: parent.top; topMargin: 20 }
        }
    }

    Row {
        id: stepper
        visible: !entityObj.sliderRepresentation
        spacing: 20
        anchors { horizontalCenter: parent.horizontalCenter; top: valueText.bottom; topMargin: 60 }

        Repeater {
            model: [
                { "icon": "uc:minus", "atLimit": entityObj.hasMin && entityObj.hasValue && entityObj.value <= entityObj.min },
                { "icon": "uc:plus", "atLimit": entityObj.hasMax && entityObj.hasValue && entityObj.value >= entityObj.max }
            ]

            Rectangle {
                width: 180; height: 140
                radius: ui.cornerRadiusSmall
                color: stepMouseArea.pressed ? colors.textPrimary : colors.surface
                opacity: modelData.atLimit ? 0.4 : 1

                Components.Icon {
                    color: stepMouseArea.pressed ? colors.bg : colors.textPrimary
                    icon: modelData.icon
                    size: 80
                    anchors.centerIn: parent
                }

                Components.HapticMouseArea {
                    id: stepMouseArea
                    anchors.fill: parent
                    enabled: !modelData.atLimit
                    onClicked: {
                        if (index === 0) {
                            entityObj.decrement();
                        } else {
                            entityObj.increment();
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: entityObj
        ignoreUnknownSignals: true

        function onValueChanged() {
            if (!valueSlider.pressed) {
                valueSlider.value = entityObj.value;
            }
        }
    }
}
