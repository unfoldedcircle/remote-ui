// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import Haptic 1.0
import Entity.Light 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    id: lightBase

    Component.onCompleted: {
        if (!entityObj.hasFeature(LightFeatures.Dim)) {
            lightFeaturesModel.append({feature: "onoff"})
        }

        if (entityObj.hasFeature(LightFeatures.Dim)) {
            lightFeaturesModel.append({feature: "brightness"})
        }

        if (entityObj.hasFeature(LightFeatures.Color)) {
            lightFeaturesModel.append({feature: "color"})
        }
    }

    overrideConfig: {
        "DPAD_LEFT": {
            "pressed": function() {
                lightFeatures.decrementCurrentIndex();
            }
        },
        "DPAD_RIGHT": {
            "pressed": function() {
                lightFeatures.incrementCurrentIndex();
            }
        },
        "DPAD_MIDDLE": {
            "pressed": function() {
                entityObj.toggle();
            }
        },
        "POWER": {
            "pressed": function() {
                entityObj.toggle();
            }
        },
    }

    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        title: entityObj.name
    }

    ListModel {
        id: lightFeaturesModel
    }

    ListView {
        id: lightFeatures
        width: parent.width
        height: parent.height - title.height
        anchors { top: title.bottom }

        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 200

        maximumFlickVelocity: 6000
        flickDeceleration: 1000

        model: lightFeaturesModel
                delegate: lightFeaturesDelegate
        clip: true
    }

    PageIndicator {
        currentIndex: lightFeatures.currentIndex
        count: lightFeatures.count
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        visible: lightFeatures.count > 1
        padding: 0

        delegate: Component {
            Rectangle {
                width: 12; height: 12
                radius: 6
                color: colors.offwhite
                opacity: index == lightFeatures.currentIndex ? 1 : 0.6
            }
        }
    }

    Component {
        id: lightFeaturesDelegate

        Loader {
            id: lightFeaturesDelegateLoader
            active: true
            asynchronous: true
            width: ListView.view.width
            height: ListView.view.height

            Component.onCompleted: {
                if (feature == "onoff") {
                    lightFeaturesDelegateLoader.setSource("qrc:/components/entities/light/OnOff.qml", {entityObj: entityObj});
                } else if (feature == "brightness") {
                    lightFeaturesDelegateLoader.setSource("qrc:/components/entities/light/Brightness.qml", {entityObj: entityObj});
                } else if (feature == "color") {
                    lightFeaturesDelegateLoader.setSource("qrc:/components/entities/light/Color.qml", {entityObj: entityObj});
                }
            }
        }
    }
}
