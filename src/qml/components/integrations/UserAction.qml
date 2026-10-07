// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

Item {
    id: root

    property alias title: title.text
    property alias message1: message1.text
    property string image
    property alias message2: message2.text

    /** KEYBOARD NAVIGATION **/
    // the page has no control: a text taller than the page takes the focus, DPAD_DOWN / UP scroll it,
    // and DOWN at its end moves on to the control the hosting form passes as navExit (its Next button).
    // A text that fits is not a stop: the form starts on Next.
    property alias flickable: contentFlickable
    readonly property bool scrollable: contentFlickable.contentHeight > contentFlickable.height
    readonly property Item firstFocusItem: scrollable ? contentFlickable : null
    readonly property Item lastFocusItem: scrollable ? contentFlickable : null
    property Item navExit: null

    Text {
        id: title

        width: parent.width - 40
        color: colors.textPrimary
        maximumLineCount: 2
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        font: fonts.title()
        anchors { top: parent.top; topMargin: 20; horizontalCenter: parent.horizontalCenter }
    }

    // the text holds the focus while it is scrolled with the keypad: it shows the ring like any control
    Components.Selectable {
        anchors.fill: contentFlickable
        anchors.margins: -8
        selected: contentFlickable.activeFocus
    }

    Flickable {
        id: contentFlickable

        width: parent.width - 40
        clip: true
        contentWidth: content.width; contentHeight: content.height
        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        anchors { top: title.bottom; topMargin: 20; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }

        KeyNavigation.down: root.navExit

        Keys.onDownPressed: {
            const maxContentY = Math.max(0, contentFlickable.contentHeight - contentFlickable.height);
            if (contentFlickable.contentY < maxContentY) {
                // half the viewport per press (docs/design-system.md section 6)
                contentFlickable.contentY = Math.min(contentFlickable.contentY + Math.round(contentFlickable.height / 2), maxContentY);
                event.accepted = true;
            } else {
                event.accepted = false;
            }
        }

        Keys.onUpPressed: {
            if (contentFlickable.contentY > 0) {
                contentFlickable.contentY = Math.max(0, contentFlickable.contentY - Math.round(contentFlickable.height / 2));
                event.accepted = true;
            } else {
                event.accepted = false;
            }
        }

        Components.ScrollIndicator {
            parent: contentFlickable
            parentObj: contentFlickable
        }

        ColumnLayout {
            id: content
            spacing: 20
            width: parent.width
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                id: message1

                Layout.fillWidth: true

                text: root.value
                color: colors.textPrimary
                textFormat: Text.MarkdownText
                linkColor: colors.textPrimary
                wrapMode: Text.WordWrap
                font: fonts.prose()
                lineHeight: fonts.proseLineHeight
            }

            Image {
                Layout.fillWidth: true

                source: {
                    if (image) {
                        return "data:image/png;base64," + image;
                    } else {
                        return "";
                    }
                }

                fillMode: Image.PreserveAspectFit
                asynchronous: true
                cache: false
            }

            Text {
                id: message2

                Layout.fillWidth: true

                text: root.value
                color: colors.textPrimary
                textFormat: Text.MarkdownText
                linkColor: colors.textPrimary
                wrapMode: Text.WordWrap
                font: fonts.prose()
                lineHeight: fonts.proseLineHeight
            }
        }
    }
}
