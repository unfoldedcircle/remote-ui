// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace uc {
namespace ui {

/**
 * Whether a control may send a command to an entity at all, decided before the command is created.
 *
 * This is the counterpart of commandRetryPolicy.h: that one judges a command that was sent and failed, this one
 * judges the entity a command is about to be addressed to.
 */

/**
 * @brief true if a command may be sent to an entity in this state.
 *
 * An unavailable entity accepts no command - it is refused where the user pressed, with a notification, instead of
 * being sent and dropped somewhere along the way.
 *
 * The one exception is the remote coming back from a wakeup. A suspend, and any other loss of the core connection,
 * marks *every* entity unavailable (EntityController::onCoreDisconnected() -> setAllEntitiesAvailable(false) ->
 * Base::setState(0)) until the entities have been reloaded, and the button press that wakes the remote lands
 * exactly in that gap. A physical key press has to reach the device whether the remote sleeps or the display is
 * off, so the command is sent, and the resume window carries it until the integrations are back
 * (see commandRetryPolicy.h).
 *
 * @param entityAvailable the entity's availability: its state is not Unavailable, i.e. Base::isEnabled()
 * @param resumePending true from the moment the remote goes to sleep until the resume window has closed again,
 *        i.e. EntityController::getResumePending(). It is false when retrying after a wakeup is turned off, and
 *        so is this exception.
 */
bool mayCommandEntity(bool entityAvailable, bool resumePending);

}  // namespace ui
}  // namespace uc
