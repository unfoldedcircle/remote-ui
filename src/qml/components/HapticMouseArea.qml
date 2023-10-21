// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import Haptic 1.0

MouseArea {
    onPressed: Haptic.play(Haptic.Click)
}
