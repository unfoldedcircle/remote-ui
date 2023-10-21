// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 ABOUT INFO COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - key
 - value
 -
**/

import QtQuick 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    width: parent.width

    property alias key: title.text
    property alias value: value.text
    property bool multiline: false
    property bool lineTop: false
    property bool lineBottom: true

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1

        color: colors.medium
        visible: lineTop
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 10
        Layout.bottomMargin: multiline ? 0 : 10

        Text {
            id: title

            Layout.fillWidth: true

            elide: Text.ElideRight
            maximumLineCount: 1
            color: colors.light
            font: fonts.secondaryFont(24)
            visible: text
        }

        Text {
            id: value

            Layout.fillWidth: true

            horizontalAlignment: Text.AlignRight
            elide: Text.ElideRight
            maximumLineCount: 1
            color: colors.offwhite
            font: fonts.primaryFont(24)
            visible: !multiline
        }
    }

    Text {
        Layout.fillWidth: true
        Layout.bottomMargin: 10

        text: value.text
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        color: colors.offwhite
        font: fonts.primaryFont(24)
        visible: multiline

    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1

        color: colors.medium
        visible: lineBottom
    }
}
