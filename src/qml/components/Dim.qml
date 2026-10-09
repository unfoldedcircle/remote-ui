// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

// The dim behind a bottom sheet or popup card (docs/design-system.md section 5): black at 0.85. It only renders;
// the host decides whether a tap on it closes the sheet.
Rectangle {
    color: colors.bg
    opacity: 0.85
}
