// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
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

    Item {
        width: parent.width
        height: parent.height - title.height
        anchors { top: title.bottom }

        Text {
            id: valueText
            text: entityObj.value !== "" ? entityObj.value : "N/A"
            width: parent.width - 20
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.offwhite
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -80
            horizontalAlignment: Text.AlignHCenter
            font: fonts.primaryFont(180,  "Light")
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 40
        }

        Text {
            text: entityObj.unit
            width: parent.width - 20
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.offwhite
            horizontalAlignment: Text.AlignHCenter
            font: fonts.secondaryFont(40)
            anchors { horizontalCenter: parent.horizontalCenter; top: valueText.bottom; topMargin: 10 }
        }
    }
}
