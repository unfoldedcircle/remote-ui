// Copyright (c) 2022-2023 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "inputController.h"

#include "../logging.h"

namespace uc {
namespace ui {

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

void InputController::takeControl(const QString &activeObject) {
    m_prevActiveObject = m_activeObject;
    m_activeObject = activeObject;

    // we need to delay this a bit, otherwise it happens so fast that both old and new objects will trigger
    QTimer::singleShot(200, [=] {
        emit activeObjectChanged();
        qCDebug(lcInput()) << "TAKE:" << m_prevActiveObject << "->" << m_activeObject;
    });
}

void InputController::releaseControl(const QString &activeObject) {
    if (activeObject.isEmpty()) {
        m_activeObject = m_prevActiveObject;
    } else {
        m_activeObject = activeObject;
    }

    // we need to delay this a bit, otherwise it happens so fast that both old and new objects will trigger
    QTimer::singleShot(200, [=] {
        emit activeObjectChanged();
        qCDebug(lcInput()) << "RELEASE ->" << m_activeObject;
    });
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

            m_longPressTriggered.insert(key, false);

            QTimer *timer = new QTimer(this);
            timer->setSingleShot(true);
            timer->setInterval(m_longPressTimeOut);

            QObject::connect(timer, &QTimer::timeout, this, [=]() {
                qCDebug(lcInput()) << "Key press and hold:" << m_keyCodeMapping.value(key);
                m_longPressTriggered.insert(key, true);
                emit keyLongPressed(m_keyCodeMapping.value(key));
            });
            timer->start();
            m_longPressTimers.insert(key, timer);

            emit keyPressed(m_keyCodeMapping.value(key));
            qCDebug(lcInput()) << "Key pressed:" << m_keyCodeMapping.value(key);
            break;
        }
        case QEvent::KeyRelease: {
            keyEvent = static_cast<QKeyEvent *>(event);
            int key = keyEvent->key();

            if (m_longPressTimers.contains(key)) {
                QTimer *timer = m_longPressTimers.value(key);
                if (timer) {
                    timer->stop();
                    timer->deleteLater();
                    m_longPressTimers.remove(key);
                }
            }

            //            if (!m_longPressTriggered.value(key)) {
            emit keyReleased(m_keyCodeMapping.value(key));
            qCDebug(lcInput()) << "Key released:" << m_keyCodeMapping.value(key);
            //            }
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

}  // namespace ui
}  // namespace uc
