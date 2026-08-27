// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15

import "qrc:/components" as Components

MouseArea {
    id: showHelpBase
    anchors.fill: parent

    property alias navigation: navigation

    Component.onCompleted: buttonNavigation.takeControl()

    function close() {
        ui.showHelp = false;
        buttonNavigation.releaseControl();
    }

    // Shown right after the initial setup, the overlay is loaded together with its container (main
    // container or the "no page" screen) and completes before it, so the container's own
    // takeControl() lands on top of ours and the d-pad keeps switching the pages behind the tips.
    // Take the input back whenever the container ends up in front while the tips are open. The
    // container is the parent of the loader that holds this overlay.
    readonly property Item hostContainer: showHelpBase.parent && showHelpBase.parent.parent
                                          ? showHelpBase.parent.parent : null

    function reclaimInput() {
        if (ui.showHelp && hostContainer && ui.inputController.activeItem === hostContainer) {
            buttonNavigation.takeControl();
        }
    }

    Connections {
        target: ui.inputController

        // deferred: taking the input from inside the change notification would re-enter the
        // input controller
        function onActiveItemChanged() {
            Qt.callLater(showHelpBase.reclaimInput);
        }
    }

    // the tip pages cover the whole screen; the page behind only shimmers through
    Rectangle {
        anchors.fill: parent
        color: colors.black
        opacity: 0.95
    }

    RowLayout {
        id: navigation
        width: parent.width
        anchors { bottom: parent.bottom; bottomMargin: 10 }
        // the tip pages are added after this row and would otherwise paint over it
        z: 1

        Components.Icon {
            Layout.leftMargin: 20
            Layout.alignment: Qt.AlignLeft

            opacity: content.currentIndex > 0
            enabled: opacity === 1
            icon: "uc:arrow-left"
            size: 60
            color: colors.offwhite

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    content.decrementCurrentIndex();
                }
            }
        }

        Components.Button {
            Layout.alignment: Qt.AlignHCenter

            text: qsTr("Close")
            trigger: function() {
                showHelpBase.close();
            }
        }

        Components.Icon {
            Layout.rightMargin: 20
            Layout.alignment: Qt.AlignRight

            opacity: content.currentIndex !== content.count - 1
            enabled: opacity === 1
            icon: "uc:arrow-right"
            size: 60
            color: colors.offwhite

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    content.incrementCurrentIndex();
                }
            }
        }
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    showHelpBase.close();
                }
            },
            "HOME": {
                "pressed": function() {
                    showHelpBase.close();
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    showHelpBase.close();
                }
            },
            "DPAD_LEFT": {
                "pressed": function() {
                    content.decrementCurrentIndex();
                }
            },
            "DPAD_RIGHT": {
                "pressed": function() {
                    content.incrementCurrentIndex();
                }
            }
        }
    }
}
