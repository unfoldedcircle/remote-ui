// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import "qrc:/components" as Components
import "qrc:/settings" as Settings

Item {
    width: parent.width; height: parent.height;

    property var parentSwipeView
    property alias topNavigation: topNavigation
    property alias topNavigationText: topNavigation.text
    property alias buttonNavigation: buttonNavigation

    Settings.TopNavigation {
        id: topNavigation
        anchors.top: parent.top
        goBack: profileRoot.goBack
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "released": function() {
                    profileRoot.goBack();
                    buttonNavigation.restoreDefaultConfig();
                }
            },
            "HOME": {
                "released": function() {
                    profileRoot.goHome();
                }
            }
        }
    }
}
