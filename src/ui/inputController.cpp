// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "inputController.h"

#include "../logging.h"

namespace uc {
namespace ui {

static bool isActuallyVisible(QQuickItem* it) {
    if (!it) return false;
    if (!it->window()) return false;
    if (!it->isVisible()) return false;
    if (!it->isEnabled()) return false;

    for (QQuickItem* p = it->parentItem(); p; p = p->parentItem()) {
        if (!p->isVisible()) return false;
        if (!p->isEnabled()) return false;
    }
    return true;
}

InputController *InputController::s_instance = nullptr;

InputController::InputController(hw::HardwareModel::Enum model) : m_model(model) {
    Q_ASSERT(s_instance == nullptr);
    s_instance = this;

    m_source = nullptr;
    m_globalPowerHoldTimer.setInterval(3000);
    m_globalPowerHoldTimer.setSingleShot(true);
    connect(&m_globalPowerHoldTimer, &QTimer::timeout, this, [this]() {
        if (!m_globalPowerPressed || m_globalPowerLongPressTriggered) {
            return;
        }

        m_globalPowerLongPressTriggered = true;
        emit globalPowerLongPressed();
    });
}

InputController::~InputController() {
    s_instance = nullptr;

    if (m_source != nullptr) {
        m_source->removeEventFilter(this);
    }
}

void InputController::setSource(QObject *source) {
    if (m_source == source) {
        return;
    }

    if (m_source != nullptr) {
        m_source->removeEventFilter(this);
    }

    m_source = source;

    if (m_source != nullptr) {
        m_source->installEventFilter(this);
    }

    qCDebug(lcInput()) << "Installed event filter for" << m_source;
}

void InputController::emitKey(Qt::Key key, bool release) {
    QKeyEvent keyPressEvent = QKeyEvent(release ? QEvent::Type::KeyRelease : QEvent::Type::KeyPress, key,
                                        Qt::NoModifier, QKeySequence(key).toString());
    QCoreApplication::sendEvent(m_source, &keyPressEvent);
}

void InputController::blockInput(bool value) {
    m_blockInput = value;

    if (m_blockInput) {
        m_globalPowerHoldTimer.stop();
        m_globalPowerPressed = false;
        m_globalPowerLongPressTriggered = false;
    }
}

void InputController::setBaseOwner(QObject *obj) {
    QMutexLocker lock(&m_mutex);

    auto* item = qobject_cast<QQuickItem*>(obj);
    if (!item) return;

    m_baseOwner = obj;
    updateActive();
}

void InputController::takeControl(QObject* obj) {
    QMutexLocker lock(&m_mutex);

    if (!obj) return;

            // ensure it's a QQuickItem (or at least something that can be used as a scope)
    auto* item = qobject_cast<QQuickItem*>(obj);
    if (!item) return;

            // remove if already in stack, then push to top
    for (int i = m_stack.size() - 1; i >= 0; --i) {
        if (m_stack[i] == obj) {
            m_stack.removeAt(i);
            break;
            // Invariant: each object appears at most once in the stack
        }
    }
    m_stack.push_back(obj);

            // auto-clean when destroyed.
            // Qt::UniqueConnection only works when connecting to a member function: with a lambda every
            // takeControl() call for the same object would add yet another connection, and scopes like the
            // main container are never destroyed, so those connections piled up for the whole runtime.
    connect(obj, &QObject::destroyed, this, &InputController::onOwnerDestroyed, Qt::UniqueConnection);

    updateActive();
}

void InputController::onOwnerDestroyed(QObject *owner) {
    QMutexLocker lock(&m_mutex);

    for (int i = m_stack.size() - 1; i >= 0; --i) {
        if (m_stack[i].data() == owner) {
            m_stack.removeAt(i);
        }
    }

    updateActive();
}

void InputController::releaseControl(QObject* obj) {
    QMutexLocker lock(&m_mutex);

    if (obj) {
        // remove that specific scope (e.g. notification closing)
        for (int i = m_stack.size() - 1; i >= 0; --i) {
            if (m_stack[i] == obj) {
                m_stack.removeAt(i);
                break;
            }
        }
    } else {
        // "release myself": pop top
        if (!m_stack.isEmpty())
            m_stack.removeLast();
    }

    updateActive();
}

QObject *InputController::qmlInstance(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    QObject *obj = s_instance;
    engine->setObjectOwnership(obj, QQmlEngine::CppOwnership);
    return obj;
}

void InputController::onPowerModeChanged(core::PowerEnums::PowerMode powerMode) {
    if (m_model == hw::HardwareModel::DEV) {
        return;
    }

    switch (powerMode) {
        case core::PowerEnums::PowerMode::NORMAL:
            m_blockTouchInput = false;
            break;
        case core::PowerEnums::PowerMode::LOW_POWER:
            m_blockTouchInput = true;
            break;
        default:
            break;
    }
}

bool InputController::eventFilter(QObject *obj, QEvent *event) {
    QKeyEvent *keyEvent;

    if (m_blockInput) {
        switch (event->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::TouchBegin:
            case QEvent::TouchUpdate:
            case QEvent::TouchEnd:
            case QEvent::TouchCancel:
                event->accept();
                return true;
            default:
                break;
        }
    }

    switch (event->type()) {
        case QEvent::KeyPress: {
            keyEvent = static_cast<QKeyEvent *>(event);
            int key = keyEvent->key();
            const QString mappedKey = m_keyCodeMapping.value(key);

            if (mappedKey.isEmpty()) {
                break;
            }

            cancelDeferredRelease(key);

            if (key == Qt::Key_PowerOff && !keyEvent->isAutoRepeat() && !m_globalPowerPressed) {
                m_globalPowerPressed = true;
                m_globalPowerLongPressTriggered = false;
                m_globalPowerHoldTimer.start();
            }

            QPointer<QObject> owner;

            if (keyEvent->isAutoRepeat()) {
                owner = m_keyOwner.value(key);
                if (owner.isNull()) {
                    owner = m_activeItem.isNull() ? m_baseOwner : m_activeItem;
                }
            } else {
                owner = m_activeItem.isNull() ? m_baseOwner : m_activeItem;
                m_keyOwner[key] = owner;
            }

            emit keyPressed(mappedKey);
            emit keyPressedFor(owner.data(), mappedKey);
            qCDebug(lcInput()) << "Key pressed:" << mappedKey << owner.data();
            break;
        }
        case QEvent::KeyRelease: {
            keyEvent = static_cast<QKeyEvent *>(event);
            int key = keyEvent->key();
            const QString mappedKey = m_keyCodeMapping.value(key);

            if (mappedKey.isEmpty()) {
                break;
            }

            if (keyEvent->isAutoRepeat()) {
                // An auto-repeat flagged release is either a synthetic mid-hold release
                // (followed by another press within the repeat period) or the terminal
                // release of a held key. Defer it: a follow-up press cancels the timer,
                // otherwise the release is delivered so stop handlers still fire.
                QTimer *timer = m_deferredRelease.value(key, nullptr);
                if (!timer) {
                    timer = new QTimer(this);
                    timer->setSingleShot(true);
                    timer->setInterval(150);
                    connect(timer, &QTimer::timeout, this, [this, key] {
                        emitKeyRelease(key, m_keyCodeMapping.value(key));
                    });
                    m_deferredRelease.insert(key, timer);
                }
                timer->start();
                break;
            }

            cancelDeferredRelease(key);
            emitKeyRelease(key, mappedKey);
            break;
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::TouchCancel: {
            if (m_blockTouchInput) {
                event->ignore();
                return true;
            }
            break;
        }
        default:  // do nothing
            break;
    }

    return QQuickItem::eventFilter(obj, event);
}

void InputController::emitKeyRelease(int key, const QString &mappedKey) {
    if (key == Qt::Key_PowerOff) {
        m_globalPowerHoldTimer.stop();
        m_globalPowerPressed = false;
        m_globalPowerLongPressTriggered = false;
    }

    QPointer<QObject> owner = m_keyOwner.take(key);
    if (owner.isNull()) {
        owner = m_activeItem.isNull() ? m_baseOwner : m_activeItem;
    }

    emit keyReleased(mappedKey);
    emit keyReleasedFor(owner.data(), mappedKey);
    qCDebug(lcInput()) << "Key released:" << mappedKey << owner.data();
}

void InputController::cancelDeferredRelease(int key) {
    QTimer *timer = m_deferredRelease.value(key, nullptr);
    if (timer) {
        timer->stop();
    }
}

void InputController::cleanupStack() {
    // remove null/dead or not-visible items from the top until we find a valid one
    // (or clean all invalid entries)
    for (int i = m_stack.size() - 1; i >= 0; --i) {
        auto* item = qobject_cast<QQuickItem*>(m_stack[i].data());
        if (!item || !isActuallyVisible(item))
            m_stack.removeAt(i);
    }
}

void InputController::updateActive() {
    cleanupStack();

    QObject* newActive = m_stack.isEmpty() ? nullptr : m_stack.last().data();

    if (!newActive)
        newActive = m_baseOwner.data();

    if (newActive == m_activeItem.data())
        return;

    m_activeItem = newActive;
    emit activeItemChanged();
    qCInfo(lcInput()) << "ACTIVE CONTROL ->" << m_activeItem.data();
}

}  // namespace ui
}  // namespace uc
