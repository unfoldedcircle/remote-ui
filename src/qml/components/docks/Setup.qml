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

    spacing: 0

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
        }

        Finish {
            id: finishStep

            onDone: {
                dockSetupContainer.done();
                dockSetupSwipeView.currentIndex = 0;
            }

            onFailed: {
                dockSetupContainer.failed();
                dockSetupSwipeView.currentIndex = 0;
            }
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
