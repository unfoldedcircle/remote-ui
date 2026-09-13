// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

FieldBase {
    id: root

    property var model

    /** KEYBOARD NAVIGATION **/
    // The ComboBox's own popup takes no input and would swallow the chain keys. Both a tap and
    // DPAD_MIDDLE open a PopupList instead, which owns the input while it is shown.
    focusItem: dropDownField

    function openList() {
        if (listLoader.active) {
            return;
        }

        listLoader.active = true;
    }

    Component.onCompleted: {
        // PopupList roles are fixed on the first append: set every optional role on every row
        for (let i = 0; i < root.model.length; i++) {
            optionsModel.append({ name: String(root.model[i].label), value: root.model[i].id,
                                  secondary: "", rightText: "", searchKey: "" });
        }
    }

    ListModel {
        id: optionsModel
    }

    Item {
        id: dropDownField
        width: parent.width
        height: 80

        KeyNavigation.up: root.navUp
        KeyNavigation.down: root.navDown

        Keys.onReturnPressed: {
            root.openList();
            event.accepted = true;
        }

        onActiveFocusChanged: {
            if (activeFocus && keyboard.active) {
                keyboard.hide();
            }
        }

        ComboBox {
            id: dropDown

            width: parent.width
            // never focused and never pressed: the wrapper handles the taps and the keys
            focusPolicy: Qt.NoFocus

            model: root.model
            textRole: "label"
            valueRole: "id"

            onCurrentValueChanged: root.value = dropDown.currentValue

            background: Rectangle {
                width: parent.width; height: 80
                color: colors.dark
                border { color: colors.medium; width: 0 }
                radius: ui.cornerRadiusLarge
            }

            contentItem: Text {
                text: dropDown.displayText
                font: fonts.secondaryFont(30)
                color: colors.offwhite
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                topPadding: 15
                leftPadding: 20
                rightPadding: dropDown.indicator.width + dropDown.spacing
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: ui.cornerRadiusLarge
            color: colors.transparent
            border {
                width: 2
                color: dropDownField.activeFocus && ui.keyNavigationActive ? colors.highlight : colors.transparent
            }
        }

        Components.HapticMouseArea {
            anchors.fill: parent
            onClicked: root.openList()
        }
    }

    Loader {
        id: listLoader
        parent: Overlay.overlay
        anchors.fill: parent
        active: false

        sourceComponent: Components.PopupList {
            title: root.labelText
            listModel: optionsModel
            showSearch: optionsModel.count > 8
            initialSelected: Math.max(0, dropDown.currentIndex)

            onItemSelected: {
                dropDown.currentIndex = dropDown.indexOfValue(value);
            }

            onDone: {
                listLoader.active = false;
            }
        }
    }
}
