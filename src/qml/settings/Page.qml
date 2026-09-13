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

    // A key that no control and no KeyNavigation link accepted bubbles up to this root: the focus sits
    // at the top or bottom of the page's focus chain. Scroll the page on, so the content after the last
    // control (a description, a trailing note) can be reached with the d-pad. This is the QML focus
    // path only - the button navigation of an idiom-a page has no DPAD handlers, so nothing fires twice.
    // Pages that drive a selection through their button navigation keep the focus on the page itself
    // and are skipped here: they scroll on that path already.
    function scrollChainEnd(event, direction) {
        if (!scrollTarget || !buttonNavigation.hasInputControl) {
            return;
        }

        const focused = buttonNavigation.windowFocusItem;
        if (!focused || focused === settingsPageBase || focused === buttonNavigation
                || !buttonNavigation.isInScope(focused)) {
            return;
        }

        event.accepted = buttonNavigation.scrollBy(direction * Math.round(scrollTarget.height / 2));
    }

    Keys.onDownPressed: scrollChainEnd(event, 1)
    Keys.onUpPressed: scrollChainEnd(event, -1)

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
