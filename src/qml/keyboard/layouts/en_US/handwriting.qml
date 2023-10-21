// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.3
import QtQuick.VirtualKeyboard 2.3

KeyboardLayout {
    function createInputMethod() {
        return Qt.createQmlObject('import QtQuick 2.15; import QtQuick.VirtualKeyboard 2.3; HandwritingInputMethod {}', parent)
    }
    sharedLayouts: ['symbols']
    inputMode: InputEngine.Latin

    KeyboardRow {
        Layout.preferredHeight: 3
        KeyboardColumn {
            Layout.preferredWidth: bottomRow.width - hideKeyboardKey.width
            KeyboardRow {
                TraceInputKey {
                    objectName: "hwrInputArea"
                    patternRecognitionMode: InputEngine.HandwritingRecoginition
                }
            }
        }
        KeyboardColumn {
            Layout.preferredWidth: hideKeyboardKey.width
            KeyboardRow {
                BackspaceKey {}
            }
            KeyboardRow {
                EnterKey {}
            }
            KeyboardRow {
                ShiftKey { }
            }
        }
    }
    KeyboardRow {
        id: bottomRow
        Layout.preferredHeight: 1
        keyWeight: 154
        InputModeKey {
            weight: 217
        }
        ChangeLanguageKey {
            weight: 154
            customLayoutsOnly: true
        }
        HandwritingModeKey {
            weight: 154
        }
        SpaceKey {
            weight: 864
        }
        Key {
            key: Qt.Key_Apostrophe
            text: "'"
            alternativeKeys: "<>()#%&*/\\\"'=+-_"
        }
        Key {
            key: Qt.Key_Period
            text: "."
            alternativeKeys: ":;,.?!"
        }
        HideKeyboardKey {
            id: hideKeyboardKey
            weight: 204
        }
    }
}
