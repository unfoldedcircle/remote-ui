// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 SCROLL INDICATOR COMPONENT
 The one scroll indicator of a scrolling page or list (docs/design-system.md section 5): a thin track at the right
 edge of the Flickable, with a thumb that shows the visible part. It is only shown when there is more content than
 fits.

 ********************************************************************
 CONFIGURABLE PROPERTIES AND OVERRIDES:
 ********************************************************************
 - parentObj: the Flickable, ListView or GridView it belongs to (a sibling, default: the parent)
 - padding: distance from the top and bottom of the Flickable
 - hideOverride: hide it, e.g. while a list is replaced
**/

import QtQuick 2.15

Rectangle {
    id: scrollIndicator
    width: 6
    radius: width / 2
    color: colors.surfaceRaised
    anchors { right: parentObj.right; rightMargin: 4; top: parentObj.top; topMargin: padding;
              bottom: parentObj.bottom; bottomMargin: padding }
    visible: !hideOverride && scrollable && parentObj.contentHeight > parentObj.height + 1

    property QtObject parentObj: parent
    property int padding: 10
    property bool hideOverride: false

    readonly property bool scrollable: parentObj && parentObj.visibleArea !== undefined

    Rectangle {
        width: parent.width
        radius: width / 2
        color: colors.textSecondary
        height: scrollIndicator.scrollable ? Math.max(40, scrollIndicator.height * scrollIndicator.parentObj.visibleArea.heightRatio)
                                           : 0
        y: scrollIndicator.scrollable ? Math.min(scrollIndicator.height - height,
                                                 Math.max(0, scrollIndicator.height * scrollIndicator.parentObj.visibleArea.yPosition))
                                      : 0
    }
}
