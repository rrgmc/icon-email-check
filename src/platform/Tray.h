#pragma once

#include <QColor>
#include <QList>
#include <QObject>
#include <QString>

#include <memory>

class QMenu;

namespace iec::platform {

// The icon in the Windows notification area, the macOS menu bar or the Linux
// system tray. Its picture follows the unread count.
//
// The icon is in colour while mail is unread, and plain otherwise. While mail
// is unread, a row of dots along its top shows which accounts have it, in each
// account's colour, up to three.
//
// On macOS it is a native NSStatusItem, in TrayMac.mm. Qt 6.8's
// QSystemTrayIcon aborts on macOS 27 when the icon is clicked: it reads the
// click count of an event that is no longer a mouse event. With nothing
// unread, the status item shows a template image, which macOS draws in the
// menu bar's colour; with unread mail, a blue envelope with the account dots
// and the count as its title. Elsewhere it wraps QSystemTrayIcon, in Tray.cpp: a grey
// outline with nothing unread, and a blue envelope with the count in a red
// badge with unread mail.
class Tray : public QObject
{
    Q_OBJECT

public:
    enum class State {
        NoMail,    // every account was read, and nothing is unread
        Unread,    // count messages are unread
        Problem,   // some account could not be read; count is what the others found
    };

    // False where there is no tray, for example GNOME without an extension, or
    // at sign-in before the taskbar has started.
    static bool isAvailable();

    // A left or a right click opens the menu. The menu must outlive the tray. On macOS the menu is rebuilt from the
    // QMenu's actions each time it opens, so changes to them show.
    Tray(const QString &toolTip, QMenu *menu, QObject *parent = nullptr);
    ~Tray() override;

    // dots: the colours of the accounts with unread mail, in order. The icon
    // shows the first maxAccountDots of them.
    void setState(State state, int count, const QList<QColor> &dots = {});
    void setToolTip(const QString &toolTip);

    // Shows a message from the icon, a notification on Windows and Linux.
    // Does nothing on macOS, where Notifier uses the system's notifications.
    void showMessage(const QString &title, const QString &text);

    // macOS: shows the Dock icon while a window is shown, and removes it while
    // the app lives only in the menu bar. Showing it also brings the app to the
    // front. Does nothing elsewhere.
    void setDockIconVisible(bool visible);

signals:
    // The last message was clicked. Not emitted on macOS.
    void messageClicked();
    // macOS only: the app was opened again from the Finder or the Dock. A
    // second process does not start there, so this is how the window comes
    // back.
    void reopened();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace iec::platform
