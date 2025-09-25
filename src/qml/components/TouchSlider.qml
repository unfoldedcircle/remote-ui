import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import Entity.Controller 1.0
import Entity.MediaPlayer 1.0
import HwInfo 1.0
import Haptic 1.0
import TouchSlider 1.0

import "qrc:/components" as Components

Popup {
    id: touchSlider
    width: ui.width; height: ui.height
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0
    y: 500

    property QtObject entityObj
    property bool active: true
    property bool isSupported: true
    property bool touchSliderActive: false
    property double prevTouchX: 0
    property int volumeDelta: 0
    property int sliderAnimationDuration: 200
    property int targetVolume: 0
    property int lastRawDelta: 0

    function caclulateSliderWidth(volume) {
        return sliderContentVisual.width * volume / 100
    }

    onEntityObjChanged: {
        console.debug("[Touch Slider] Current entity changed:", entityObj.id);
        if (entityObj.type === EntityTypes.Media_player && entityObj.hasFeature(MediaPlayerFeatures.Volume)) {
            touchSlider.isSupported = true;
            console.debug("Slider supported");
        } else {
            touchSlider.isSupported = false;
            console.debug("Slider not supported");
        }
    }

    enter: Transition {
        PropertyAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.OutExpo; duration: 300 }
        PropertyAnimation { properties: "y"; from: 500; to: 0; easing.type: Easing.OutExpo; duration: 300 }
    }

    exit: Transition {
        PropertyAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.InExpo; duration: 300 }
        PropertyAnimation { properties: "y"; from: 0; to: 500; easing.type: Easing.InExpo; duration: 300 }
    }

    background: Item {}

    Item {
        id: sliderContainer
        width: ui.width; height: 300
        x: 0
        y: ui.height - sliderContainer.height

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
                    icon: touchSlider.isSupported ? "uc:volume" : "uc:ban"
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
                    width: touchSlider.isSupported ? caclulateSliderWidth(entityObj.volume) : 0
                    height: 12

                    Behavior on width {
                        NumberAnimation { duration: touchSlider.sliderAnimationDuration; easing.type: Easing.OutExpo }
                    }

                    Connections {
                        target: entityObj
                        ignoreUnknownSignals: true

                        function onVolumeChanged() {
                            if (!touchSlider.touchSliderActive) {
                                sliderContentVisualValue.width = touchSlider.isSupported ? caclulateSliderWidth(entityObj.volume) : 0;
                                sliderContentValueText.text = touchSlider.isSupported ? entityObj.volume : 0
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
                    text: touchSlider.isSupported ? entityObj.volume : 0
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
            text: touchSlider.isSupported ? entityObj.name : qsTr("Not supported")
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
    }

    Connections {
        target: TouchSliderProcessor
        ignoreUnknownSignals: true
        enabled: touchSlider.active

        function onTouchPressed() {
            console.log("Touch pressed");

            touchSlider.touchSliderActive = true;

            releaseTimer.stop();
            touchSlider.prevTouchX = TouchSliderProcessor.touchX;
            sliderContentVisualValue.width = touchSlider.isSupported ? caclulateSliderWidth(entityObj.volume) : 0;
            sliderContentValueText.text = touchSlider.isSupported ? entityObj.volume : 0;
            touchSlider.targetVolume = entityObj.volume;

            touchSlider.open();

            updateVolumeTimer.start();
        }

        function onTouchXChanged(x) {
            console.log("Touch x: ", x);

            if (!touchSlider.isSupported) {
                return;
            }

            // Calculate the raw delta
            const rawDelta = TouchSliderProcessor.touchX - touchSlider.prevTouchX;
            touchSlider.lastRawDelta = rawDelta;

            // We need minimum 5 pixel movement, otherwise it's way too sensitive
            if (Math.abs(rawDelta) < 5) {
                touchSlider.lastRawDelta = 0;
                return;
            }

            if (touchSlider.touchSliderActive) {
                Haptic.play(Haptic.Bump);
            }

            touchSlider.targetVolume += Math.sign(rawDelta);
            touchSlider.targetVolume = Math.max(0, Math.min(100, touchSlider.targetVolume));

            sliderContentVisualValue.width = caclulateSliderWidth(touchSlider.targetVolume);
            sliderContentValueText.text =touchSlider.targetVolume;

            // Update previous touch position
            touchSlider.prevTouchX = TouchSliderProcessor.touchX;
        }

        function onTouchReleased() {
            console.log("Touch released");

            updateVolumeTimer.stop();

            // we do a last check to see if one just swiped across fast
            if (touchSlider.lastRawDelta != 0) {
                const rawDelta = touchSlider.lastRawDelta;
                touchSlider.sliderAnimationDuration = Math.abs(rawDelta) > 60 ? 1000 : 200;

                // Limit the maximum effective delta to 10
                touchSlider.volumeDelta = Math.sign(rawDelta) * (Math.abs(rawDelta) > 60 ? 10 : 1);
                touchSlider.targetVolume += touchSlider.volumeDelta;
                touchSlider.targetVolume = Math.max(0, Math.min(100, touchSlider.targetVolume));
            }

            if (entityObj.volume != touchSlider.targetVolume) {
                entityObj.setVolume(touchSlider.targetVolume);
                sliderContentValueText.text = touchSlider.targetVolume;
            }
            touchSlider.volumeDelta = 0;
            touchSlider.touchSliderActive = false;
            touchSlider.lastRawDelta = 0;

            releaseTimer.start();
        }
    }

    Timer {
        id: updateVolumeTimer
        running: false
        repeat: true
        interval: 200
        onTriggered: {
            if (entityObj.volume != touchSlider.targetVolume) {
                entityObj.setVolume(touchSlider.targetVolume);
            }
        }
    }

    Timer {
        id: releaseTimer
        running: false
        repeat: false
        interval: 1000
        onTriggered: touchSlider.close()
    }
}
