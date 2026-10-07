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

            const lines = res.split("\n");
            if (!aboutPageContent.isMarkdown) {
                aboutPageContent.stringList = lines;
                return;
            }

            const parts = [];
            let currentPart = "";

            for (const line of lines) {
                if (/^##\s/.test(line)) {
                    if (currentPart !== "") {
                        parts.push(currentPart.trim());
                        currentPart = '';
                    }
                }
                currentPart += line + "\n";
            }

            if (currentPart !== "") {
                parts.push(currentPart.trim());
            }

            aboutPageContent.stringList = parts;
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
            wrapMode: Text.WordWrap
            color: colors.textPrimary
            baseUrl: aboutPageContent.baseDir
            text: model.modelData
            textFormat: aboutPageContent.isMarkdown ? Text.MarkdownText : Text.RichText
            linkColor: colors.textPrimary
            font: fonts.prose()
            lineHeight: fonts.proseLineHeight
            onLinkActivated: {
                if (link.includes("http")) {
                    return;
                }

                if (aboutPageContent.followLinks) {
                    aboutPageContent.followLinks = false;
                    aboutPageContent.isMarkdown = false;
                    resource.getLinkContent(content.baseUrl, link);
                }
            }
        }
    }

    Components.ScrollIndicator {
        parentObj: flickable
    }
}
