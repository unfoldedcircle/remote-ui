import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import TouchSlider 1.0

import "qrc:/components" as Components

Item {
    id: sliderContainer
    width: ui.width; height: 300

    property QtObject entityObj
    property bool touchSliderActive: false
    property double prevTouchX: 0
    property int brightnessDelta: 0
    property int sliderAnimationDuration: 200
    property int targetBrightness: 0
    property int lastRawDelta: 0

    signal open()
    signal close()

    function caclulateSliderWidth(brightness) {
        return sliderContentVisual.width * brightness / 255;
    }

    Connections {
        target: TouchSliderProcessor
        ignoreUnknownSignals: true

        function onTouchPressed() {
            console.log("Touch pressed");

            sliderContainer.touchSliderActive = true;

            releaseTimer.stop();
            sliderContainer.prevTouchX = TouchSliderProcessor.touchX;
            sliderContentVisualValue.width = caclulateSliderWidth(entityObj.brightness);
            sliderContentValueText.text = Math.round(entityObj.brightness / 255 * 100);
            sliderContainer.targetBrightness = entityObj.brightness;

            sliderContainer.open();

            updateDataTimer.start();
        }

        function onTouchXChanged(x) {
            console.log("Touch x: ", x);

            // Calculate the raw delta
            const rawDelta = TouchSliderProcessor.touchX - sliderContainer.prevTouchX;
            sliderContainer.lastRawDelta = rawDelta;

            // We need minimum 5 pixel movement, otherwise it's way too sensitive
            if (Math.abs(rawDelta) < 5) {
                sliderContainer.lastRawDelta = 0;
                return;
            }

            if (sliderContainer.touchSliderActive) {
                Haptic.play(Haptic.Bump);
            }

            sliderContainer.targetBrightness += Math.sign(rawDelta);
            sliderContainer.targetBrightness = Math.max(0, Math.min(255, sliderContainer.targetBrightness));

            sliderContentVisualValue.width = caclulateSliderWidth(sliderContainer.targetBrightness);
            sliderContentValueText.text = Math.round(sliderContainer.targetBrightness / 255 * 100);

            // Update previous touch position
            sliderContainer.prevTouchX = TouchSliderProcessor.touchX;
        }

        function onTouchReleased() {
            console.log("Touch released");

            updateDataTimer.stop();

            // we do a last check to see if one just swiped across fast
            if (sliderContainer.lastRawDelta != 0) {
                const rawDelta = sliderContainer.lastRawDelta;
                sliderContainer.sliderAnimationDuration = Math.abs(rawDelta) > 60 ? 1000 : 200;

                // Limit the maximum effective delta to 10
                sliderContainer.brightnessDelta = Math.sign(rawDelta) * (Math.abs(rawDelta) > 60 ? 10 : 1);
                sliderContainer.targetBrightness += sliderContainer.brightnessDelta;
                sliderContainer.targetBrightness = Math.max(0, Math.min(255, sliderContainer.targetBrightness));
            }

            if (entityObj.brightness != sliderContainer.targetBrightness) {
                entityObj.setBrightness(sliderContainer.targetBrightness);
                sliderContentValueText.text = Math.round(sliderContainer.targetBrightness / 255 * 100);
            }
            sliderContainer.brightnessDelta = 0;
            sliderContainer.touchSliderActive = false;
            sliderContainer.lastRawDelta = 0;

            releaseTimer.start();
        }
    }

    LinearGradient {
        anchors.fill: parent
        start: Qt.point(0, 0)
        end: Qt.point(0, parent.height)
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#00000000" }
            GradientStop { position: 0.5; color: colors.black }
            GradientStop { position: 1.0; color: colors.black }
        }
    }

    Rectangle {
        id: sliderContent
        width: parent.width - 20
        height: 100
        color: colors.transparent
        border { width: 1; color: Qt.hsla(colors.white.hslHue, colors.white.hslSaturation, colors.white.hslLightness, 0.3) }
        radius: height / 2
        anchors { bottom: parent.bottom; bottomMargin: 10; horizontalCenter: parent.horizontalCenter }

        Item {
            id: sliderContentIcon
            width: 100
            height: 100
            anchors { left: parent.left }

            Components.Icon {
                color: colors.white
                anchors.centerIn: parent
                icon: "uc:brightness"
                size: 80
            }
        }

        Rectangle {
            id: sliderContentVisual
            color: colors.transparent
            border { width: 1; color: colors.white }
            radius: 6
            height: 12
            anchors { verticalCenter: parent.verticalCenter;  left: sliderContentIcon.right; right: sliderContentValue.left }

            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource:
                    Rectangle {
                    width: sliderContentVisual.width
                    height: sliderContentVisual.height
                    radius: sliderContentVisual.radius
                }
            }

            Rectangle {
                id: sliderContentVisualValue
                color: colors.white
                x: 0
                width: caclulateSliderWidth(entityObj.brightness)
                height: 12

                Behavior on width {
                    NumberAnimation { duration: sliderContainer.sliderAnimationDuration; easing.type: Easing.OutExpo }
                }

                Connections {
                    target: entityObj
                    ignoreUnknownSignals: true

                    function onBrightnessChanged() {
                        if (!sliderContainer.touchSliderActive) {
                            sliderContentVisualValue.width = caclulateSliderWidth(entityObj.brightness);
                            sliderContentValueText.text = Math.rodun(entityObj.brightness / 255 * 100);
                        }
                    }
                }
            }
        }

        Item {
            id: sliderContentValue
            width: 100
            height: 100
            anchors { right: parent.right }

            Text {
                id: sliderContentValueText
                text: entityObj.brightness
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.NoWrap
                maximumLineCount: 1
                color: colors.white
                font: fonts.primaryFont(40)
                anchors.fill: parent
            }
        }
    }

    Text {
        text: entityObj.name
        width: parent.width - 20
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        maximumLineCount: 1
        elide: Text.ElideRight
        color: colors.white
        font: fonts.primaryFont(30)
        anchors { bottom: sliderContent.top; bottomMargin: 20; horizontalCenter: parent.horizontalCenter; }
    }

    Timer {
        id: updateDataTimer
        running: false
        repeat: true
        interval: 200
        onTriggered: {
            if (entityObj.brightness != sliderContainer.targetBrightness) {
                entityObj.setBrightness(sliderContainer.targetBrightness);
            }
        }
    }

    Timer {
        id: releaseTimer
        running: false
        repeat: false
        interval: 1000
        onTriggered: sliderContainer.close()
    }
}
