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
    signal home()

    spacing: 0

    // called by the hosting popup once it is open (and once the loader is ready): a step that takes
    // the input while the popup is still invisible is dropped by the input controller
    function activateCurrentStep() {
        const step = integrationSetupSwipeView.currentItem;
        if (step && typeof step.activate === "function") {
            Qt.callLater(step.activate);
        }
    }

    // BACK on the hosting popup: cancel the running setup instead of only closing the popup. Once
    // the integration exists (the add-entities step) BACK skips to the finish step like the X icon.
    function cancel() {
        switch (integrationSetupSwipeView.currentIndex) {
        case 0:
            configureStep.cancelSetup();
            break;
        case 1:
            integrationSetupSwipeView.currentIndex = 2;
            break;
        default:
            if (finishStep.success) {
                finishStep.done();
            } else {
                finishStep.failed();
            }
        }
    }

    Component.onCompleted: {
        if (IntegrationController.integrationDriverTosetup.discovered) {
            let setupData = {};
            setupData["driver_url"] = IntegrationController.integrationDriverTosetup.driverUrl;
            setupData["token"] = "";
            IntegrationController.configureDiscoveredIntegrationDriver(IntegrationController.integrationDriverTosetup.id, setupData);
        }
    }

    // the close target only exists on the step that can be left that way: it skips to the result
    Components.TitleBar {
        id: setupTitle
        action: integrationSetupSwipeView.currentIndex == 1 ? "close" : ""
        text: qsTr("Integration setup")
        goBack: function() {
            integrationSetupSwipeView.currentIndex = 2;
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
            onHome: integrationSetupContainer.home()
        }

        AddEntities {
            id: entitiesStep
            onDone: integrationSetupSwipeView.currentIndex = 2;
            onHome: integrationSetupContainer.home()
        }

        Finish {
            id: finishStep

            // Done closes the hosting popup, which unloads this setup at the end of its fade-out;
            // resetting the swipe view here re-activated the configure step behind the fading
            // popup. Try again keeps the popup open and starts over at the configure step.
            onDone: integrationSetupContainer.done()
            onFailed: integrationSetupSwipeView.currentIndex = 0
            onHome: integrationSetupContainer.home()
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
                break;
            }
        }
    }
}
