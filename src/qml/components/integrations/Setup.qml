// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Haptic 1.0
import Integration.Controller 1.0

import "qrc:/components" as Components

ColumnLayout {
    id: integrationSetupContainer

    signal done()

    spacing: 0

    Component.onCompleted: {
        if (IntegrationController.integrationDriverTosetup.discovered) {
            let setupData = {};
            setupData["driver_url"] = IntegrationController.integrationDriverTosetup.driverUrl;
            setupData["token"] = "";
            IntegrationController.configureDiscoveredIntegrationDriver(IntegrationController.integrationDriverTosetup.id, setupData);
        }
    }

    Item {
        id: setupTitle

        Layout.fillWidth: true
        Layout.preferredHeight: 60

        Text {
            text: qsTr("Integration setup")
            width: parent.width - 20
            elide: Text.ElideRight
            color: colors.offwhite
            verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
            anchors.centerIn: parent
            font: fonts.primaryFont(24)
        }

        Components.Icon {
            id: closeIcon
            color: colors.offwhite
            icon: "uc:xmark"
            anchors { verticalCenter: parent.verticalCenter; right: parent.right }
            size: 60
            visible: integrationSetupSwipeView.currentIndex == 1;

            Components.HapticMouseArea {
                width: parent.width + 20; height: width
                anchors.centerIn: parent
                onClicked: {
                    integrationSetupSwipeView.currentIndex = 2;
                }
            }
        }
    }

    SwipeView {
        id: integrationSetupSwipeView

        Layout.fillWidth: true
        Layout.fillHeight: true
        interactive: false
        clip: true

        Configure {
            id: configureStep
            onCancelled: {
                loading.stop();
                integrationSetupContainer.done();
            }
        }

        AddEntities {
            id: entitiesStep
            onDone: integrationSetupSwipeView.currentIndex = 2;
        }

        Finish {
            id: finishStep

            onDone: {
                integrationSetupContainer.done();
                integrationSetupSwipeView.currentIndex = 0;
            }

            onFailed: integrationSetupSwipeView.currentIndex = 0
        }
    }

    Connections {
        target: IntegrationController
        ignoreUnknownSignals: true

        function onIntegrationSetupStopped() {
            loading.stop();
        }

        function onConfigureDiscoveredIntegrationDriverError(message) {
            finishStep.success = false;
            finishStep.errorString = message;
            loading.failure();
            integrationSetupSwipeView.currentIndex = 2;
            configureStep.goToStart();
        }

        function onIntegrationSetupChange(driverId, state, error, data) {
            switch (state) {
            case IntegrationControllerEnums.Error:
                finishStep.success = false;
                finishStep.errorString = error;
                loading.failure();
                integrationSetupSwipeView.currentIndex = 2;
                configureStep.goToStart();
                break;
            case IntegrationControllerEnums.Ok:
                finishStep.success = true;
                loading.success();
                integrationSetupSwipeView.currentIndex = 1;
                entitiesStep.currentItem = true;
                break;
            }
        }
    }
}
