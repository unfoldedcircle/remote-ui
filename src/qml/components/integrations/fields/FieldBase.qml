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

    /** KEYBOARD NAVIGATION **/
    // The control of the field that takes the focus (null for a field without one, e.g. a label).
    // The form assigns the neighbours; each field binds its control's KeyNavigation to them, as an
    // attached property can only be set on the item that declares it.
    property Item focusItem: null
    property Item navUp: null
    property Item navDown: null

    // Return on a text field: move on to the next control
    function advance() {
        if (root.navDown) {
            root.navDown.forceActiveFocus();
        }
    }

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
