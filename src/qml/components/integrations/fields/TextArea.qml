// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

FieldBase {
    id: root

    function showError(error) {
        inputField.showError(error);
    }

    Components.InputField {
        id: inputField
        inputField.placeholderText: root.value
        onInputValueChanged: root.value = inputField.inputValue
        label: root.labelText
    }
}
