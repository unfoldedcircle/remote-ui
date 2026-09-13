// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    property alias title: title.text
    property alias message1: message1.text
    property string image
    property alias message2: message2.text

    /** KEYBOARD NAVIGATION **/
    // the page has no control: the text takes the focus, DPAD_DOWN / UP scroll it, and DOWN at
    // its end moves on to the control the hosting form passes as navExit (its Next button)
    property alias flickable: contentFlickable
    readonly property Item firstFocusItem: contentFlickable
    readonly property Item lastFocusItem: contentFlickable
    property Item navExit: null

    Text {
        id: title

        width: parent.width - 40
        color: colors.offwhite
        maximumLineCount: 2
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
        font: fonts.primaryFont(30)
        anchors { top: parent.top; topMargin: 20; horizontalCenter: parent.horizontalCenter }
    }

    Flickable {
        id: contentFlickable

        width: parent.width - 40
        clip: true
        contentWidth: content.width; contentHeight: content.height
        maximumFlickVelocity: 6000
        flickDeceleration: 1000
        anchors { top: title.bottom; topMargin: 40; bottom: parent.bottom; horizontalCenter: parent.horizontalCenter }

        KeyNavigation.down: root.navExit

        Keys.onDownPressed: {
            const maxContentY = Math.max(0, contentFlickable.contentHeight - contentFlickable.height);
            if (contentFlickable.contentY < maxContentY) {
                contentFlickable.contentY = Math.min(contentFlickable.contentY + 200, maxContentY);
                event.accepted = true;
            } else {
                event.accepted = false;
            }
        }

        Keys.onUpPressed: {
            if (contentFlickable.contentY > 0) {
                contentFlickable.contentY = Math.max(0, contentFlickable.contentY - 200);
                event.accepted = true;
            } else {
                event.accepted = false;
            }
        }

        ScrollBar.vertical: ScrollBar {
            opacity: 0.5
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
                color: colors.light
                textFormat: Text.MarkdownText
                wrapMode: Text.WordWrap
                font: fonts.secondaryFont(24)
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
                color: colors.light
                textFormat: Text.MarkdownText
                wrapMode: Text.WordWrap
                font: fonts.secondaryFont(24)
            }
        }
    }
}
