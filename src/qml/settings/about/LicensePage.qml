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
    property var stringList
    property string baseDir
    property bool followLinks: true
    property bool isMarkdown: true

    // the d-pad moves by half the viewport (docs/design-system.md section 6)
    function scrollStep(direction) {
        const maxContentY = Math.max(0, flickable.contentHeight - flickable.height);
        flickable.contentY = Math.max(0, Math.min(maxContentY, flickable.contentY + direction * Math.round(flickable.height / 2)));
    }

    Component.onCompleted: {
        resource.getAboutInfo(type);

        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         aboutPageContent.scrollStep(1);
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         aboutPageContent.scrollStep(-1);
                                                     }
                                                 }
                                             });
    }

    Connections {
        target: resource
        ignoreUnknownSignals: true

        function onAboutInfo(res, baseDir) {
            aboutPageContent.baseDir = "file:" + baseDir + "/";
            aboutPageContent.stringList = resource.licenseBlocks(res, aboutPageContent.isMarkdown,
                                                                 aboutPageContent.followLinks);
        }
    }

    ListView {
        id: flickable
        width: parent.width
        height: parent.height - topNavigation.height
        anchors { top: topNavigation.bottom; horizontalCenter: parent.horizontalCenter }
        clip: true
        model: aboutPageContent.stringList

        Behavior on contentY {
            NumberAnimation { easing.type: Easing.OutExpo; duration: 300 }
        }

        delegate: Text {
            id: content
            x: 20
            width: ListView.view.width - 40
            height: content.implicitHeight
            // a word or table cell wider than the screen breaks anywhere instead of being cut off
            wrapMode: Text.Wrap
            color: colors.textPrimary
            baseUrl: aboutPageContent.baseDir
            text: model.modelData
            textFormat: aboutPageContent.isMarkdown ? Text.MarkdownText : Text.PlainText
            linkColor: colors.textPrimary
            font: fonts.prose()
            lineHeight: fonts.proseLineHeight
            onLinkActivated: {
                if (link.includes("http")) {
                    return;
                }

                if (aboutPageContent.followLinks) {
                    aboutPageContent.followLinks = false;
                    aboutPageContent.isMarkdown = resource.isMarkdownFile(link);
                    resource.getLinkContent(content.baseUrl, link);
                }
            }
        }
    }

    Components.ScrollIndicator {
        parentObj: flickable
    }
}
