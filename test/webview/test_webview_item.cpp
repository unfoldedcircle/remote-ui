// Copyright (c) 2026 Unfolded Circle ApS and/or its affiliates. <hello@unfoldedcircle.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QQuickWindow>
#include <QtTest>

#include "webview/webviewitem.h"
#include "webview/webviewlibrary.h"

using uc::webview::WebViewFunctions;
using uc::webview::WebViewItem;
using uc::webview::WebViewLibrary;

// A fake libucwebview: records every call and lets the test fire the callbacks the real library would deliver
// from the GLib main context on the GUI thread.
namespace stub {

struct State {
    int                  initCalls = 0;
    void                *eglDisplay = nullptr;
    uc_webview          *view = nullptr;  // the one live view (the test creates one item at a time)
    int                  width = 0;
    int                  height = 0;
    uc_webview_callbacks callbacks{};
    void                *user = nullptr;
    QStringList          calls;  // "name(args)" in order
    bool                 canGoBack = false;
    bool                 canGoForward = false;
};

State s;

uc_webview *const kView = reinterpret_cast<uc_webview *>(0x1234);

void record(const QString &call) {
    s.calls.append(call);
}

int apiVersion() {
    return UC_WEBVIEW_API_VERSION;
}

int globalInit(void *egl, const char *, const char *) {
    ++s.initCalls;
    s.eglDisplay = egl;
    return 0;
}

void globalShutdown() {}

uc_webview *create(int w, int h, const uc_webview_callbacks *cb, void *user) {
    s.view = kView;
    s.width = w;
    s.height = h;
    s.callbacks = *cb;
    s.user = user;
    record(QStringLiteral("new(%1x%2)").arg(w).arg(h));
    return kView;
}

void destroy(uc_webview *v) {
    QCOMPARE(v, kView);
    s.view = nullptr;
    record(QStringLiteral("free"));
}

void resize(uc_webview *, int w, int h) {
    s.width = w;
    s.height = h;
    record(QStringLiteral("resize(%1x%2)").arg(w).arg(h));
}

void setMapped(uc_webview *, int mapped) {
    record(QStringLiteral("set_mapped(%1)").arg(mapped));
}

void setFocused(uc_webview *, int focused) {
    record(QStringLiteral("set_focused(%1)").arg(focused));
}

void setScale(uc_webview *, double scale) {
    record(QStringLiteral("set_scale(%1)").arg(scale));
}

void loadUri(uc_webview *, const char *uri) {
    record(QStringLiteral("load_uri(%1)").arg(QString::fromUtf8(uri)));
}

void loadHtml(uc_webview *, const char *html, const char *base) {
    record(QStringLiteral("load_html(%1,%2)").arg(QString::fromUtf8(html), base ? QString::fromUtf8(base) : "null"));
}

void reload(uc_webview *) {
    record(QStringLiteral("reload"));
}

void stop(uc_webview *) {
    record(QStringLiteral("stop"));
}

void goBack(uc_webview *) {
    record(QStringLiteral("go_back"));
}

void goForward(uc_webview *) {
    record(QStringLiteral("go_forward"));
}

int canGoBack(uc_webview *) {
    return s.canGoBack ? 1 : 0;
}

int canGoForward(uc_webview *) {
    return s.canGoForward ? 1 : 0;
}

const char *getUri(uc_webview *) {
    return nullptr;
}

const char *getTitle(uc_webview *) {
    return nullptr;
}

void runJavascript(uc_webview *, const char *script) {
    record(QStringLiteral("run_javascript(%1)").arg(QString::fromUtf8(script)));
}

void setZoom(uc_webview *, double) {}

void setBackground(uc_webview *, float r, float g, float b, float a) {
    record(QStringLiteral("set_background_rgba(%1,%2,%3,%4)").arg(r).arg(g).arg(b).arg(a));
}

void frameRendered(uc_webview *, void *handle) {
    record(QStringLiteral("frame_rendered(%1)").arg(reinterpret_cast<quintptr>(handle)));
}

void frameRelease(uc_webview *, void *handle) {
    record(QStringLiteral("frame_release(%1)").arg(reinterpret_cast<quintptr>(handle)));
}

void touch(uc_webview *, int id, int phase, double x, double y, uint32_t) {
    record(QStringLiteral("touch(%1,%2,%3,%4)").arg(id).arg(phase).arg(x).arg(y));
}

void pointerButton(uc_webview *, int button, int pressed, double x, double y, unsigned, uint32_t) {
    record(QStringLiteral("pointer_button(%1,%2,%3,%4)").arg(button).arg(pressed).arg(x).arg(y));
}

void pointerMove(uc_webview *, double x, double y, unsigned, uint32_t) {
    record(QStringLiteral("pointer_move(%1,%2)").arg(x).arg(y));
}

void scroll(uc_webview *, double dx, double dy, double, double, int precise, uint32_t) {
    record(QStringLiteral("scroll(%1,%2,%3)").arg(dx).arg(dy).arg(precise));
}

void key(uc_webview *, uint32_t keysym, uint32_t, int pressed, unsigned mods, uint32_t) {
    record(QStringLiteral("key(0x%1,%2,%3)").arg(keysym, 0, 16).arg(pressed).arg(mods));
}

void imeCommit(uc_webview *, const char *text) {
    record(QStringLiteral("ime_commit(%1)").arg(QString::fromUtf8(text)));
}

void imePreedit(uc_webview *, const char *text, int cursor) {
    record(QStringLiteral("ime_preedit(%1,%2)").arg(QString::fromUtf8(text)).arg(cursor));
}

void imeDeleteSurrounding(uc_webview *, int offset, int n) {
    record(QStringLiteral("ime_delete_surrounding(%1,%2)").arg(offset).arg(n));
}

WebViewFunctions functions() {
    WebViewFunctions f;
    f.api_version = &apiVersion;
    f.global_init = &globalInit;
    f.global_shutdown = &globalShutdown;
    f.create = &create;
    f.destroy = &destroy;
    f.resize = &resize;
    f.set_mapped = &setMapped;
    f.set_focused = &setFocused;
    f.set_scale = &setScale;
    f.load_uri = &loadUri;
    f.load_html = &loadHtml;
    f.reload = &reload;
    f.stop = &stop;
    f.go_back = &goBack;
    f.go_forward = &goForward;
    f.can_go_back = &canGoBack;
    f.can_go_forward = &canGoForward;
    f.get_uri = &getUri;
    f.get_title = &getTitle;
    f.run_javascript = &runJavascript;
    f.set_zoom = &setZoom;
    f.set_background_rgba = &setBackground;
    f.frame_rendered = &frameRendered;
    f.frame_release = &frameRelease;
    f.touch = &touch;
    f.pointer_button = &pointerButton;
    f.pointer_move = &pointerMove;
    f.scroll = &scroll;
    f.key = &key;
    f.ime_commit = &imeCommit;
    f.ime_preedit = &imePreedit;
    f.ime_delete_surrounding = &imeDeleteSurrounding;
    return f;
}

// the library's callbacks, as the GLib main context would deliver them
void fireLoadChanged(int event, const char *uri) {
    s.callbacks.load_changed(s.view, event, uri, s.user);
}

void fireTitle(const char *title) {
    s.callbacks.title_changed(s.view, title, s.user);
}

void fireProgress(double progress) {
    s.callbacks.progress_changed(s.view, progress, s.user);
}

void fireTerminated(int reason) {
    s.callbacks.web_process_terminated(s.view, reason, s.user);
}

void fireImeFocus(int focused, unsigned hints, unsigned purpose, const char *surrounding, int cursor) {
    s.callbacks.ime_focus(s.view, focused, hints, purpose, surrounding, cursor, s.user);
}

void fireImeCursorArea(int x, int y, int w, int h) {
    s.callbacks.ime_cursor_area(s.view, x, y, w, h, s.user);
}

void fireImeSurrounding(const char *text, int cursor, int anchor) {
    s.callbacks.ime_surrounding(s.view, text, cursor, anchor, s.user);
}

void fireFrame(int number) {
    uc_webview_frame frame{};
    frame.width = s.width;
    frame.height = s.height;
    frame.handle = reinterpret_cast<void *>(static_cast<intptr_t>(number));
    s.callbacks.frame_ready(s.view, &frame, s.user);
}

}  // namespace stub

