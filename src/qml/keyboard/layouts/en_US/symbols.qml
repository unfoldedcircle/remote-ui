// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.0
import QtQuick.VirtualKeyboard 2.3

KeyboardLayoutLoader {
    property bool secondPage
    onVisibleChanged: if (!visible) secondPage = false
    sourceComponent: secondPage ? page2 : page1
    Component {
        id: page1
        KeyboardLayout {
            keyWeight: 160
            KeyboardRow {
                keyWeight: 154
                SymbolModeKey {
                    weight: 217
                    displayText: "ABC"
                }
                Key {
                    key: Qt.Key_Period
                    text: "."
                }
                Key {
                    key: Qt.Key_Comma
                    text: ","
                }
                Key {
                    weight: 100
                    displayText: "1/2"
                    functionKey: true
                    onClicked: secondPage = !secondPage
                }
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
            }
            KeyboardRow {
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
                    key: Qt.Key_At
                    text: "@"
                }
                Key {
                    key: Qt.Key_NumberSign
                    text: "#"
                }
                Key {
                    key:  Qt.Key_Percent
                    text: "%"
                }
                Key {
                    key: Qt.Key_Ampersand
                    text: "&"
                }
                Key {
                    key: Qt.Key_Asterisk
                    text: "*"
                }
                Key {
                    key: Qt.Key_Minus
                    text: "-"
                }
                Key {
                    key: Qt.Key_Plus
                    text: "+"
                }
                Key {
                    key: Qt.Key_ParenLeft
                    text: "("
                }
                Key {
                    key: Qt.Key_ParenRight
                    text: ")"
                }
            }
            KeyboardRow {
                keyWeight: 156
                Key {
                    key: Qt.Key_Exclam
                    text: "!"
                }
                Key {
                    key:  Qt.Key_QuoteDbl
                    text: '"'
                }
                Key {
                    key: Qt.Key_Less
                    text: "<"
                }
                Key {
                    key: Qt.Key_Greater
                    text: ">"
                }
                Key {
                    key: Qt.Key_Apostrophe
                    text: "'"
                }
                Key {
                    key: Qt.Key_Colon
                    text: ":"
                }
                Key {
                    key: Qt.Key_Semicolon
                    text: ";"
                }
                Key {
                    key: Qt.Key_Slash
                    text: "/"
                }
                Key {
                    key: Qt.Key_Question
                    text: "?"
                }
            }
        }
    }
    Component {
        id: page2
        KeyboardLayout {
            keyWeight: 160
            KeyboardRow {
                SymbolModeKey {
                    weight: 217
                    displayText: "ABC"
                }
                Key {
                    key: 0x2026
                    text: "\u2026"
                }
                Key {
                    weight: 100
                    displayText: "2/2"
                    functionKey: true
                    onClicked: secondPage = !secondPage
                }
                BackspaceKey {}
            }
            KeyboardRow {
                Key {
                    key: Qt.Key_AsciiTilde
                    text: "~"
                }
                Key {
                    key: Qt.Key_Agrave
                    text: "`"
                }
                Key {
                    key: Qt.Key_Bar
                    text: "|"
                }
                Key {
                    key: 0x7B
                    text: "·"
                }
                Key {
                    key: 0x221A
                    text: "√"
                }
                Key {
                    key: Qt.Key_division
                    text: "÷"
                }
                Key {
                    key: Qt.Key_multiply
                    text: "×"
                }
            }
            KeyboardRow {
                Key {
                    key: Qt.Key_onehalf
                    text: "½"
                    alternativeKeys: "¼⅓½¾⅞"
                }
                Key {
                    key: Qt.Key_BraceLeft
                    text: "{"
                }
                Key {
                    key: Qt.Key_BraceRight
                    text: "}"
                }
                Key {
                    key: Qt.Key_Dollar
                    text: "$"
                }
                Key {
                    key: 0x20AC
                    text: "€"
                }
                Key {
                    key: 0xC2
                    text: "£"
                }
                Key {
                    key: 0xA2
                    text: "¢"
                }
            }
            KeyboardRow {
                Key {
                    key: 0xA5
                    text: "¥"
                }
                Key {
                    key: Qt.Key_Equal
                    text: "="
                }
                Key {
                    key: Qt.Key_section
                    text: "§"
                }
                Key {
                    key: Qt.Key_BracketLeft
                    text: "["
                }
                Key {
                    key: Qt.Key_BracketRight
                    text: "]"
                }
                Key {
                    key: Qt.Key_Underscore
                    text: "_"
                }
                Key {
                    key: 0x2122
                    text: '™'
                }

            }
            KeyboardRow {
                Key {
                    key: 0x00AE
                    text: '®'
                }
                Key {
                    key: Qt.Key_guillemotleft
                    text: '«'
                }
                Key {
                    key: Qt.Key_guillemotright
                    text: '»'
                }
                Key {
                    key: 0x201C
                    text: '“'
                }
                Key {
                    key: 0x201D
                    text: '”'
                }
                Key {
                    key: Qt.Key_Backslash
                    text: "\\"
                }
                Key {
                    key: Qt.Key_AsciiCircum
                    text: "^"
                }
            }
        }
    }
}
