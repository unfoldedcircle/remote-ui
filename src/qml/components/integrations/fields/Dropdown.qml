// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

FieldBase {
    id: root

    property var model

    ComboBox {
        id: dropDown

        width: parent.width

        model: root.model
        textRole: "label"
        valueRole: "id"

        onCurrentValueChanged: root.value = dropDown.currentValue

        delegate: ItemDelegate {
            id: itemDelegate
            width: dropDown.width
            contentItem: Text {
                text: modelData.label
                color: colors.offwhite
                font: fonts.secondaryFont(30)
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            highlighted: dropDown.highlightedIndex === index

            background: Rectangle {
                color: itemDelegate.hovered ? colors.light : colors.medium
                radius: ui.cornerRadiusLarge
            }
        }

        background: Rectangle {
            width: parent.width; height: 80
            color: colors.dark
            border { color: colors.medium; width: 0 }
            radius: ui.cornerRadiusLarge
        }

        contentItem: Text {
            text: dropDown.displayText
            font: fonts.secondaryFont(30)
            color: dropDown.pressed ? colors.primaryButton : colors.offwhite
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            topPadding: 15
            leftPadding: 20
            rightPadding: dropDown.indicator.width + dropDown.spacing
        }

        popup: Popup {
            y: dropDown.height - 1
            width: dropDown.width
            implicitHeight: contentItem.implicitHeight
            padding: 1

            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: dropDown.popup.visible ? dropDown.delegateModel : null
                currentIndex: dropDown.highlightedIndex

                ScrollIndicator.vertical: ScrollIndicator {}
            }

            background: Rectangle {
                color: colors.medium
                radius: ui.cornerRadiusLarge
            }
        }
    }
}
