// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

// The look of every bottom sheet (docs/design-system.md section 5, Q-4): black, a 22 px top radius and a 2 px
// divider edge that separates it from the dimmed page. Use it as the background of the sheet; the host keeps the
// opening, closing and keys, and puts a Components.Dim of its own behind the sheet.
Item {
    id: sheet
    clip: true

    Rectangle {
        // the rounded rectangle reaches below the sheet, so only the top corners are rounded and the edge
        // runs along the top and the sides
        anchors { fill: parent; bottomMargin: -radius }
        radius: ui.cornerRadiusLarge
        color: colors.bg
        border { width: 2; color: colors.divider }
    }
}
