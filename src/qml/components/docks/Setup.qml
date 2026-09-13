// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Dock.Controller 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: dockSetupContainer

    signal done()
    signal failed()
    signal home()

    spacing: 0

    // called by the hosting popup once it is open (and once the loader is ready): a step that takes
    // the input while the popup is still invisible is dropped by the input controller
    function activateCurrentStep() {
        const step = dockSetupSwipeView.currentItem;
        if (step && typeof step.activate === "function") {
            Qt.callLater(step.activate);
        }
    }

    // BACK on the hosting popup: cancel the running setup instead of only closing the popup
    function cancel() {
        if (dockSetupSwipeView.currentIndex === 0) {
            configureStep.cancelSetup();
        } else if (finishStep.success) {
            finishStep.done();
        } else {
            finishStep.failed();
        }
    }

    Item {
        id: setupTitle

        Layout.fillWidth: true
        Layout.preferredHeight: 60

        Text {
            text: qsTr("Dock setup")
            width: parent.width - 20
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(24)
        }
    }

    SwipeView {
        id: dockSetupSwipeView

        Layout.fillWidth: true
        Layout.fillHeight: true
        interactive: false
        clip: true

        Configure {
            id: configureStep
            onCancelled: {
                loading.stop();
                dockSetupContainer.done();
            }
            onHome: dockSetupContainer.home()
        }

        // Done / Try again close the hosting popup, which unloads this setup at the end of its
        // fade-out. The swipe view is not reset to the configure step here: that re-activated the
        // step behind the fading popup, which claimed its name field and left the on-screen
        // keyboard open over the page below.
        Finish {
            id: finishStep

            onDone: dockSetupContainer.done()
            onFailed: dockSetupContainer.failed()
            onHome: dockSetupContainer.home()
        }
    }

    Connections {
        target: DockController
        ignoreUnknownSignals: true

        function onSetupFinished(success, message) {
            if (success) {
                loading.success();
                finishStep.dockName = DockController.getConfiguredDock(DockController.dockToSetup).name
                dockSetupSwipeView.currentIndex = 1;
            } else {
                finishStep.success = false;
                finishStep.errorString = message;
                loading.failure();
                dockSetupSwipeView.currentIndex = 1;
            }
        }
    }
}
