// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

Column {
    id: root

    property string labelId
    property var value
    property alias labelText: label.text
    property alias label: label

    Layout.fillWidth: true
    spacing: 10

    Text {
        id: label
        width: parent.width
        color: colors.offwhite
        textFormat: Text.RichText
        wrapMode: Text.WordWrap
        font: fonts.secondaryFont(30)
        visible: label.text !== ""
    }
}
