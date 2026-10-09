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

    Component.onCompleted: {
        resource.getAboutInfo(type);

        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         flickable.scrollStep(1);
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         flickable.scrollStep(-1);
                                                     }
                                                 }
                                             });
    }

    Connections {
        target: resource
        ignoreUnknownSignals: true

        function onAboutInfo(res, baseDir) {
            flickable.baseUrl = "file:" + baseDir + "/";
            flickable.text = res;
        }
    }

    Components.Prose {
        id: flickable
        width: parent.width
        anchors { top: topNavigation.bottom; bottom: parent.bottom }
        textFormat: aboutPageContent.type === ResourceTypes.Licenses ? Text.MarkdownText : Text.RichText

        property bool followLinks: true

        onLinkActivated: {
            if (link.includes("http")) {
                return;
            }

            if (followLinks) {
                followLinks = false;
                // the linked document arrives through onAboutInfo; getLinkContent() returns nothing
                resource.getLinkContent(flickable.baseUrl, link);
            }
        }
    }
}
