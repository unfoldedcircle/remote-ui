// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Entity.Cover 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: openCloseRoot
    spacing: 20

    Item {
        Layout.fillWidth: true
        implicitHeight: childrenRect.height
        visible: openCloseFeature.visible

        Text {
            //: State of the cover entity (eg. blinds, shades)
            text: {
                switch (entityObj.state) {
                case CoverStates.Unknown:
                    return qsTr("Unknown");
                case CoverStates.Open:
                    return qsTr("Open");
                case CoverStates.Closed:
                    return qsTr("Closed");
                default:
                    return qsTr("Unknown");
                }
            }

            color: colors.offwhite
            font: fonts.primaryFont(90)
        }
    }

    Components.Button {
        Layout.fillWidth: true
        Layout.fillHeight: true

        text: qsTr("Close")
        color: colors.medium
        trigger: function() {
            entityObj.close();
        }
        enabled: entityObj.state === CoverStates.Open || entityObj.state === CoverStates.Unknown
        opacity: enabled ? 1 : 0.5
    }

    Components.Button {
        Layout.fillWidth: true
        Layout.fillHeight: true

        text: qsTr("Open")
        color: colors.medium
        trigger: function() {
            entityObj.open();
        }
        enabled: entityObj.state === CoverStates.Closed || entityObj.state === CoverStates.Unknown
        opacity: enabled ? 1 : 0.5
    }
}
