// The macOS part of Tray: a native NSStatusItem. Built with ARC. See Tray.h.
#include "platform/Tray.h"

#include "platform/TrayBadge.h"

#include <QAction>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QMenu>
#include <QPointer>

#import <AppKit/AppKit.h>

#include <functional>

// Rebuilds the native menu from the QMenu each time it opens, so it follows the
// actions' text and enabled state.
@interface IecTrayMenuDelegate : NSObject <NSMenuDelegate>
- (instancetype)initWithMenu:(QMenu *)menu;
@end

@implementation IecTrayMenuDelegate {
    QPointer<QMenu> _menu;
}

- (instancetype)initWithMenu:(QMenu *)menu
{
    if ((self = [super init]))
        _menu = menu;
    return self;
}

- (void)menuNeedsUpdate:(NSMenu *)nsMenu
{
    [nsMenu removeAllItems];
    if (!_menu)
        return;
    const QList<QAction *> actions = _menu->actions();
    for (qsizetype i = 0; i < actions.size(); ++i) {
        QAction *action = actions[i];
        if (!action->isVisible())
            continue;
        if (action->isSeparator()) {
            [nsMenu addItem:[NSMenuItem separatorItem]];
            continue;
        }
        // Native menus have no mnemonics.
        const QString title = QString(action->text()).remove(QLatin1Char('&'));
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title.toNSString()
                                                      action:@selector(itemClicked:)
                                               keyEquivalent:@""];
        item.target = self;
        item.tag = i;
        item.enabled = action->isEnabled();
        item.state = action->isCheckable() && action->isChecked() ? NSControlStateValueOn : NSControlStateValueOff;
        [nsMenu addItem:item];
    }
}

- (void)itemClicked:(NSMenuItem *)item
{
    if (!_menu)
        return;
    const QList<QAction *> actions = _menu->actions();
    if (item.tag < 0 || item.tag >= actions.size())
        return;
    // Run the action after the menu has finished tracking.
    QMetaObject::invokeMethod(actions[item.tag], &QAction::trigger, Qt::QueuedConnection);
}

@end

@interface IecReopenHandler : NSObject
- (instancetype)initWithCallback:(std::function<void()>)callback;
- (void)handleReopen:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)reply;
@end

@implementation IecReopenHandler {
    std::function<void()> _callback;
}

- (instancetype)initWithCallback:(std::function<void()>)callback
{
    if ((self = [super init]))
        _callback = std::move(callback);
    return self;
}

- (void)handleReopen:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)reply
{
    _callback();
}

@end

namespace {

// The icon as a menu bar image. A template image is drawn by macOS from its
// alpha only, in the menu bar's colour; any other image keeps its colours.
NSImage *toStatusImage(const QIcon &icon, bool isTemplate)
{
    const NSSize size = NSMakeSize(18, 18);
    NSImage *image = [[NSImage alloc] initWithSize:size];
    for (const qreal scale : {1.0, 2.0}) {
        const QImage qimage = icon.pixmap(QSize(18, 18), scale).toImage();
        CGImageRef cgImage = qimage.toCGImage();
        if (!cgImage)
            continue;
        NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
        CGImageRelease(cgImage);
        rep.size = size;
        [image addRepresentation:rep];
    }
    [image setTemplate:isTemplate ? YES : NO];    // "template" is a C++ keyword
    return image;
}

} // namespace

