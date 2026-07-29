// Copyright (c) 2022-2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "touchSliderEventFilter.h"

#include <vector>

namespace uc {
namespace hw {

void TouchSliderEventFilter::processEvent(uint16_t type, uint16_t code, int32_t value) {
    if (m_dropped) {
        // after SYN_DROPPED the device state is unreliable: discard everything up to and
        // including the next SYN_REPORT
        if (type == TypeSyn && code == CodeSynReport) {
            m_dropped = false;
            clearFrame();
        }
        return;
    }

    if (type == TypeAbs && code == CodeAbsX) {
        m_frameHasX = true;
        m_frameX = value;
    } else if (type == TypeKey && code == CodeBtnTouch) {
        m_frameHasBtn = true;
        m_frameBtn = value;
    } else if (type == TypeSyn && code == CodeSynDropped) {
        m_dropped = true;
        m_pendingMove = false;
        clearFrame();
        m_outputs.push_back({Output::Dropped, m_currentX});
    } else if (type == TypeSyn && code == CodeSynReport) {
        commitFrame();
    }
}

void TouchSliderEventFilter::commitFrame() {
    // commit X before the press decision so consumers reading the current position on a
    // press notification see the position of this gesture, not the previous one
    if (m_frameHasX) {
        m_currentX = m_frameX;
    }

    if (m_frameHasBtn && m_frameBtn == 1 && !m_touching) {
        m_touching = true;
        // the X of the press frame is the gesture start position, not movement
        m_pendingMove = false;
        m_outputs.push_back({Output::Pressed, m_currentX});
    } else if (m_frameHasX && m_touching) {
        // coalesce: the latest position wins, endOfDrain() or a release flushes it
        m_pendingMove = true;
    }

    if (m_frameHasBtn && m_frameBtn == 0 && m_touching) {
        // flush the coalesced move first so consumers see the final finger position
        if (m_pendingMove) {
            m_outputs.push_back({Output::Moved, m_currentX});
            m_pendingMove = false;
        }
        m_touching = false;
        m_outputs.push_back({Output::Released, m_currentX});
    }

    clearFrame();
}

std::vector<TouchSliderEventFilter::Output> TouchSliderEventFilter::endOfDrain() {
    if (m_pendingMove) {
        m_outputs.push_back({Output::Moved, m_currentX});
        m_pendingMove = false;
    }

    std::vector<Output> outputs;
    outputs.swap(m_outputs);
    return outputs;
}

void TouchSliderEventFilter::resyncState(int x, bool touching) {
    m_currentX = x;
    m_touching = touching;
    m_pendingMove = false;
    m_dropped = false;
    clearFrame();
    m_outputs.clear();
}

void TouchSliderEventFilter::clearFrame() {
    m_frameHasX = false;
    m_frameHasBtn = false;
}

}  // namespace hw
}  // namespace uc
