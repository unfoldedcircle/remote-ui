// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.VirtualKeyboard 2.3


KeyboardLayout {
    inputMode: InputEngine.InputMode.Latin
    keyWeight: 160

    KeyboardRow {
//        HandwritingModeKey { weight: 154 }
//        ChangeLanguageKey { weight: 100 }
        SymbolModeKey { weight: 100 }
        BackspaceKey {}
    }

    KeyboardRow {
        Key {
            key: Qt.Key_1
            text: "1"
        }
        Key {
            key: Qt.Key_2
            text: "2"
        }
        Key {
            key: Qt.Key_3
            text: "3"
        }
        Key {
            key: Qt.Key_4
            text: "4"
        }
        Key {
            key: Qt.Key_5
            text: "5"
        }
        Key {
            key: Qt.Key_6
            text: "6"
        }
        Key {
            key: Qt.Key_7
            text: "7"
        }
        Key {
            key: Qt.Key_8
            text: "8"
        }
        Key {
            key: Qt.Key_9
            text: "9"
        }
        Key {
            key: Qt.Key_0
            text: "0"
        }
    }
    KeyboardRow {
        Key {
            key: Qt.Key_Q
            text: "q"
        }
        Key {
            key: Qt.Key_W
            text: "w"
        }
        Key {
            key: Qt.Key_E
            text: "e"
        }
        Key {
            key: Qt.Key_R
            text: "r"
        }
        Key {
            key: Qt.Key_T
            text: "t"
        }
        Key {
            key: Qt.Key_Y
            text: "y"
        }
        Key {
            key: Qt.Key_U
            text: "u"
        }
        Key {
            key: Qt.Key_I
            text: "i"
        }
        Key {
            key: Qt.Key_O
            text: "o"
        }
        Key {
            key: Qt.Key_P
            text: "p"
        }
    }
    KeyboardRow {
        Key {
            key: Qt.Key_A
            text: "a"
        }
        Key {
            key: Qt.Key_S
            text: "s"
        }
        Key {
            key: Qt.Key_D
            text: "d"
        }
        Key {
            key: Qt.Key_F
            text: "f"
        }
        Key {
            key: Qt.Key_G
            text: "g"
        }
        Key {
            key: Qt.Key_H
            text: "h"
        }
        Key {
            key: Qt.Key_J
            text: "j"
        }
        Key {
            key: Qt.Key_K
            text: "k"
        }
        Key {
            key: Qt.Key_L
            text: "l"
        }
    }
    KeyboardRow {
        Key {
            key: Qt.Key_Z
            text: "z"
        }
        Key {
            key: Qt.Key_X
            text: "x"
        }
        Key {
            key: Qt.Key_C
            text: "c"
        }
        Key {
            key: Qt.Key_V
            text: "v"
        }
        Key {
            key: Qt.Key_B
            text: "b"
        }
        Key {
            key: Qt.Key_N
            text: "n"
        }
        Key {
            key: Qt.Key_M
            text: "m"
        }
    }
    KeyboardRow {
        ShiftKey {
            weight: 160
        }
        Key {
            text: " "
            displayText: "Space"
            repeat: true
            showPreview: false
            key: Qt.Key_Space
            weight: 300
        }
        HideKeyboardKey {
            onClicked: root.keyboard.hide();
        }
    }
}