// A WebView item inside a (never shown) window, like the QML page creates it. The window is never exposed, so the
// scene graph does not render and updatePaintNode() is not called: the test covers the GUI thread side.
struct Fixture {
    QQuickWindow window;
    WebViewItem *item;

    Fixture() {
        stub::s = stub::State();
        WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
        item = new WebViewItem(window.contentItem());
        item->setSize(QSizeF(300, 200));
    }

    ~Fixture() { delete item; }

    void sendKey(QEvent::Type type, int key, const QString &text = QString(),
                 Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QKeyEvent event(type, key, modifiers, text);
        event.ignore();
        QCoreApplication::sendEvent(item, &event);
        lastAccepted = event.isAccepted();
    }

    bool lastAccepted = false;
};

class testWebViewItem : public QObject {
    Q_OBJECT

 private slots:
    void realLibraryIsAbsentOnTheHost();
    void availableWithTheFunctionTable();
    void viewIsCreatedInDevicePixelsOnceItHasWindowAndSize();
    void pendingUrlIsLoadedWhenTheViewExists();
    void loadEventsDriveTheProperties();
    void loadFailureIsSignalled();
    void crashReloadsOnceUntilALoadFinishes();
    void imeFocusEnablesTheInputMethodWithTheMappedHints();
    void inputMethodEventsReachTheLibrary();
    void pageKeysAreForwardedRemoteButtonsAreNot();
    void framesArriveAndAreReleasedWhenTheItemGoes();
    void destructorFreesTheView();
};

