// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import Haptic 1.0

MouseArea {
    // Opt-in for rows that are part of a keypad focus chain: DPAD_MIDDLE (Return) while focused
    // triggers the same clicked handler as a tap. Handlers of such rows must not read the mouse
    // argument. Draw the focus outline in the row itself from `activeFocus && ui.keyNavigationActive`.
    property bool keypadActivatable: false

    onPressed: Haptic.play(Haptic.Click)

    Keys.onReturnPressed: {
        if (!keypadActivatable) {
            event.accepted = false;
            return;
        }

        Haptic.play(Haptic.Click);
        clicked(null);
        event.accepted = true;
    }
}
