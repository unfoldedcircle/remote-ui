// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import "qrc:/components" as Components

// Long text to read (docs/design-system.md sections 4 and 7): release notes, legal texts, driver instructions.
// The prose role in the 20 px gutter, the scroll indicator, and scrollStep() for the d-pad, which moves by half
// the viewport (section 6). The host keeps its keys and calls scrollStep(1) / scrollStep(-1).
Flickable {
    id: prose

    property alias text: proseText.text
    property alias textFormat: proseText.textFormat
    property alias baseUrl: proseText.baseUrl
    property alias textItem: proseText

    signal linkActivated(string link)

    function scrollStep(direction) {
        const maxContentY = Math.max(0, contentHeight - height);
        contentY = Math.max(0, Math.min(maxContentY, contentY + direction * Math.round(height / 2)));
    }

    contentWidth: width
    contentHeight: proseText.implicitHeight + 20
    clip: true
    flickableDirection: Flickable.VerticalFlick
    boundsBehavior: Flickable.StopAtBounds

    Behavior on contentY {
        NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
    }

    Text {
        id: proseText
        x: 20
        width: prose.width - 40
        wrapMode: Text.WordWrap
        color: colors.textPrimary
        font: fonts.prose()
        lineHeight: fonts.proseLineHeight
        onLinkActivated: prose.linkActivated(link)
    }

    Components.ScrollIndicator {
        parent: prose
        parentObj: prose
    }
}
