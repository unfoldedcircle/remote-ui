import QtQuick 2.15
import QtQuick.Controls 2.15

import Entity.Controller 1.0
import Entity.Light 1.0
import Entity.Cover 1.0
import Entity.MediaPlayer 1.0

import HwInfo 1.0

import "qrc:/components" as Components

Popup {
    id: touchSlider
    width: ui.width; height: ui.height
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0
    y: 500

    property QtObject entityObj
    property string feature: "volume"
    property bool active: false

    property var requiredMediaPlayerFeaturesForVolume: [MediaPlayerFeatures.Volume]
    property var requiredMediaPlayerFeaturesForSeek: [MediaPlayerFeatures.Seek, MediaPlayerFeatures.Media_duration, MediaPlayerFeatures.Media_position]
    property var requiredLightFeaturesForBrightness: [LightFeatures.Dim]
    property var requiredCoverFeaturesForPosition: [CoverFeatures.Position]

    function startSetup() {
        if (HwInfo.modelNumber != "UCR3" && HwInfo.modelNumber != "DEV") {
            console.info("[Touch Slider] Disabled on this hardware");
            touchSlider.active = false;
            sliderLoader.source = "";
            return;
        }

        console.info("[Touch Slider] Starting setup. Feature:", feature, "Type:", entityObj.type, "Active:", active);

        switch (entityObj.type) {
        case EntityTypes.Media_player: {
            switch (touchSlider.feature) {
            case "volume": {
                if (entityObj.hasAllFeatures(requiredMediaPlayerFeaturesForVolume)) {
                    console.info("Slider volume is supported");
                    sliderLoader.setSource("qrc:/components/TouchSliderVolume.qml", { "entityObj": entityObj });
                } else {
                    touchSlider.active = false;
                    console.info("Slider not supported");
                }
                break;
            }
            case "seek": {
                if (entityObj.hasAllFeatures(requiredMediaPlayerFeaturesForSeek)) {
                    console.info("Slider seek is supported");
                    sliderLoader.setSource("qrc:/components/TouchSliderSeek.qml", { "entityObj": entityObj });
                } else {
                    touchSlider.active = false;
                    console.info("Slider not supported");
                }
                break;
            }
            }
            break;
        }
        case EntityTypes.Light: {
            switch (touchSlider.feature) {
            case "dim": {
                if (entityObj.hasAllFeatures(requiredLightFeaturesForBrightness)) {
                    console.info("Slider brightness is supported");
                    sliderLoader.setSource("qrc:/components/TouchSliderBrightness.qml", { "entityObj": entityObj });
                } else {
                    touchSlider.active = false;
                    console.info("Slider not supported");
                }
                break;
            }
            }
            break;
        }
        case EntityTypes.Cover: {
            switch (touchSlider.feature) {
            case "position": {
                if (entityObj.hasAllFeatures(requiredCoverFeaturesForPosition)) {
                    console.info("Slider brightness is supported");
                    sliderLoader.setSource("qrc:/components/TouchSliderPosition.qml", { "entityObj": entityObj });
                } else {
                    touchSlider.active = false;
                    console.info("Slider not supported");
                }
                break;
            }
            }
            break;
        }
        default: {
            touchSlider.active = false;
            console.info("Slider not supported");
            break;
        }
        }
    }

    onActiveChanged: {
        console.info("[Touch Slider] active changed:", active);
        if (!active) {
            sliderLoader.source = "";
            return;
        }
        startSetup();
    }

    onEntityObjChanged: {
        console.info("[Touch Slider] entityObj changed:", entityObj);
        startSetup();
    }

    onFeatureChanged: {
        console.info("[Touch Slider] feature changed:", feature);
        startSetup();
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

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    touchSlider.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    touchSlider.close();
                }
            }
        }
    }

    Loader {
        id: sliderLoader
        width: ui.width; height: 300
        x: 0
        y: ui.height - sliderLoader.item.height
        active: touchSlider.active


        Connections {
            target: sliderLoader.item
            ignoreUnknownSignals: true

            function onOpen() {
                touchSlider.open();
            }

            function onClose() {
                touchSlider.close();
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            touchSlider.close();
        }
    }
}
