// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "entityCommandPolicy.h"

namespace uc {
namespace ui {

bool mayCommandEntity(bool entityAvailable, bool resumePending) {
    // "unavailable" only means "the device is gone" once the remote is fully back: while it is waking up every
    // entity carries that state, and the press the user made has to be sent and retried, not refused
    return entityAvailable || resumePending;
}

}  // namespace ui
}  // namespace uc