namespace iec::platform {

struct Tray::Private
{
    NSStatusItem *item = nil;
    NSMenu *menu = nil;
    IecTrayMenuDelegate *delegate = nil;
    IecReopenHandler *reopenHandler = nil;
    NSImage *noMail = nil;
    NSImage *unread = nil;
    NSImage *problem = nil;
    State state = State::NoMail;
    int count = 0;
    // The last state asked for, which a delayed change checks it still applies.
    bool dockIconVisible = true;
};

namespace {

void applyActivationPolicy(bool dockIconVisible)
{
    // An accessory app has no Dock icon and no menu bar, like a menu bar app.
    const NSApplicationActivationPolicy policy =
        dockIconVisible ? NSApplicationActivationPolicyRegular : NSApplicationActivationPolicyAccessory;
    if (NSApp.activationPolicy == policy)
        return;
    [NSApp setActivationPolicy:policy];
    // An app that was an accessory does not come to the front by itself.
    if (dockIconVisible) {
        if (@available(macOS 14, *))
            [NSApp activate];
        else
            [NSApp activateIgnoringOtherApps:YES];
    }
}

} // namespace

bool Tray::isAvailable()
{
    // The offscreen platform has no menu bar.
    return QGuiApplication::platformName() == QLatin1String("cocoa");
}

Tray::Tray(const QString &toolTip, QMenu *menu, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    // In the menu bar's colour while nothing is unread, and in colour while
    // something is, so unread mail stands out.
    d->noMail = toStatusImage(QIcon(QStringLiteral(":/icons/tray-mono.svg")), true);
    d->unread = toStatusImage(QIcon(QStringLiteral(":/icons/tray-unread.svg")), false);
    d->problem = toStatusImage(QIcon(QStringLiteral(":/icons/tray-mono-error.svg")), true);
    d->delegate = [[IecTrayMenuDelegate alloc] initWithMenu:menu];
    d->menu = [[NSMenu alloc] init];
    d->menu.autoenablesItems = NO;
    d->menu.delegate = d->delegate;

    d->item = [NSStatusBar.systemStatusBar statusItemWithLength:NSSquareStatusItemLength];
    d->item.button.image = d->noMail;
    d->item.button.toolTip = toolTip.toNSString();
    // Any click opens the menu, as with other menu bar items.
    d->item.menu = d->menu;

    IecReopenHandler *handler = [[IecReopenHandler alloc] initWithCallback:[this] { emit reopened(); }];
    d->reopenHandler = handler;
    // AppKit installs its own reopen handler while launching, so replace it
    // afterwards. The tray is made before the event loop starts.
    __block id observer = [NSNotificationCenter.defaultCenter
        addObserverForName:NSApplicationDidFinishLaunchingNotification
                    object:nil
                     queue:nil
                usingBlock:^(NSNotification *) {
                    [NSNotificationCenter.defaultCenter removeObserver:observer];
                    [NSAppleEventManager.sharedAppleEventManager
                        setEventHandler:handler
                            andSelector:@selector(handleReopen:withReplyEvent:)
                          forEventClass:kCoreEventClass
                             andEventID:kAEReopenApplication];
                }];
}

Tray::~Tray()
{
    [NSAppleEventManager.sharedAppleEventManager removeEventHandlerForEventClass:kCoreEventClass
                                                                      andEventID:kAEReopenApplication];
    [NSStatusBar.systemStatusBar removeStatusItem:d->item];
}

void Tray::setState(State state, int count)
{
    if (state == d->state && count == d->count)
        return;
    d->state = state;
    d->count = count;
    d->item.button.image = state == State::Problem ? d->problem : (count > 0 ? d->unread : d->noMail);
    // A square item has room for the icon only, so the item grows with a count.
    d->item.length = count > 0 ? NSVariableStatusItemLength : NSSquareStatusItemLength;
    d->item.button.imagePosition = count > 0 ? NSImageLeft : NSImageOnly;
    d->item.button.title = count > 0 ? trayBadgeText(count).toNSString() : @"";
}

void Tray::setToolTip(const QString &toolTip) { d->item.button.toolTip = toolTip.toNSString(); }

void Tray::showMessage(const QString &title, const QString &text)
{
    Q_UNUSED(title);
    Q_UNUSED(text);
}

void Tray::setDockIconVisible(bool visible)
{
    d->dockIconVisible = visible;
    if (visible) {
        applyActivationPolicy(true);
        return;
    }
    // On macOS 27, becoming an accessory while AppKit handles the window's
    // close button changes NSApp.activationPolicy but leaves the Dock icon. The
    // same change made once that event is handled removes it.
    QPointer<Tray> tray(this);
    dispatch_async(dispatch_get_main_queue(), ^{
        if (tray && !tray->d->dockIconVisible)
            applyActivationPolicy(false);
    });
}

} // namespace iec::platform
