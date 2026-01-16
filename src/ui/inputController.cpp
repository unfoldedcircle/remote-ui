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
}

InputController::~InputController() {
    s_instance = nullptr;

    if (m_source != nullptr) {
        m_source->removeEventFilter(this);
    }
}

void InputController::setSource(QObject *source) {
    source->installEventFilter(this);
    m_source = source;
    qCDebug(lcInput()) << "Installed event filter for" << m_source;
}

void InputController::emitKey(Qt::Key key, bool release) {
    QKeyEvent keyPressEvent = QKeyEvent(release ? QEvent::Type::KeyRelease : QEvent::Type::KeyPress, key,
                                        Qt::NoModifier, QKeySequence(key).toString());
    QCoreApplication::sendEvent(m_source, &keyPressEvent);
}

void InputController::blockInput(bool value) {
    m_blockInput = value;
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

            // auto-clean when destroyed
    connect(obj, &QObject::destroyed, this, [this](QObject* dead){
            QMutexLocker lock(&m_mutex);
            for (int i = m_stack.size() - 1; i >= 0; --i) {
                if (m_stack[i].data() == dead)
                    m_stack.removeAt(i);
            }
            updateActive();
        }, Qt::UniqueConnection);

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
        return false;
    }

    switch (event->type()) {
        case QEvent::KeyPress: {
            keyEvent = static_cast<QKeyEvent *>(event);
            int key = keyEvent->key();

            m_keyOwner[key] = m_activeItem;

            emit keyPressed(m_keyCodeMapping.value(key));
            qCInfo(lcInput()) << "Key pressed:" << m_keyCodeMapping.value(key) << m_activeItem;
            break;
        }
        // Release events are always delivered to the component that pressed the key,
        // even if focus has shifted. Uses QPointer for safe nulling if component destroyed.
        case QEvent::KeyRelease: {
            keyEvent = static_cast<QKeyEvent *>(event);
            int key = keyEvent->key();

            QPointer<QObject> owner = m_keyOwner.take(key);
            QPointer<QObject> saved = m_activeItem;
            m_activeItem = owner;
            emit keyReleased(m_keyCodeMapping.value(key));
            m_activeItem = saved;

            qCInfo(lcInput()) << "Key released:" << m_keyCodeMapping.value(key) << m_activeItem;
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
