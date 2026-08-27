// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0

import "qrc:/keypad" as Keypad
import "qrc:/components" as Components

Item {
    id: keyPadContiner

    signal pinEntered(string pin)

    property string pinToCheck
    property color pinDotColor: colors.offwhite

    onPinToCheckChanged: {
        if (pinToCheck.length == 4) {
            pinEntered(pinToCheck);
        }
    }

    function showError(message) {
        keyPadContiner.pinDotColor = colors.red;
        ui.setTimeOut(2000, function() {
            keyPadContiner.pinDotColor = colors.offwhite;
            keyPadContiner.reset();
        })
    }

    function reset() {
        keyPadContiner.pinToCheck = "";
    }

    /** KEYBOARD NAVIGATION **/
    // 3 x 4 grid, laid out in the same order as the Flow below. Index 9 is the empty filler cell
    // and is skipped while navigating.
    readonly property int columns: 3
    readonly property int emptyIndex: 9
    readonly property int noSelection: -1
    // No key is outlined until the d-pad is used: a touch user never sees the selection, and a
    // d-pad user gets it on the first key press.
    property int selectedIndex: noSelection

    function clearSelection() {
        keyPadContiner.selectedIndex = keyPadContiner.noSelection;
    }

    function moveSelection(deltaColumn, deltaRow) {
        // first d-pad press: show the selection on the first key instead of moving it
        if (keyPadContiner.selectedIndex === keyPadContiner.noSelection) {
            keyPadContiner.selectedIndex = 0;
            return;
        }

        const columnCount = keyPadContiner.columns;
        const rowCount = Math.ceil(keyModel.length / columnCount);

        let column = keyPadContiner.selectedIndex % columnCount;
        let row = Math.floor(keyPadContiner.selectedIndex / columnCount);

        column = Math.max(0, Math.min(columnCount - 1, column + deltaColumn));
        row = Math.max(0, Math.min(rowCount - 1, row + deltaRow));

        let next = row * columnCount + column;

        // the filler cell holds no key: keep going in the same direction, and stay put if that
        // would leave the grid
        if (next === keyPadContiner.emptyIndex) {
            const shifted = next + (deltaColumn !== 0 ? deltaColumn : 0);
            if (deltaColumn !== 0 && shifted >= 0 && shifted < keyModel.length) {
                next = shifted;
            } else {
                return;
            }
        }

        keyPadContiner.selectedIndex = next;
    }

    function applyKey(index) {
        const key = keyModel[index];
        if (!key) {
            return;
        }

        if (key.backspace) {
            keyPadContiner.pinToCheck = keyPadContiner.pinToCheck.slice(0, -1);
        } else if (key.value !== "") {
            keyPadContiner.pinToCheck += key.value;
        }
    }

    function activateSelection() {
        // nothing is outlined yet: the first middle press only shows the selection, so the user
        // sees which key is about to be entered
        if (keyPadContiner.selectedIndex === keyPadContiner.noSelection) {
            keyPadContiner.selectedIndex = 0;
            return;
        }

        // the touch path gets its haptic from the key's own MouseArea
        Haptic.play(Haptic.Click);
        keyPadContiner.applyKey(keyPadContiner.selectedIndex);
    }

    readonly property var keyModel: [
        { value: "1", backspace: false },
        { value: "2", backspace: false },
        { value: "3", backspace: false },
        { value: "4", backspace: false },
        { value: "5", backspace: false },
        { value: "6", backspace: false },
        { value: "7", backspace: false },
        { value: "8", backspace: false },
        { value: "9", backspace: false },
        { value: "",  backspace: false },
        { value: "0", backspace: false },
        { value: "",  backspace: true }
    ]

    // pin dots
    Flow {
        id: pinDots
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter }
        height: 50
        topPadding: 20
        spacing: 20

        Rectangle {
            width: 12; height: 12
            radius: 6
            color: keyPadContiner.pinDotColor
            opacity: pinToCheck.length >= 1 ? 1 : 0.6

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }

        Rectangle {
            width: 12; height: 12
            radius: 6
            color: keyPadContiner.pinDotColor
            opacity: pinToCheck.length >= 2 ? 1 : 0.6

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }

        Rectangle {
            width: 12; height: 12
            radius: 6
            color: keyPadContiner.pinDotColor
            opacity: pinToCheck.length >= 3 ? 1 : 0.6

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }

        Rectangle {
            width: 12; height: 12
            radius: 6
            color: keyPadContiner.pinDotColor
            opacity: pinToCheck.length >= 4 ? 1 : 0.6

            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }
    }


    // keypad
    Flow {
        id: keyPad
        width: parent.width
        anchors.top: pinDots.bottom

        Repeater {
            model: keyPadContiner.keyModel

            delegate: Item {
                width: keyPad.width/3; height: 140

                Keypad.Key {
                    anchors.fill: parent
                    value: modelData.value
                    highlight: keyPadContiner.selectedIndex === index
                    // the filler cell in the bottom left row holds no key
                    visible: modelData.value !== "" || modelData.backspace

                    mouseArea.onClicked: {
                        // touch input works without the outline
                        keyPadContiner.clearSelection();
                        keyPadContiner.applyKey(index);
                    }

                    Components.Icon {
                        color: colors.offwhite
                        icon: "uc:arrow-left"
                        anchors.centerIn: parent
                        size: 80
                        visible: modelData.backspace
                    }
                }
            }
        }
    }
}