void testWebViewItem::realLibraryIsAbsentOnTheHost() {
    // runs first, before a Fixture installs the stub table: the real probe on a build host without
    // libucwebview.so.1 (or without Qt's GLib dispatcher, as with the offscreen platform) reports why
    QVERIFY(!WebViewLibrary::instance()->isAvailable());
    QVERIFY(!WebViewLibrary::instance()->unavailableReason().isEmpty());
    QVERIFY(!WebViewLibrary::instance()->ensureInitialized());

    // an item created without the library is a placeholder: no view, no library calls, events are ignored
    QQuickWindow window;
    WebViewItem  item(window.contentItem());
    item.setSize(QSizeF(100, 100));
    QVERIFY(!item.isAvailable());
    QCOMPARE(item.inputMethodQuery(Qt::ImEnabled).toBool(), false);
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
    key.ignore();
    QCoreApplication::sendEvent(&item, &key);
    QVERIFY(!key.isAccepted());
    item.setUrl(QUrl(QStringLiteral("http://localhost/")));
    item.reload();
    QCOMPARE(item.url(), QUrl(QStringLiteral("http://localhost/")));
}

void testWebViewItem::availableWithTheFunctionTable() {
    Fixture f;
    QVERIFY(f.item->isAvailable());
    QVERIFY(WebViewLibrary::instance()->isAvailable());
}

void testWebViewItem::viewIsCreatedInDevicePixelsOnceItHasWindowAndSize() {
    Fixture f;
    QCOMPARE(stub::s.initCalls, 1);
    QVERIFY(stub::s.view != nullptr);
    const qreal ratio = f.window.effectiveDevicePixelRatio();
    QCOMPARE(stub::s.width, qRound(300 * ratio));
    QCOMPARE(stub::s.height, qRound(200 * ratio));
    QVERIFY(stub::s.calls.contains(QStringLiteral("set_scale(%1)").arg(ratio)));
    // the window is not shown: the page is told it is hidden
    QVERIFY(stub::s.calls.contains(QStringLiteral("set_mapped(0)")));

    f.item->setSize(QSizeF(100, 50));
    QCOMPARE(stub::s.calls.last(), QStringLiteral("resize(%1x%2)").arg(qRound(100 * ratio)).arg(qRound(50 * ratio)));
}

