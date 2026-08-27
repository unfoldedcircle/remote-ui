// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 HELP OVERLAY TIP PAGE

 One page of the help overlay: a title, an optional text below it, and whatever markers the page
 adds as children to point at the element of the UI it describes. Fills the whole screen, so the
 page below stays covered; the navigation row of the overlay is drawn on top of it.
**/

import QtQuick 2.15

Item {
    id: tip

    property alias title: titleText.text
    property alias text: bodyText.text
    property alias titleAlignment: titleText.horizontalAlignment
    property int titleRightMargin: 20
    property int topMargin: 20

    Text {
        id: titleText
        anchors { top: parent.top; topMargin: tip.topMargin; left: parent.left; leftMargin: 20; right: parent.right; rightMargin: tip.titleRightMargin }
        wrapMode: Text.WordWrap
        color: colors.offwhite
        font: fonts.primaryFont(30)
    }

    Text {
        id: bodyText
        anchors { top: titleText.bottom; topMargin: 20; left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20 }
        wrapMode: Text.WordWrap
        color: colors.light
        font: fonts.secondaryFont(24)
        visible: text !== ""
    }
}
