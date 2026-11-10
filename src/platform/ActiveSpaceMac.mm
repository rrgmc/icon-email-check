// The macOS implementation of ActiveSpace.h.
#include "platform/ActiveSpace.h"

#import <AppKit/AppKit.h>
#import <objc/runtime.h>

#include <QGuiApplication>
#include <QWidget>

namespace iec::platform {

namespace {

// The key of the associated object that marks a window whose flag is set.
char restoreKey;

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

// Removes MoveToActiveSpace again when the app is no longer active, so that
// later the window stays on its Space like other windows.
void restoreOnResignActive(NSWindow *window)
{
    if (objc_getAssociatedObject(window, &restoreKey))
        return;
    __weak NSWindow *weakWindow = window;
    __block id observer = [NSNotificationCenter.defaultCenter
        addObserverForName:NSApplicationDidResignActiveNotification
                    object:nil
                     queue:nil
                usingBlock:^(NSNotification *) {
                    [NSNotificationCenter.defaultCenter removeObserver:observer];
                    NSWindow *strongWindow = weakWindow;
                    if (!strongWindow)
                        return;
                    strongWindow.collectionBehavior &= ~NSWindowCollectionBehaviorMoveToActiveSpace;
                    objc_setAssociatedObject(strongWindow, &restoreKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                }];
    objc_setAssociatedObject(window, &restoreKey, observer, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

} // namespace

void showOnActiveSpace(const QList<QWidget *> &windows)
{
    // AppKit shows a window on the Space it was on, and activating the app
    // then switches to that Space. With MoveToActiveSpace the window comes to
    // the current Space instead. The flag stays until the app is no longer
    // active, because activation finishes after this returns.
    for (QWidget *widget : windows) {
        NSWindow *window = nativeWindow(widget);
        if (!window)
            continue;
        const NSWindowCollectionBehavior behavior = window.collectionBehavior;
        if (behavior & NSWindowCollectionBehaviorCanJoinAllSpaces)
            continue;
        if (window.visible && window.onActiveSpace)
            continue;
        if (!(behavior & NSWindowCollectionBehaviorMoveToActiveSpace)) {
            window.collectionBehavior = behavior | NSWindowCollectionBehaviorMoveToActiveSpace;
            restoreOnResignActive(window);
        }
        // A window on screen on another Space moves only when it is ordered
        // in again.
        if (window.visible) {
            [window orderOut:nil];
            [window orderFront:nil];
        }
    }
}

} // namespace iec::platform
