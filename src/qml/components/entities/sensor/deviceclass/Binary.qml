// Copyright (c) 2025 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.0

import "qrc:/components" as Components
import "qrc:/components/entities" as EntityComponents

EntityComponents.BaseDetail {
    EntityComponents.BaseTitle {
        id: title
        icon: entityObj.icon
        title: entityObj.name
    }

    LinearGradient {
        anchors {
            top: title.bottom
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        start: Qt.point(0, 0)
        end: Qt.point(0, parent.height)
        gradient: Gradient {
            GradientStop { position: 0.0; color: colors.transparent }
            GradientStop { position: 1.0; color: colors.medium }
        }
    }

    GridLayout {
        columns: 2
        rows: 2
        anchors {
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        Text {
            id: valueText

            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: 10
            Layout.fillWidth: true
            Layout.fillHeight: true
            // binary sensor value is device class specific and translated in sensor.cpp
            text: entityObj.value
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font: fonts.primaryFont(160,  "Light")
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 30
        }
    }
}
