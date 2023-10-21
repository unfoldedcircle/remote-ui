// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SENSOR BUTTON COMPONENT

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - value
**/

import QtQuick 2.15

Item {
    id: sensorButton
    anchors.fill: parent

    property string value

    Text {
        id: infoText
        text: sensorButton.value !== "" ? sensorButton.value : "N/A"
        maximumLineCount: 3
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        elide: Text.ElideRight
        color: colors.light
        verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
        anchors.fill: parent
        padding: 5
        font: fonts.secondaryFont(24)
    }
}