void testWebViewItem::pendingUrlIsLoadedWhenTheViewExists() {
    stub::s = stub::State();
    WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
    QQuickWindow window;

    // url set before the item is in a scene: remembered and loaded with the view
    auto *item = new WebViewItem();
    item->setUrl(QUrl(QStringLiteral("http://localhost/")));
    QVERIFY(stub::s.view == nullptr);
    item->setSize(QSizeF(300, 200));
    item->setParentItem(window.contentItem());
    QVERIFY(stub::s.view != nullptr);
    QVERIFY(stub::s.calls.contains(QStringLiteral("load_uri(http://localhost/)")));

    QSignalSpy urlSpy(item, &WebViewItem::urlChanged);
    item->setUrl(QUrl(QStringLiteral("http://example.com/a")));
    QCOMPARE(urlSpy.count(), 1);
    QCOMPARE(stub::s.calls.last(), QStringLiteral("load_uri(http://example.com/a)"));

    item->loadHtml(QStringLiteral("<p>hi</p>"), QUrl(QStringLiteral("http://base/")));
    QCOMPARE(stub::s.calls.last(), QStringLiteral("load_html(<p>hi</p>,http://base/)"));
    item->runJavaScript(QStringLiteral("1+1"));
    QCOMPARE(stub::s.calls.last(), QStringLiteral("run_javascript(1+1)"));
    item->reload();
    QCOMPARE(stub::s.calls.last(), QStringLiteral("reload"));
    delete item;
}

void testWebViewItem::loadEventsDriveTheProperties() {
    Fixture    f;
    QSignalSpy loading(f.item, &WebViewItem::loadingChanged);
    QSignalSpy history(f.item, &WebViewItem::navigationHistoryChanged);

    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://localhost/");
    QVERIFY(f.item->isLoading());
    QCOMPARE(f.item->url(), QUrl(QStringLiteral("http://localhost/")));
    QCOMPARE(loading.count(), 1);

    stub::fireProgress(0.5);
    QCOMPARE(f.item->loadProgress(), 50);
    stub::fireTitle("Welcome");
    QCOMPARE(f.item->title(), QStringLiteral("Welcome"));

    stub::fireLoadChanged(UC_WEBVIEW_LOAD_REDIRECTED, "http://localhost/index.html");
    QCOMPARE(f.item->url(), QUrl(QStringLiteral("http://localhost/index.html")));

    stub::s.canGoBack = true;
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_COMMITTED, "http://localhost/index.html");
    QVERIFY(f.item->canGoBack());
    QVERIFY(!f.item->canGoForward());
    QCOMPARE(history.count(), 1);

    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://localhost/index.html");
    QVERIFY(!f.item->isLoading());
    QCOMPARE(loading.count(), 2);
}

void testWebViewItem::loadFailureIsSignalled() {
    Fixture    f;
    QSignalSpy failed(f.item, &WebViewItem::loadFailed);

    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://nowhere/");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FAILED, "http://nowhere/");
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().first().toUrl(), QUrl(QStringLiteral("http://nowhere/")));
    // loading stays true until the FINISHED event that follows a failure
    QVERIFY(f.item->isLoading());
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://nowhere/");
    QVERIFY(!f.item->isLoading());
}

void testWebViewItem::crashReloadsOnceUntilALoadFinishes() {
    Fixture    f;
    QSignalSpy crashed(f.item, &WebViewItem::crashed);

    stub::fireTerminated(UC_WEBVIEW_TERMINATED_CRASHED);
    QCOMPARE(crashed.count(), 1);
    QCOMPARE(crashed.first().first().toInt(), UC_WEBVIEW_TERMINATED_CRASHED);
    // the reload is deferred out of the callback
    QCOMPARE(stub::s.calls.count(QStringLiteral("reload")), 0);
    QTRY_COMPARE(stub::s.calls.count(QStringLiteral("reload")), 1);

    // a second crash without a completed load in between is not reloaded again
    stub::fireTerminated(UC_WEBVIEW_TERMINATED_EXCEEDED_MEMORY_LIMIT);
    QCOMPARE(crashed.count(), 2);
    QTest::qWait(10);
    QCOMPARE(stub::s.calls.count(QStringLiteral("reload")), 1);

    // a completed load re-arms the automatic reload
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://localhost/");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://localhost/");
    stub::fireTerminated(UC_WEBVIEW_TERMINATED_CRASHED);
    QTRY_COMPARE(stub::s.calls.count(QStringLiteral("reload")), 2);

    // a failed load does not
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_STARTED, "http://localhost/");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FAILED, "http://localhost/");
    stub::fireLoadChanged(UC_WEBVIEW_LOAD_FINISHED, "http://localhost/");
    stub::fireTerminated(UC_WEBVIEW_TERMINATED_CRASHED);
    QTest::qWait(10);
    QCOMPARE(stub::s.calls.count(QStringLiteral("reload")), 2);
}

