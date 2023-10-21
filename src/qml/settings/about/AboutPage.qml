// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

import ResourceTypes 1.0

import "qrc:/components" as Components
import "qrc:/settings" as Settings

Settings.Page {
    id: aboutPageContent

    property int type
    property int scrollCounter: 1

    function scrollDown() {
        flickable.contentY += 100 * scrollCounter;
        if (flickable.contentY > flickable.contentHeight - flickable.height) {
            flickable.contentY = flickable.contentHeight - flickable.height;
        }
    }

    function scrollUp() {
        if (flickable.contentY == 0) {
            return;
        }
        flickable.contentY -= 100 * scrollCounter;
        if (flickable.contentY < 0) {
            flickable.contentY = 0;
        }
    }

    Component.onCompleted: {
        resource.getAboutInfo(type);

        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         aboutPageContent.scrollDown();
                                                     },
                                                     "released": function() {
                                                         scrollCounter = 1;
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         aboutPageContent.scrollUp();
                                                     },
                                                     "released": function() {
                                                         scrollCounter = 1;
                                                     }
                                                 }
                                             });
    }

    Connections {
        target: resource
        ignoreUnknownSignals: true

        function onAboutInfo(res, baseDir) {
            content.baseUrl = "file:" + baseDir + "/";
            content.text = res;
        }
    }

    Flickable {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom; horizontalCenter: parent.horizontalCenter }
        contentWidth: parent.width - 20; contentHeight: content.implicitHeight
        clip: true
        flickableDirection: Flickable.VerticalFlick

        Behavior on contentY {
            NumberAnimation { easing.type: scrollCounter === 1 ? Easing.OutExpo : Easing.Linear; duration: 500 }
        }

        Text {
            id: content
            width: parent.width
            wrapMode: Text.WordWrap
            color: colors.light
            textFormat: aboutPageContent.type === ResourceTypes.Licenses ? Text.MarkdownText : Text.RichText
            font: fonts.secondaryFont(24)
            x: 10
            onLinkActivated: {
                if (link.includes("http")) {
                    return;
                }

                if (content.followLinks) {
                    content.followLinks = false;
                    content.text = resource.getLinkContent(content.baseUrl, link);
                }
            }

            property bool followLinks: true
        }

        ScrollBar.vertical: ScrollBar {
            opacity: 0.5
        }
    }

    Components.ScrollIndicator {
        parentObj: flickable
    }
}
