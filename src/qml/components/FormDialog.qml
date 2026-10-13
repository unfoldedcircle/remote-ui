// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import "qrc:/components" as Components

// The layout of a rename or password dialog (docs/design-system.md sections 5 and 7): the title bar with a close
// target, an optional description, the field(s) and the button pair, all inside the 20 px gutter. The fields are
// the children of the dialog; the buttons go to `buttons`, each `width: <dialog>.buttonWidth`, Cancel first.
// It only lays out: the host keeps showing and hiding, the keys and the focus chain.
Item {
    id: formDialog
    anchors.fill: parent

    property alias title: titleBar.text
    property alias description: descriptionText.text
    // called by the close target of the title bar, usually the Cancel action
    property var goBack
    // false hides the close target, e.g. when the form cannot be cancelled
    property bool closable: true

    default property alias fields: fieldColumn.data
    property alias buttons: buttonRow.data

    readonly property real buttonWidth: (buttonRow.width - buttonRow.spacing) / 2
    // tighter spacing while the on-screen keyboard is up, so the buttons stay above it
    readonly property bool compact: typeof keyboard !== "undefined" && keyboard.active

    Components.TitleBar {
        id: titleBar
        anchors.top: parent.top
        action: formDialog.closable ? "close" : ""
        goBack: formDialog.goBack
    }

    Text {
        id: descriptionText
        visible: text !== ""
        x: 20
        width: parent.width - 40
        anchors.top: titleBar.bottom
        wrapMode: Text.WordWrap
        color: colors.textPrimary
        font: fonts.prose()
        lineHeight: fonts.proseLineHeight
    }

    Column {
        id: fieldColumn
        x: 20
        width: parent.width - 40
        spacing: formDialog.compact ? 10 : 20
        anchors { top: descriptionText.visible ? descriptionText.bottom : titleBar.bottom; topMargin: 10 }
    }

    Row {
        id: buttonRow
        x: 20
        width: parent.width - 40
        spacing: 20
        anchors { top: fieldColumn.bottom; topMargin: formDialog.compact ? 20 : 40 }
    }
}