void testWebViewItem::imeFocusEnablesTheInputMethodWithTheMappedHints() {
    Fixture f;
    QCOMPARE(f.item->inputMethodQuery(Qt::ImEnabled).toBool(), false);

    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_DIGITS, "12", 2);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImEnabled).toBool(), true);
    QCOMPARE(Qt::InputMethodHints(f.item->inputMethodQuery(Qt::ImHints).toInt()),
             Qt::InputMethodHints(Qt::ImhDigitsOnly));
    QCOMPARE(f.item->inputMethodQuery(Qt::ImSurroundingText).toString(), QStringLiteral("12"));
    QCOMPARE(f.item->inputMethodQuery(Qt::ImCursorPosition).toInt(), 2);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImAnchorPosition).toInt(), 2);
    // the page's editable element has the keyboard focus of the item
    QVERIFY(f.item->hasFocus());

    // the caret rectangle comes in view pixels and is answered in item coordinates
    const qreal ratio = f.window.effectiveDevicePixelRatio();
    stub::fireImeCursorArea(qRound(20 * ratio), qRound(40 * ratio), qRound(2 * ratio), qRound(16 * ratio));
    QCOMPARE(f.item->inputMethodQuery(Qt::ImCursorRectangle).toRectF(), QRectF(20, 40, 2, 16));

    stub::fireImeSurrounding("123", 3, 1);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImSurroundingText).toString(), QStringLiteral("123"));
    QCOMPARE(f.item->inputMethodQuery(Qt::ImCursorPosition).toInt(), 3);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImAnchorPosition).toInt(), 1);

    // hints change while focused: a password field now
    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_PASSWORD, nullptr, -1);
    QVERIFY(Qt::InputMethodHints(f.item->inputMethodQuery(Qt::ImHints).toInt()) & Qt::ImhHiddenText);

    stub::fireImeFocus(0, 0, 0, nullptr, -1);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImEnabled).toBool(), false);
    QCOMPARE(f.item->inputMethodQuery(Qt::ImSurroundingText).toString(), QString());
}

void testWebViewItem::inputMethodEventsReachTheLibrary() {
    Fixture f;

    // without a focused editable element the keyboard's events are not forwarded
    {
        QInputMethodEvent commit;
        commit.setCommitString(QStringLiteral("x"));
        QCoreApplication::sendEvent(f.item, &commit);
        QVERIFY(!stub::s.calls.contains(QStringLiteral("ime_commit(x)")));
    }

    stub::fireImeFocus(1, UC_WEBVIEW_IME_HINT_NONE, UC_WEBVIEW_IME_PURPOSE_FREE_FORM, "", 0);
    const int before = stub::s.calls.size();

    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 1, 1, QVariant()));
    QInputMethodEvent preedit(QStringLiteral("ab"), attributes);
    QCoreApplication::sendEvent(f.item, &preedit);
    QCOMPARE(stub::s.calls.mid(before), QStringList{QStringLiteral("ime_preedit(ab,1)")});

    QInputMethodEvent commit;
    commit.setCommitString(QStringLiteral("abc"));
    QCoreApplication::sendEvent(f.item, &commit);
    QCOMPARE(stub::s.calls.mid(before + 1), QStringList{QStringLiteral("ime_commit(abc)")});

    // an empty event after a pre-edit ends it; after a commit there is nothing to end
    QInputMethodEvent empty;
    QCoreApplication::sendEvent(f.item, &empty);
    QCOMPARE(stub::s.calls.size(), before + 2);

    QInputMethodEvent preeditAgain(QStringLiteral("z"), QList<QInputMethodEvent::Attribute>());
    QCoreApplication::sendEvent(f.item, &preeditAgain);
    QCoreApplication::sendEvent(f.item, &empty);
    QCOMPARE(stub::s.calls.mid(before + 2),
             (QStringList{QStringLiteral("ime_preedit(z,1)"), QStringLiteral("ime_preedit(,0)")}));

    // word replacement: delete around the caret, then commit
    QInputMethodEvent replace;
    replace.setCommitString(QStringLiteral("word"), -3, 3);
    QCoreApplication::sendEvent(f.item, &replace);
    QCOMPARE(stub::s.calls.mid(before + 4),
             (QStringList{QStringLiteral("ime_delete_surrounding(-3,3)"), QStringLiteral("ime_commit(word)")}));
}

