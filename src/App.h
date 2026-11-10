#pragma once

#include "accounts/Account.h"
#include "mail/CheckResult.h"

#include <QObject>
#include <QPointer>
#include <QTimer>

class QMenu;

namespace iec {

class MailMonitor;
class MainWindow;
class Settings;

namespace platform {
class Notifier;
class Tray;
} // namespace platform

// The tray side of the app: the icon and its menu, which follow the unread
// counts, and the notifications for new email.
class App : public QObject
{
    Q_OBJECT

public:
    App(Settings *settings, MailMonitor *monitor, MainWindow *window, QObject *parent = nullptr);
    ~App() override;

    // Makes the tray icon. At sign-in the taskbar may not be ready, so it
    // tries again for a minute; with no tray then, the window is shown, since
    // nothing else could bring it back.
    void startTray();

    bool hasTray() const { return m_tray != nullptr; }

private:
    void updateTray();
    void rebuildMenu();
    void onNewMail(const Account &account, int newCount, const QList<MailSummary> &messages);
    void openInbox(const QString &accountId);

    Settings *m_settings;
    MailMonitor *m_monitor;
    MainWindow *m_window;
    QMenu *m_menu = nullptr;
    platform::Tray *m_tray = nullptr;
    platform::Notifier *m_notifier = nullptr;
    int m_trayAttempts = 0;
    QTimer m_refresh;
};

} // namespace iec
