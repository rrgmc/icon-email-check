// The macOS part of Notifier: the User Notifications framework. Built with ARC.
// See Notifier.h.
#include "platform/Notifier.h"

#include <QPointer>
#include <QtGlobal>

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

#include <functional>

// Shows notifications while Iec is in front too, and reports clicks.
@interface IecNotificationDelegate : NSObject <UNUserNotificationCenterDelegate>
- (instancetype)initWithCallback:(std::function<void(NSString *)>)callback;
@end

@implementation IecNotificationDelegate {
    std::function<void(NSString *)> _callback;
}

- (instancetype)initWithCallback:(std::function<void(NSString *)>)callback
{
    if ((self = [super init]))
        _callback = std::move(callback);
    return self;
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler
{
    completionHandler(UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList);
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
    didReceiveNotificationResponse:(UNNotificationResponse *)response
             withCompletionHandler:(void (^)(void))completionHandler
{
    if ([response.actionIdentifier isEqualToString:UNNotificationDefaultActionIdentifier]) {
        id value = response.notification.request.content.userInfo[@"id"];
        _callback([value isKindOfClass:[NSString class]] ? (NSString *)value : @"");
    }
    completionHandler();
}

@end

namespace {

// UNUserNotificationCenter raises an exception in a process without a bundle
// identifier, such as the executable started from outside its bundle.
bool hasBundle() { return NSBundle.mainBundle.bundleIdentifier != nil; }

} // namespace

namespace iec::platform {

struct Notifier::Private
{
    IecNotificationDelegate *delegate = nil;
};

Notifier::Notifier(Tray *tray, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    Q_UNUSED(tray);
    if (!hasBundle())
        return;
    QPointer<Notifier> self(this);
    d->delegate = [[IecNotificationDelegate alloc] initWithCallback:[self](NSString *id) {
        const QString itemId = QString::fromNSString(id);
        // The delegate is called on the main queue, so the signal is queued
        // only to leave AppKit's handler first.
        QMetaObject::invokeMethod(
            self.data(),
            [self, itemId] {
                if (self)
                    emit self->clicked(itemId);
            },
            Qt::QueuedConnection);
    }];
    UNUserNotificationCenter *center = UNUserNotificationCenter.currentNotificationCenter;
    center.delegate = d->delegate;
    [center requestAuthorizationWithOptions:UNAuthorizationOptionAlert | UNAuthorizationOptionSound
                          completionHandler:^(BOOL granted, NSError *error) {
                              if (error)
                                  qWarning("Notifications are not allowed: %s",
                                           error.localizedDescription.UTF8String);
                              else if (!granted)
                                  qInfo("Notifications are turned off for this app in System Settings.");
                          }];
}

Notifier::~Notifier()
{
    if (!hasBundle())
        return;
    UNUserNotificationCenter *center = UNUserNotificationCenter.currentNotificationCenter;
    if (center.delegate == d->delegate)
        center.delegate = nil;
}

void Notifier::show(const QString &title, const QString &text, const QString &id)
{
    if (!hasBundle())
        return;
    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
    content.title = title.toNSString();
    content.body = text.toNSString();
    content.userInfo = @{@"id" : id.toNSString()};
    UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:NSUUID.UUID.UUIDString
                                                                          content:content
                                                                          trigger:nil];
    // Without permission the system drops the request.
    [UNUserNotificationCenter.currentNotificationCenter addNotificationRequest:request
                                                         withCompletionHandler:^(NSError *error) {
                                                             if (error)
                                                                 qWarning("A notification was not shown: %s",
                                                                          error.localizedDescription.UTF8String);
                                                         }];
}

} // namespace iec::platform
