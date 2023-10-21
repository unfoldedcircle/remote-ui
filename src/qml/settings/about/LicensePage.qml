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
    property var stringList
    property string baseDir
    property bool followLinks: true

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
            aboutPageContent.baseDir = "file:" + baseDir + "/";

            const lines = res.split("\n");
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
            NumberAnimation { easing.type: scrollCounter === 1 ? Easing.OutExpo : Easing.Linear; duration: 500 }
        }

        delegate: Text {
            id: content
            width: ListView.view.width
            height: content.implicitHeight
            wrapMode: Text.WordWrap
            color: colors.light
            baseUrl: aboutPageContent.baseDir
            text: model.modelData
            textFormat: aboutPageContent.type === ResourceTypes.Licenses ? Text.MarkdownText : Text.RichText
            font: fonts.secondaryFont(24)
            onLinkActivated: {
                if (link.includes("http")) {
                    return;
                }

                if (aboutPageContent.followLinks) {
                    aboutPageContent.followLinks = false;
                    let res = resource.getLinkContent(content.baseUrl, link);
                    aboutPageContent.stringList = res.split('\n');
                }
            }
        }
    }

    Components.ScrollIndicator {
        parentObj: flickable
    }
}
