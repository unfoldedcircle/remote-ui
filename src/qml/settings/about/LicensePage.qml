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
    // the links of the shown document in reading order, and the one the d-pad selected, -1 for none
    property var links: []
    property int selectedLink: -1
    // where the links of a block are: block -> { link: y within the block }
    property var linkPositions: ({})

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

        const closed = opened[opened.length - 1];
        aboutPageContent.openedLinks = opened.slice(0, -1);
        if (opened.length > 1) {
            aboutPageContent.openLink(opened[opened.length - 2].baseUrl, opened[opened.length - 2].link);
        } else {
            aboutPageContent.isMarkdown = true;
            resource.getAboutInfo(aboutPageContent.type);
        }
        flickable.positionViewAtIndex(closed.section, ListView.Beginning);
        // a link opened with the d-pad is selected again
        aboutPageContent.selectedLink = aboutPageContent.links.findIndex(function(link) {
            return link.block === closed.section && link.index === closed.linkIndex;
        });
    }

    // a link of the shown document: a heading of it, or another document of the legal directory
    function activate(link, block, linkIndex) {
        if (link.startsWith("#")) {
            const target = resource.licenseAnchorBlock(aboutPageContent.stringList, link.substring(1));
            if (target >= 0) {
                aboutPageContent.selectedLink = -1;
                flickable.positionViewAtIndex(target, ListView.Beginning);
            }
            return;
        }
        if (link.includes("http")) {
            return;
        }

        aboutPageContent.openedLinks = aboutPageContent.openedLinks.concat([{
            "baseUrl": aboutPageContent.baseDir, "link": link, "section": block, "linkIndex": linkIndex
        }]);
        aboutPageContent.openLink(aboutPageContent.baseDir, link);
    }

    // Text has no API for where a link is: look where linkAt() finds each link of a block, once per block and document
    function linkPositionsOf(block) {
        if (aboutPageContent.linkPositions[block] !== undefined) {
            return aboutPageContent.linkPositions[block];
        }
        const item = flickable.itemAtIndex(block);
        if (item === null || item.height === 0) {
            return null;
        }
        const found = {};
        for (let y = 8; y < item.height; y += 17) {
            for (let x = 0; x < item.width; x += 16) {
                const link = item.linkAt(x, y);
                if (link !== "" && found[link] === undefined) {
                    found[link] = y;
                }
            }
        }
        aboutPageContent.linkPositions[block] = found;
        return found;
    }

    // the position of a link in the list, -1 while its block is not laid out
    function linkY(i) {
        const link = aboutPageContent.links[i];
        const positions = aboutPageContent.linkPositionsOf(link.block);
        return positions !== null && positions[link.link] !== undefined
                ? flickable.itemAtIndex(link.block).y + positions[link.link] : -1;
    }

    // DPAD_DOWN / DPAD_UP select the next or previous link while it is shown or one scroll step away, and scroll
    // by half the page otherwise
    function moveSelection(direction) {
        const step = Math.round(flickable.height / 2);
        const viewTop = flickable.contentY;
        const viewBottom = viewTop + flickable.height;
        const links = aboutPageContent.links;
        let next = -1;
        if (aboutPageContent.selectedLink >= 0) {
            next = aboutPageContent.selectedLink + direction;
        } else {
            // nothing selected: the first link shown, from the top going down, from the bottom going up
            for (let i = direction > 0 ? 0 : links.length - 1; i >= 0 && i < links.length; i += direction) {
                const y = aboutPageContent.linkY(i);
                if (y >= viewTop && y <= viewBottom) {
                    next = i;
                    break;
                }
            }
        }

        const y = next >= 0 && next < links.length ? aboutPageContent.linkY(next) : -1;
        if (y >= 0 && y >= viewTop - (direction < 0 ? step : 0) && y <= viewBottom + (direction > 0 ? step : 0)) {
            aboutPageContent.selectedLink = next;
            const margin = 40;
            const maxContentY = Math.max(0, flickable.contentHeight - flickable.height);
            if (y + margin > viewBottom) {
                flickable.contentY = Math.min(maxContentY, y + margin - flickable.height);
            } else if (y - margin < viewTop) {
                flickable.contentY = Math.max(0, y - margin);
            }
            return;
        }

        const target = aboutPageContent.scrollStep(direction);
        // a selection the step scrolls out of view is dropped
        if (aboutPageContent.selectedLink >= 0) {
            const selectedY = aboutPageContent.linkY(aboutPageContent.selectedLink);
            if (selectedY < target || selectedY > target + flickable.height) {
                aboutPageContent.selectedLink = -1;
            }
        }
    }

    // the d-pad moves by half the viewport (docs/design-system.md section 6)
    function scrollStep(direction) {
        const maxContentY = Math.max(0, flickable.contentHeight - flickable.height);
        flickable.contentY = Math.max(0, Math.min(maxContentY, flickable.contentY + direction * Math.round(flickable.height / 2)));
        return flickable.contentY;
    }

    Component.onCompleted: {
        resource.getAboutInfo(type);

        buttonNavigation.extendDefaultConfig({
                                                 "DPAD_DOWN": {
                                                     "pressed": function() {
                                                         aboutPageContent.moveSelection(1);
                                                     }
                                                 },
                                                 "DPAD_UP": {
                                                     "pressed": function() {
                                                         aboutPageContent.moveSelection(-1);
                                                     }
                                                 },
                                                 "DPAD_MIDDLE": {
                                                     "pressed": function() {
                                                         const selected = aboutPageContent.links[aboutPageContent.selectedLink];
                                                         if (selected) {
                                                             aboutPageContent.activate(selected.link, selected.block,
                                                                                       selected.index);
                                                         }
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
            aboutPageContent.links = resource.licenseLinks(aboutPageContent.stringList);
            aboutPageContent.linkPositions = {};
            aboutPageContent.selectedLink = -1;
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
            // the link the d-pad selected is drawn on the selection fill while the keypad is in use
            text: aboutPageContent.selectedLink >= 0 && ui.keyNavigationActive
                  && aboutPageContent.links[aboutPageContent.selectedLink].block === index
                  ? resource.licenseBlockWithSelection(model.modelData,
                                                       aboutPageContent.links[aboutPageContent.selectedLink].index,
                                                       colors.surfaceSelected.toString())
                  : model.modelData
            textFormat: aboutPageContent.isMarkdown ? Text.MarkdownText : Text.PlainText
            linkColor: colors.textPrimary
            font: fonts.prose()
            lineHeight: fonts.proseLineHeight
            onLinkActivated: aboutPageContent.activate(link, index, -1)
        }
    }

    Components.ScrollIndicator {
        parentObj: flickable
    }
}
