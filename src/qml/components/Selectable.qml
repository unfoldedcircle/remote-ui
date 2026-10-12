// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

// The d-pad selection of its parent (docs/design-system.md section 6, ADR 0020). It only renders: it handles no
// keys and moves no focus, so the screen keeps its navigation idiom. Declare it as the first child of the selected
// item, so the fill lies under the content.
//
// - style "ring": a 3 px ring and no fill, for settings, set-up flows and buttons everywhere
// - style "fill": the selection fill and no ring, for the main UI (tiles, popup menus, page selector, profile
//   switcher), the menus of the profile page, Settings and About and the picker lists; everything drawn on
//   the fill uses colors.textPrimary
Rectangle {
    // true while the parent is the selected element; drawn only while the keypad is active
    property bool selected: false
    property string style: "ring"
    // drawn after a touch as well: the page selector's current page and a picker list's current value
    // (docs/design-system.md section 6)
    property bool shownAfterTouch: false

    readonly property bool shown: selected && (ui.keyNavigationActive || shownAfterTouch)

    anchors.fill: parent
    radius: ui.cornerRadiusSmall
    color: shown && style === "fill" ? colors.surfaceSelected : colors.transparent
    border {
        width: style === "ring" ? 3 : 0
        color: shown && style === "ring" ? colors.focusRing : colors.transparent
    }
}
