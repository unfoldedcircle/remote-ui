// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

FieldBase {
    id: root

    Text {
        width: parent.width
        text: root.value
        color: colors.textPrimary
        textFormat: Text.MarkdownText
        linkColor: colors.textPrimary
        wrapMode: Text.WordWrap
        font: fonts.prose()
        lineHeight: fonts.proseLineHeight
        visible: root.value !== ""
    }
}
