// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

FieldBase {
    id: root

    Text {
        width: parent.width
        text: root.value
        color: colors.light
        textFormat: Text.MarkdownText
        wrapMode: Text.WordWrap
        font: fonts.secondaryFont(24)
        visible: root.value !== ""
    }
}
