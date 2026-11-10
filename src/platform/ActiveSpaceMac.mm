// The macOS implementation of ActiveSpace.h.
#include "platform/ActiveSpace.h"

#import <AppKit/AppKit.h>

#include <QGuiApplication>
#include <QPointer>
#include <QWidget>

namespace iec::platform {

namespace {

NSWindow *nativeWindow(QWidget *widget)
{
    // Only the Cocoa platform has an NSView behind winId(). Under another
    // platform, such as the offscreen one the tests use, it is not a pointer.
    if (QGuiApplication::platformName() != QLatin1String("cocoa"))
        return nil;
    if (!widget || !widget->testAttribute(Qt::WA_WState_Created))
        return nil;
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(widget->winId());
    return view ? view.window : nil;
}

} // namespace

void showOnActiveSpace(const QList<QWidget *> &windows)
{
    // AppKit shows a hidden window on the Space it was on, and activating the
    // app then switches to that Space. With MoveToActiveSpace the window comes
    // to the current Space instead. The flag is removed once the window is on
    // screen, so that later the window stays on its Space like other windows.
    for (QWidget *widget : windows) {
        NSWindow *window = nativeWindow(widget);
        if (!window)
            continue;
        const NSWindowCollectionBehavior before = window.collectionBehavior;
        if (before & (NSWindowCollectionBehaviorMoveToActiveSpace | NSWindowCollectionBehaviorCanJoinAllSpaces))
            continue;
        window.collectionBehavior = before | NSWindowCollectionBehaviorMoveToActiveSpace;
        QPointer<QWidget> guard(widget);
        dispatch_async(dispatch_get_main_queue(), ^{
            if (guard && nativeWindow(guard) == window)
                window.collectionBehavior = before;
        });
    }
}

} // namespace iec::platform
