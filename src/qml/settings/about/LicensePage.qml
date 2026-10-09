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
    property bool isMarkdown: true
    // the documents opened by links, the last one shown: how each was opened and the block of the document before
    // it that held its link
    property var openedLinks: []

    topNavigation.goBack: function() {
        aboutPageContent.goBack();
    }

    function openLink(baseUrl, link) {
        aboutPageContent.isMarkdown = resource.isMarkdownFile(link);
        resource.getLinkContent(baseUrl, link);
    }

    // back to the document that held the link, at the block of the link; from the overview to the About page
    function goBack() {
        const opened = aboutPageContent.openedLinks;
        if (opened.length === 0) {
            profileRoot.goBack();
            buttonNavigation.restoreDefaultConfig();
            return;
        }

        aboutPageContent.openedLinks = opened.slice(0, -1);
        if (opened.length > 1) {
            aboutPageContent.openLink(opened[opened.length - 2].baseUrl, opened[opened.length - 2].link);
        } else {
            aboutPageContent.isMarkdown = true;
            resource.getAboutInfo(aboutPageContent.type);
        }
        flickable.positionViewAtIndex(opened[opened.length - 1].section, ListView.Beginning);
    }

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
                                                 },
                                                 "BACK": {
                                                     "pressed": function() {
                                                         aboutPageContent.goBack();
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
                                                                 aboutPageContent.openedLinks.length === 0);
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
                if (link.startsWith("#")) {
                    const block = resource.licenseAnchorBlock(aboutPageContent.stringList, link.substring(1));
                    if (block >= 0) {
                        flickable.positionViewAtIndex(block, ListView.Beginning);
                    }
                    return;
                }
                if (link.includes("http")) {
                    return;
                }

                aboutPageContent.openedLinks = aboutPageContent.openedLinks.concat([{
                    "baseUrl": content.baseUrl, "link": link, "section": index
                }]);
                aboutPageContent.openLink(content.baseUrl, link);
            }
        }
    }

    Components.ScrollIndicator {
        parentObj: flickable
    }
}
