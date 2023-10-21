// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 CLIMATE BUTTON COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - text
**/

import QtQuick 2.15

Item {
    anchors.fill: parent

    property alias info: infoText.text

    Text {
        id: infoText
        color: colors.light
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.centerIn: parent
        font: fonts.secondaryFont(24)
    }
}
