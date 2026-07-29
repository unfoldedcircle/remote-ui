// Copyright (c) 2022-2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <vector>

namespace uc {
namespace hw {

/**
 * @brief Parses the Linux evdev event stream of the capacitive touch slider into SYN_REPORT
 * delimited frames and coalesces move events: one drain of the device queue produces at most
 * one move output per gesture segment, while press/release transitions are preserved in order.
 *
 * The class intentionally avoids Qt and linux/input.h so the unit tests can compile and run
 * on any host. The event type/code values below are stable kernel ABI
 * (see linux/input-event-codes.h).
 */
class TouchSliderEventFilter {
 public:
    enum EventType : uint16_t { TypeSyn = 0x00, TypeKey = 0x01, TypeAbs = 0x03 };
    enum EventCode : uint16_t { CodeSynReport = 0x00, CodeSynDropped = 0x03, CodeAbsX = 0x00, CodeBtnTouch = 0x14a };

    struct Output {
        enum Type { Pressed, Moved, Released, Dropped };

        Type type;
        int  x;
    };

    void processEvent(uint16_t type, uint16_t code, int32_t value);

    /**
     * @brief Flushes the coalesced move and returns all outputs produced since the last call.
     * Call once after the device queue has been drained.
     */
    std::vector<Output> endOfDrain();

    bool isTouching() const { return m_touching; }
    int  currentX() const { return m_currentX; }

    /**
     * @brief Resets the filter to a known device state, e.g. at startup.
     */
    void resyncState(int x, bool touching);

 private:
    void commitFrame();
    void clearFrame();

    bool    m_touching = false;
    bool    m_dropped = false;
    int     m_currentX = 0;
    bool    m_pendingMove = false;
    bool    m_frameHasX = false;
    int32_t m_frameX = 0;
    bool    m_frameHasBtn = false;
    int32_t m_frameBtn = 0;

    std::vector<Output> m_outputs;
};

}  // namespace hw
}  // namespace uc
