import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import Haptic 1.0
import TouchSlider 1.0
import Config 1.0

import "qrc:/components" as Components

Item {
    id: sliderContainer
    width: ui.width; height: 300

    property QtObject entityObj
    property bool touchSliderActive: false
    property double prevTouchX: 0
    property int sliderAnimationDuration: 200
    property int targetVolume: 0
    property bool unavailableWarningShown: false
    property double lastHapticMs: 0

    // value units per raw slider unit, computed at gesture start
    property real touchScale: 0
    // fractional value change accumulator, keeps sub-unit finger motion
    property real deltaAcc: 0
    // how much of the value range a full length swipe covers
    readonly property real sweepGain: Config.touchSliderGainVolume
    // raw axis range to assume when the driver does not report one
    readonly property int fallbackAxisRange: 300

    signal open()
    signal close()

    function caclulateSliderWidth(volume) {
        return sliderContentVisual.width * volume / 100
    }

    Connections {
        target: sliderContainer.entityObj
        ignoreUnknownSignals: true

        function onEntityEnabledChanged() {
            if (sliderContainer.entityObj.enabled) {
                sliderContainer.unavailableWarningShown = false;
            }
        }
    }

    Connections {
        target: TouchSliderProcessor
        ignoreUnknownSignals: true

        function onTouchPressed() {
            console.log("Touch pressed");

            // if entity is unavalable we do nothing, but only warn once
            if (!entityObj.enabled) {
                if (!sliderContainer.unavailableWarningShown) {
                    ui.createActionableNotification(qsTr("Touch slider is not available."),
                                                    qsTr("%1 is not available. Please check your configuration.").arg(entityObj.name),
                                                    "uc:link-slash");
                    sliderContainer.unavailableWarningShown = true;
                }
                return;
            }

            // entity is available: clear any stale warning latch so the gesture is never
            // silently blocked after the entity recovers (e.g. reloaded on resume)
            sliderContainer.unavailableWarningShown = false;

            sliderContainer.touchSliderActive = true;

            releaseTimer.stop();
            sliderContainer.prevTouchX = TouchSliderProcessor.touchX;
            sliderContainer.deltaAcc = 0;
            let axisRange = TouchSliderProcessor.touchXMax - TouchSliderProcessor.touchXMin;
            if (axisRange <= 0) {
                axisRange = sliderContainer.fallbackAxisRange;
            }
            sliderContainer.touchScale = 100 * sliderContainer.sweepGain / axisRange;
            sliderContentVisualValue.width = caclulateSliderWidth(entityObj.volume);
            sliderContentValueText.text = entityObj.volume;
            sliderContainer.targetVolume = entityObj.volume;

            sliderContainer.open();

            updateDataTimer.start();
        }

        function onTouchXChanged(x) {
            // if entity is unavalable or the gesture is not active we do nothing
            if (!entityObj.enabled || !sliderContainer.touchSliderActive) {
                return;
            }

            // proportional mapping: the finger distance determines the value change,
            // sub-unit motion accumulates in deltaAcc so slow drags still register
            sliderContainer.deltaAcc += (x - sliderContainer.prevTouchX) * sliderContainer.touchScale;
            sliderContainer.prevTouchX = x;

            const step = sliderContainer.deltaAcc > 0 ? Math.floor(sliderContainer.deltaAcc)
                                                      : Math.ceil(sliderContainer.deltaAcc);
            if (step === 0) {
                return;
            }
            sliderContainer.deltaAcc -= step;

            const newTarget = Math.max(0, Math.min(100, sliderContainer.targetVolume + step));
            if (newTarget === sliderContainer.targetVolume) {
                // pinned at an end stop
                return;
            }
            sliderContainer.targetVolume = newTarget;

            // rate limit haptic feedback, fast swipes generate a lot of value changes
            const now = Date.now();
            if (now - sliderContainer.lastHapticMs >= 30) {
                Haptic.play(Haptic.Bump);
                sliderContainer.lastHapticMs = now;
            }

            sliderContentVisualValue.width = caclulateSliderWidth(sliderContainer.targetVolume);
            sliderContentValueText.text = sliderContainer.targetVolume;
        }

        function onTouchReleased() {
            console.log("Touch released");

            // if entity is unavalable we do nothing
            if (!entityObj.enabled) {
                return;
            }

            updateDataTimer.stop();

            if (entityObj.volume != sliderContainer.targetVolume) {
                entityObj.setVolume(sliderContainer.targetVolume);
                sliderContentValueText.text = sliderContainer.targetVolume;
            }
            sliderContainer.touchSliderActive = false;

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
                icon: "uc:volume"
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
                width: caclulateSliderWidth(entityObj.volume)
                height: 12

                Behavior on width {
                    NumberAnimation { duration: sliderContainer.sliderAnimationDuration; easing.type: Easing.OutExpo }
                }

                Connections {
                    target: entityObj
                    ignoreUnknownSignals: true

                    function onVolumeChanged() {
                        if (!sliderContainer.touchSliderActive) {
                            sliderContentVisualValue.width = caclulateSliderWidth(entityObj.volume);
                            sliderContentValueText.text = entityObj.volume;
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
                text: entityObj.volume
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
            if (entityObj.volume != sliderContainer.targetVolume) {
                entityObj.setVolume(sliderContainer.targetVolume);
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
