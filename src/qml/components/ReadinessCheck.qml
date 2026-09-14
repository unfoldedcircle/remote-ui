// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 READINESS CHECK COMPONENT

 The screens shown when the readiness check of an activity comes back with problems: a headline over the page
 with the count of affected devices and where the run would stop, and the predicted run step by step behind
 "What is wrong". Cancel and Proceed end the decision from either screen.

 A passing check never gets here - nothing is shown and the sequence runs. Several activities can fail their
 check at once (turning off every activity): each report queues on a stack and is decided in turn.

 ********************************************************************
 API
 ********************************************************************
 - show(summary, proceed): summary is the report summary of SequenceReadinessReport::toSummary(),
   proceed the function to call when the user runs the sequence anyway
 - clearAll()
**/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.0

import "qrc:/components" as Components

Popup {
    id: readinessCheck
    x: 0; y: 0
    width: parent.width
    height: parent.height
    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0

    onOpened: {
        buttonNavigation.takeControl();
    }

    onClosed: {
        buttonNavigation.releaseControl();
    }

    function show(summary, proceed) {
        // the same decision is never asked twice at once
        for (let i = 0; i < reportList.depth; i++) {
            const item = reportList.get(i);

            if (item.summary.entityId === summary.entityId && item.summary.cmdId === summary.cmdId) {
                return;
            }
        }

        readinessCheck.open();
        reportList.push(reportComponent.createObject(reportList, { summary: summary, proceed: proceed }));
    }

    function clearAll() {
        readinessCheck.close();

        for (let i = 0; i < reportList.depth; i++) {
            reportList.get(i).destroy();
        }

        reportList.clear();
    }

    Components.ButtonNavigation {
        id: buttonNavigation
        defaultConfig: {
            "BACK": {
                "pressed": function() {
                    if (reportList.currentItem) {
                        reportList.currentItem.close();
                    }
                }
            },
            "HOME": {
                "pressed": function() {
                    readinessCheck.clearAll();
                }
            },
            "DPAD_MIDDLE": {
                "pressed": function() {
                    if (reportList.currentItem) {
                        reportList.currentItem.activate();
                    }
                }
            },
            "DPAD_LEFT": {
                "pressed": function() {
                    if (reportList.currentItem) {
                        reportList.currentItem.selection = 1;
                    }
                }
            },
            "DPAD_RIGHT": {
                "pressed": function() {
                    if (reportList.currentItem) {
                        reportList.currentItem.selection = 2;
                    }
                }
            },
            "DPAD_UP": {
                "pressed": function() {
                    if (reportList.currentItem) {
                        reportList.currentItem.moveUp();
                    }
                }
            },
            "DPAD_DOWN": {
                "pressed": function() {
                    if (reportList.currentItem) {
                        reportList.currentItem.moveDown();
                    }
                }
            }
        }
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; easing.type: Easing.InExpo; duration: 300 }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; easing.type: Easing.OutExpo; duration: 200 }
    }

    background: Item {}

    contentItem: StackView {
        id: reportList
    }

    // Short status for one row of the plan: the reason at row width, composed from the code and the names the
    // core already translated. The report's own message is an English diagnostic and is never shown.
    function stepStatusText(row) {
        switch (row.marker) {
        case "notNeeded":
            //: Status of a step the activity leaves out because its device is already in the wanted state.
            return qsTr("Not needed — already in this state");
        case "skipped":
            //: Status of a step whose device was deleted after the activity was set up.
            return qsTr("Skipped — deleted");
        case "ok":
            return "";
        }

        switch (row.code) {
        case "INTEGRATION_NOT_CONNECTED":
            return qsTr("Not connected");
        case "INTEGRATION_DISABLED":
            return qsTr("Integration disabled");
        case "IR_EMITTER_NOT_AVAILABLE":
            return qsTr("IR emitter unavailable");
        case "REMOTE_IR_OUTPUT_MISSING":
            return qsTr("No IR output configured");
        case "REMOTE_IR_OUTPUT_INVALID":
            return qsTr("IR output unavailable");
        case "BT_NOT_CONNECTED":
            return qsTr("Bluetooth not connected");
        case "ENTITY_UNAVAILABLE":
            return qsTr("Not available");
        case "SEQUENCE_ALREADY_RUNNING":
            return qsTr("Already running");
        default:
            // NESTING_LIMIT, INVALID_COMMAND and codes a newer core may add
            return qsTr("Cannot run right now");
        }
    }

    function markerIcon(row) {
        if (row.type === "delay") {
            return "uc:clock";
        }

        switch (row.marker) {
        case "aborting":
            return "uc:circle-xmark";
        case "blocked":
            return "uc:triangle-exclamation";
        case "skipped":
            return "uc:forward-step";
        case "notNeeded":
            return "uc:circle-minus";
        default:
            return "uc:check";
        }
    }

    function markerColor(row) {
        if (row.type === "delay") {
            return colors.light;
        }

        switch (row.marker) {
        case "aborting":
            return colors.red;
        case "blocked":
            return colors.orange;
        case "skipped":
            return colors.yellow;
        case "notNeeded":
            return colors.inactiveText;
        default:
            return colors.green;
        }
    }

    // the letter-spaced variants of the secondary font, for the kicker and the "stops here" rule
    function spacedFont(size, spacing) {
        let font = fonts.secondaryFont(size);
        font.letterSpacing = spacing;
        return font;
    }

    // Cancel on the left, Proceed on the right. The ring follows the keypad selection, and only shows while
    // the keypad is in use so a screen opened by touch starts unhighlighted.
    component DecisionButtons: Item {
        id: decisionButtons
        property int selection: 2
        signal cancelled()
        signal proceeded()

        height: proceedButton.height

        Text {
            id: cancelButton
            //: Button on the readiness check screens: do not run the activity.
            text: qsTr("Cancel")
            width: parent.width / 2 - 10
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignLeft
            color: colors.offwhite
            font: fonts.secondaryFont(26, "Bold")
            anchors { left: parent.left; verticalCenter: parent.verticalCenter }

            Rectangle {
                anchors { fill: parent; margins: -8 }
                radius: ui.cornerRadiusSmall
                color: colors.transparent
                border { width: 2; color: decisionButtons.selection === 1 && ui.keyNavigationActive
                                          ? colors.highlight : colors.transparent }
            }

            Components.HapticMouseArea {
                width: parent.width + 40
                height: parent.height + 40
                anchors.centerIn: parent
                onClicked: decisionButtons.cancelled()
            }
        }

        Text {
            id: proceedButton
            //: Button on the readiness check screens: run the activity although devices are not ready.
            text: qsTr("Proceed")
            width: parent.width / 2 - 10
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignRight
            color: colors.offwhite
            font: fonts.secondaryFont(26, "Bold")
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }

            Rectangle {
                anchors { fill: parent; margins: -8 }
                radius: ui.cornerRadiusSmall
                color: colors.transparent
                border { width: 2; color: decisionButtons.selection === 2 && ui.keyNavigationActive
                                          ? colors.highlight : colors.transparent }
            }

            Components.HapticMouseArea {
                width: parent.width + 40
                height: parent.height + 40
                anchors.centerIn: parent
                onClicked: decisionButtons.proceeded()
            }
        }
    }

    Component {
        id: reportComponent

        Item {
            id: reportItem

            property var summary
            property var proceed

            // "headline" or "detail"
            property string screen: "headline"
            // keypad selection: 0 the "What is wrong" row (headline only), 1 Cancel, 2 Proceed
            property int selection: 2

            readonly property bool notReady: summary.verdict === "not_ready"
            // the run stops: red. It runs on, but a device stays silent: orange, as in the step list
            readonly property color severityColor: notReady ? colors.red : colors.orange
            // a blocked step without a device name would otherwise count as no device at all
            readonly property int deviceCount: summary.unresponsiveNames.length > 0
                                               ? summary.unresponsiveNames.length
                                               : summary.abortingSteps + summary.blockedSteps

            function close() {
                if (reportList.depth < 2) {
                    readinessCheck.close();
                    reportList.clear();
                } else {
                    reportList.pop();
                }

                reportItem.destroy();
            }

            function accept() {
                const run = reportItem.proceed;
                reportItem.close();
                run();
            }

            function showDetail() {
                planList.currentIndex = 0;
                reportItem.selection = 2;
                reportItem.screen = "detail";
            }

            function activate() {
                if (reportItem.screen === "headline" && reportItem.selection === 0) {
                    reportItem.showDetail();
                    return;
                }

                if (reportItem.selection === 1) {
                    reportItem.close();
                } else {
                    reportItem.accept();
                }
            }

            function moveUp() {
                if (reportItem.screen === "headline") {
                    reportItem.selection = 0;
                } else {
                    planList.decrementCurrentIndex();
                }
            }

            function moveDown() {
                if (reportItem.screen === "headline") {
                    if (reportItem.selection === 0) {
                        reportItem.selection = 2;
                    }
                } else {
                    planList.incrementCurrentIndex();
                }
            }

            // ----- screen 1: headline -----

            Item {
                id: headline
                anchors.fill: parent
                visible: reportItem.screen === "headline"

                // the page stays visible behind the message
                LinearGradient {
                    anchors.fill: parent
                    start: Qt.point(0, 0)
                    end: Qt.point(0, parent.height)
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: colors.transparent }
                        GradientStop { position: 0.34; color: Qt.rgba(0, 0, 0, 0.55) }
                        GradientStop { position: 0.58; color: colors.black }
                        GradientStop { position: 1.0; color: colors.black }
                    }
                }

                Rectangle {
                    id: colorBar
                    width: parent.width - 20
                    height: 4
                    radius: ui.cornerRadiusSmall
                    color: reportItem.severityColor
                    anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom }
                }

                SequentialAnimation {
                    loops: Animation.Infinite
                    running: headline.visible && readinessCheck.opened

                    NumberAnimation { target: colorBar; properties: "opacity"; from: 1; to: 0; duration: 1000 }
                    NumberAnimation { target: colorBar; properties: "opacity"; from: 0; to: 1; duration: 1000 }
                    PauseAnimation { duration: 2000 }
                }

                Column {
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                              leftMargin: 20; rightMargin: 20; bottomMargin: 30 }

                    // The activity name is its own clamped line, never part of a sentence: a long name elides
                    // instead of breaking the layout.
                    Text {
                        width: parent.width
                        text: reportItem.summary.name
                        maximumLineCount: 1
                        elide: Text.ElideRight
                        color: colors.light
                        font: readinessCheck.spacedFont(24, 2)
                        bottomPadding: 12
                    }

                    Components.Icon {
                        icon: "uc:triangle-exclamation"
                        size: 56
                        color: reportItem.severityColor
                    }

                    Text {
                        width: parent.width
                        //: Headline of the readiness check screen. %n is the number of devices that will not react.
                        text: qsTr("%n device(s) need attention", "", reportItem.deviceCount)
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        color: colors.offwhite
                        font: fonts.primaryFont(40)
                        lineHeight: 1.05
                        topPadding: 8
                    }

                    Text {
                        width: parent.width
                        text: reportItem.summary.stopLabel !== ""
                              //: %1 is the position of the step the activity would stop at, %2 the number of steps.
                              ? qsTr("The activity would stop at step %1 of %2.")
                                    .arg(reportItem.summary.stopLabel).arg(reportItem.summary.stepCount)
                              //: The activity runs to its end, but some devices will not react.
                              : qsTr("Some devices will not respond.")
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        color: colors.light
                        font: fonts.secondaryFont(28)
                        topPadding: 8
                        bottomPadding: 24
                    }

                    // the one way into the step list
                    Item {
                        id: detailRow
                        width: parent.width
                        height: detailRowLabel.height + 32

                        Rectangle { width: parent.width; height: 1; color: colors.medium; anchors.top: parent.top }
                        Rectangle { width: parent.width; height: 1; color: colors.medium; anchors.bottom: parent.bottom }

                        Text {
                            id: detailRowLabel
                            //: Row on the readiness check screen that opens the list of steps.
                            text: qsTr("What is wrong")
                            color: colors.offwhite
                            font: fonts.secondaryFont(26, "Bold")
                            anchors { left: parent.left; leftMargin: 4; verticalCenter: parent.verticalCenter }
                        }

                        Components.Icon {
                            icon: "uc:chevron-right"
                            size: 28
                            color: colors.offwhite
                            anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                        }

                        Rectangle {
                            anchors { fill: parent; leftMargin: -8; rightMargin: -8 }
                            radius: ui.cornerRadiusSmall
                            color: colors.transparent
                            border { width: 2; color: reportItem.selection === 0 && ui.keyNavigationActive
                                                      ? colors.highlight : colors.transparent }
                        }

                        Components.HapticMouseArea {
                            anchors.fill: parent
                            onClicked: reportItem.showDetail()
                        }
                    }

                    Item { width: 1; height: 28 }

                    DecisionButtons {
                        width: parent.width
                        selection: reportItem.selection
                        onCancelled: reportItem.close()
                        onProceeded: reportItem.accept()
                    }
                }
            }

            // ----- screen 2: what is wrong -----

            Item {
                id: detail
                anchors.fill: parent
                visible: reportItem.screen === "detail"

                Rectangle {
                    anchors.fill: parent
                    color: colors.black
                }

                Text {
                    id: detailTitle
                    //: Title of the screen listing the steps of an activity that is not ready.
                    text: qsTr("What is wrong")
                    color: colors.offwhite
                    font: fonts.primaryFont(32)
                    // the status bar owns the first 40 px
                    anchors { top: parent.top; topMargin: 60; left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20 }
                }

                Text {
                    id: detailSubtitle
                    //: %1 is the name of the activity, %n the number of steps of its sequence.
                    text: qsTr("%1 · %n step(s)", "", reportItem.summary.stepCount).arg(reportItem.summary.name)
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    color: colors.light
                    font: fonts.secondaryFont(22)
                    anchors { top: detailTitle.bottom; topMargin: 4; left: parent.left; leftMargin: 20; right: parent.right; rightMargin: 20 }
                }

                ListView {
                    id: planList
                    anchors { top: detailSubtitle.bottom; topMargin: 16; bottom: detailButtons.top; bottomMargin: 16
                              left: parent.left; leftMargin: 10; right: parent.right; rightMargin: 10 }
                    clip: true
                    model: reportItem.summary.plan
                    currentIndex: 0

                    maximumFlickVelocity: 6000
                    flickDeceleration: 1000
                    highlightMoveDuration: 150
                    // the keypad drives the list through ButtonNavigation; the focus chain must not double it
                    keyNavigationEnabled: false

                    // the highlight keeps the current row on screen, the ring itself is drawn by the row
                    highlight: Item {}

                    delegate: Item {
                        id: planRow
                        width: ListView.view.width
                        height: rowContent.height + (stopRule.visible ? stopRule.height : 0)

                        readonly property bool isCurrent: ListView.isCurrentItem
                        readonly property bool isDelay: modelData.type === "delay"
                        readonly property string status: readinessCheck.stepStatusText(modelData)

                        Item {
                            id: rowContent
                            width: parent.width
                            height: Math.max(nameColumn.height, 26) + 24
                            // the steps after the stop are held back lightly, they must stay readable
                            opacity: modelData.reached ? 1 : 0.72

                            Text {
                                id: rowLabel
                                width: 26
                                text: modelData.label
                                color: colors.inactiveText
                                font: fonts.secondaryFont(22)
                                anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                            }

                            Components.Icon {
                                id: rowMarker
                                icon: readinessCheck.markerIcon(modelData)
                                size: 26
                                color: readinessCheck.markerColor(modelData)
                                anchors { left: rowLabel.right; leftMargin: 14; verticalCenter: parent.verticalCenter }
                            }

                            Column {
                                id: nameColumn
                                spacing: 4
                                anchors { left: rowMarker.right; leftMargin: 14; right: parent.right; rightMargin: 10
                                          verticalCenter: parent.verticalCenter }

                                Text {
                                    width: parent.width
                                    text: planRow.isDelay
                                          //: A delay step of an activity. %1 is the number of seconds.
                                          ? qsTr("Wait %1 s").arg(modelData.delay / 1000)
                                          : modelData.name
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                    color: colors.offwhite
                                    font: planRow.isDelay ? fonts.secondaryFont(25) : fonts.primaryFont(27)
                                }

                                Text {
                                    width: parent.width
                                    visible: planRow.status !== ""
                                    text: planRow.status
                                    maximumLineCount: 1
                                    elide: Text.ElideRight
                                    color: readinessCheck.markerColor(modelData)
                                    font: fonts.secondaryFont(23)
                                }
                            }

                            // The rows are not actionable: the d-pad only scrolls the list. A thin bar at the
                            // edge marks the position without suggesting the row can be activated.
                            Rectangle {
                                width: 3
                                radius: 1.5
                                color: colors.highlight
                                visible: planRow.isCurrent && ui.keyNavigationActive
                                anchors { left: parent.left; top: parent.top; bottom: parent.bottom
                                          topMargin: 10; bottomMargin: 10 }
                            }
                        }

                        // the run ends with the first aborting step: the rule belongs to that row alone
                        Item {
                            id: stopRule
                            visible: modelData.stopsRun
                            width: parent.width
                            height: 36
                            anchors.top: rowContent.bottom

                            Rectangle {
                                height: 1
                                color: colors.red
                                opacity: 0.5
                                anchors { left: parent.left; leftMargin: 10; right: stopLabel.left; rightMargin: 12
                                          verticalCenter: parent.verticalCenter }
                            }

                            Text {
                                id: stopLabel
                                //: Marks the step an activity would stop at. Upper case.
                                text: qsTr("STOPS HERE")
                                color: colors.red
                                font: readinessCheck.spacedFont(20, 2)
                                anchors.centerIn: parent
                            }

                            Rectangle {
                                height: 1
                                color: colors.red
                                opacity: 0.5
                                anchors { left: stopLabel.right; leftMargin: 12; right: parent.right; rightMargin: 10
                                          verticalCenter: parent.verticalCenter }
                            }
                        }
                    }
                }

                Components.ScrollIndicator {
                    parentObj: planList
                    padding: 10
                }

                DecisionButtons {
                    id: detailButtons
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                              leftMargin: 20; rightMargin: 20; bottomMargin: 30 }
                    selection: reportItem.selection
                    onCancelled: reportItem.close()
                    onProceeded: reportItem.accept()
                }
            }
        }
    }
}