void testWebViewItem::pageKeysAreForwardedRemoteButtonsAreNot() {
    Fixture   f;
    const int before = stub::s.calls.size();

    f.sendKey(QEvent::KeyPress, Qt::Key_Return, QStringLiteral("\r"));
    QVERIFY(f.lastAccepted);
    f.sendKey(QEvent::KeyRelease, Qt::Key_Return, QStringLiteral("\r"));
    QVERIFY(f.lastAccepted);
    f.sendKey(QEvent::KeyPress, Qt::Key_A, QStringLiteral("A"), Qt::ShiftModifier);
    QVERIFY(f.lastAccepted);
    QCOMPARE(stub::s.calls.mid(before),
             (QStringList{QStringLiteral("key(0xff0d,1,0)"), QStringLiteral("key(0xff0d,0,0)"),
                          QStringLiteral("key(0x41,1,%1)").arg(UC_WEBVIEW_MOD_SHIFT)}));

    // BACK and HOME of the remote are ignored, so the QML page gets them
    f.sendKey(QEvent::KeyPress, Qt::Key_Exit);
    QVERIFY(!f.lastAccepted);
    f.sendKey(QEvent::KeyPress, Qt::Key_Home);
    QVERIFY(!f.lastAccepted);
    f.sendKey(QEvent::KeyPress, Qt::Key_VolumeUp);
    QVERIFY(!f.lastAccepted);
    QCOMPARE(stub::s.calls.size(), before + 3);
}

void testWebViewItem::framesArriveAndAreReleasedWhenTheItemGoes() {
    stub::s = stub::State();
    WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
    QQuickWindow window;
    auto        *item = new WebViewItem(window.contentItem());
    item->setSize(QSizeF(300, 200));
    QSignalSpy stats(item, &WebViewItem::frameStatsChanged);

    stub::fireFrame(1);
    QCOMPARE(item->framesReceived(), 1);
    QCOMPARE(item->framesReleased(), 0);
    QCOMPARE(stats.count(), 1);

    // the window never renders: the second frame replaces the first, which goes straight back
    stub::fireFrame(2);
    QCOMPARE(item->framesReceived(), 2);
    QCOMPARE(item->framesReleased(), 1);
    QVERIFY(stub::s.calls.contains(QStringLiteral("frame_release(1)")));

    // the item is destroyed with a frame in hand: released before the view is freed
    delete item;
    const int release = stub::s.calls.indexOf(QStringLiteral("frame_release(2)"));
    const int free = stub::s.calls.indexOf(QStringLiteral("free"));
    QVERIFY(release >= 0);
    QVERIFY(free > release);
}

void testWebViewItem::destructorFreesTheView() {
    stub::s = stub::State();
    WebViewLibrary::instance()->setFunctionsForTest(stub::functions());
    {
        QQuickWindow window;
        auto        *item = new WebViewItem(window.contentItem());
        item->setSize(QSizeF(10, 10));
        QVERIFY(stub::s.view != nullptr);
        delete item;
    }
    QVERIFY(stub::s.view == nullptr);
    QCOMPARE(stub::s.calls.count(QStringLiteral("free")), 1);
}

QTEST_MAIN(testWebViewItem)
#include "test_webview_item.moc"
