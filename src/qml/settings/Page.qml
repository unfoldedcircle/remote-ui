// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import "qrc:/components" as Components
import "qrc:/settings" as Settings

Item {
    id: settingsPageBase
    width: parent.width; height: parent.height;

    // Set this to the page's Flickable to keep the d-pad focused control on screen.
    property alias scrollTarget: buttonNavigation.scrollTarget

    // The control that should hold the keypad focus when the page opens.
    // Set this instead of calling forceActiveFocus() from a control's Component.onCompleted: the
    // level-2/3 Loaders are asynchronous, so the page being replaced is sometimes destroyed after
    // the new page has completed, and destroying the focused item clears the window's active focus
    // again. Declaring it here lets the page reclaim the focus once it owns the input.
    property alias initialFocusItem: buttonNavigation.initialFocusItem

    function ensureVisible(item) {
        buttonNavigation.ensureVisible(item);
    }

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
        // settings pages navigate through the QML focus chain, so the focus has to be handed over
        // whenever a popup takes the input, otherwise the page keeps reacting behind it
        manageFocus: true
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    profileRoot.goBack();
                    buttonNavigation.restoreDefaultConfig();
                }
            },
            "HOME": {
                "pressed": function() {
                    profileRoot.goHome();
                }
            }
        }
    }
}
