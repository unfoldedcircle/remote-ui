// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
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
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 10
            Layout.rightMargin: 20
            Layout.fillWidth: true
            Layout.fillHeight: true
            //: Sensor entity: electrical current, e.g. "12 ampere"
            text: qsTr("Current")
            wrapMode: Text.WordWrap
            maximumLineCount: 1
            color: colors.light
            verticalAlignment: Text.AlignVCenter
            font: fonts.secondaryFont(24)
        }

        Text {
            id: valueText

            Layout.alignment: Qt.AlignVCenter
            Layout.rightMargin: 10
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: entityObj.value !== "" ? entityObj.value : qsTranslate("Abbreviation for not available", "N/A")
            elide: Text.ElideRight
            maximumLineCount: 1
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignRight
            font: fonts.primaryFont(160,  "Light")
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: 30
        }

        Text {
            Layout.columnSpan: 2
            Layout.topMargin: -20
            Layout.rightMargin: 20
            Layout.fillWidth: true
            text: entityObj.unit
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            elide: Text.ElideRight
            maximumLineCount: 2
            color: colors.offwhite
            horizontalAlignment: Text.AlignRight
            font: fonts.secondaryFont(40)
        }
    }
}
