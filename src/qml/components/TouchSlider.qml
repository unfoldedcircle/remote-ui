import QtQuick 2.15
import QtQuick.Controls 2.15

import Entity.Controller 1.0
import Entity.Light 1.0
import Entity.Cover 1.0
import Entity.MediaPlayer 1.0

import Config 1.0
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
    // guards against repeatedly fetching an entity that has no features yet
    property bool _featuresRequested: false

    property var requiredMediaPlayerFeaturesForVolume: [MediaPlayerFeatures.Volume]
    property var requiredMediaPlayerFeaturesForSeek: [MediaPlayerFeatures.Seek, MediaPlayerFeatures.Media_duration, MediaPlayerFeatures.Media_position]
    property var requiredLightFeaturesForBrightness: [LightFeatures.Dim]
    property var requiredCoverFeaturesForPosition: [CoverFeatures.Position]

    // An activity slider config without an explicit feature reports "default".
    // That is not a feature name, so it has to be resolved to the slider feature
    // that matches the target entity's type, e.g. a light dims instead of
    // changing volume.
    function defaultFeatureForType(type) {
        switch (type) {
        case EntityTypes.Media_player:
            return "volume";
        case EntityTypes.Light:
            return "dim";
        case EntityTypes.Cover:
            return "position";
        default:
            return "";
        }
    }

    function resolvedFeature() {
        if (touchSlider.feature !== "" && touchSlider.feature !== "default") {
            return touchSlider.feature;
        }

        return entityObj ? defaultFeatureForType(entityObj.type) : "";
    }

    function setSliderSource(source) {
        if (sliderLoader.source === source && sliderLoader.item && sliderLoader.item.entityObj === entityObj) {
            return;
        }

        sliderLoader.setSource(source, { "entityObj": entityObj });
    }

    function refreshSetup() {
        if (!touchSlider.active || !entityObj) {
            return;
        }

        startSetup();
    }

    function ensureEntityFeatures() {
        // Configured entities are bulk-loaded without their feature list; only
        // get_entity delivers it. Fetch it once (like the entity control screen
        // does) so the capability checks in startSetup() pass for entities that
        // were never opened on their own, e.g. a light mapped to an activity's
        // touch slider. featuresChanged then re-runs setup.
        if (touchSlider._featuresRequested || !entityObj || !entityObj.id) {
            return;
        }

        touchSlider._featuresRequested = true;
        EntityController.refreshEntity(entityObj.id);
    }

    function startSetup() {
        if (!Config.touchSliderEnabled) {
            sliderLoader.source = "";
            return;
        }

        if (!entityObj) {
            sliderLoader.source = "";
            return;
        }

        if (HwInfo.modelNumber != "UCR3" && HwInfo.modelNumber != "DEV") {
            touchSlider.active = false;
            sliderLoader.source = "";
            return;
        }

        // Features may not be loaded yet (bulk-loaded entity). Fetch them and
        // wait for featuresChanged instead of deactivating the slider, so the
        // capability checks below don't fail on missing feature data.
        if (!entityObj.featuresLoaded()) {
            ensureEntityFeatures();
            sliderLoader.source = "";
            return;
        }

        const feature = resolvedFeature();

        switch (entityObj.type) {
        case EntityTypes.Media_player: {
            switch (feature) {
            case "volume": {
                if (entityObj.hasAllFeatures(requiredMediaPlayerFeaturesForVolume)) {
                    setSliderSource("qrc:/components/TouchSliderVolume.qml");
                } else {
                    touchSlider.active = false;
                }
                break;
            }
            case "seek": {
                if (entityObj.hasAllFeatures(requiredMediaPlayerFeaturesForSeek)) {
                    setSliderSource("qrc:/components/TouchSliderSeek.qml");
                } else {
                    touchSlider.active = false;
                }
                break;
            }
            default: {
                touchSlider.active = false;
                break;
            }
            }
            break;
        }
        case EntityTypes.Light: {
            switch (feature) {
            case "dim":
            case "brightness": {
                if (entityObj.hasAllFeatures(requiredLightFeaturesForBrightness)) {
                    setSliderSource("qrc:/components/TouchSliderBrightness.qml");
                } else {
                    touchSlider.active = false;
                }
                break;
            }
            default: {
                touchSlider.active = false;
                break;
            }
            }
            break;
        }
        case EntityTypes.Cover: {
            switch (feature) {
            case "position": {
                if (entityObj.hasAllFeatures(requiredCoverFeaturesForPosition)) {
                    setSliderSource("qrc:/components/TouchSliderPosition.qml");
                } else {
                    touchSlider.active = false;
                }
                break;
            }
            default: {
                touchSlider.active = false;
                break;
            }
            }
            break;
        }
        default: {
            touchSlider.active = false;
            break;
        }
        }
    }

    onActiveChanged: {
        if (!active) {
            sliderLoader.source = "";
            return;
        }
        // allow a fresh features fetch each time the slider becomes active
        touchSlider._featuresRequested = false;
        refreshSetup();
    }

    onEntityObjChanged: {
        touchSlider._featuresRequested = false;
        refreshSetup();
    }

    onFeatureChanged: {
        refreshSetup();
    }

    Connections {
        target: Config
        ignoreUnknownSignals: true

        function onTouchSliderEnabledChanged() {
            if (Config.touchSliderEnabled) {
                refreshSetup();
            } else {
                sliderLoader.source = "";
            }
        }
    }

    Connections {
        target: touchSlider.entityObj
        ignoreUnknownSignals: true

        // re-run setup once the entity's features arrive (see ensureEntityFeatures)
        function onFeaturesChanged() {
            refreshSetup();
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
        y: ui.height - (sliderLoader.item ? sliderLoader.item.height : sliderLoader.height)
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
