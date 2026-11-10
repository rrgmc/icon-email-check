#include "platform/Tray.h"

#include "platform/TrayBadge.h"

// macOS has its own implementation in TrayMac.mm.
#if !defined(Q_OS_MACOS)

#include <QCursor>
#include <QIcon>
#include <QMenu>
#include <QPointer>
#include <QSystemTrayIcon>

namespace iec::platform {

struct Tray::Private
{
    QSystemTrayIcon *icon = nullptr;
    QIcon noMail;
    QIcon unread;
    QIcon problem;
    State state = State::NoMail;
    int count = 0;
    QList<QColor> dots;
};

bool Tray::isAvailable() { return QSystemTrayIcon::isSystemTrayAvailable(); }

Tray::Tray(const QString &toolTip, QMenu *menu, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    // A grey outline while nothing is unread, and in colour with the count
    // while something is, so unread mail stands out.
    d->noMail = QIcon(QStringLiteral(":/icons/tray-idle.svg"));
    d->unread = QIcon(QStringLiteral(":/icons/tray.svg"));
    d->problem = QIcon(QStringLiteral(":/icons/tray-error.svg"));
    d->icon = new QSystemTrayIcon(d->noMail, this);
    d->icon->setToolTip(toolTip);
    d->icon->setContextMenu(menu);
    // The system opens the menu on a right click. A left click opens it too,
    // where the cursor is.
    QPointer<QMenu> guard(menu);
    connect(d->icon, &QSystemTrayIcon::activated, this, [guard](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger && guard)
            guard->popup(QCursor::pos());
    });
    connect(d->icon, &QSystemTrayIcon::messageClicked, this, &Tray::messageClicked);
    d->icon->show();
}

Tray::~Tray() = default;

void Tray::setState(State state, int count, const QList<QColor> &dots)
{
    const QList<QColor> shown = dots.mid(0, maxAccountDots);
    if (state == d->state && count == d->count && shown == d->dots)
        return;
    d->state = state;
    d->count = count;
    d->dots = shown;
    const QIcon &base = state == State::Problem ? d->problem : (count > 0 ? d->unread : d->noMail);
    d->icon->setIcon(withTrayBadge(base, count, shown));
}

void Tray::setToolTip(const QString &toolTip) { d->icon->setToolTip(toolTip); }

void Tray::showMessage(const QString &title, const QString &text)
{
    d->icon->showMessage(title, text, QSystemTrayIcon::Information);
}

void Tray::setDockIconVisible(bool visible) { Q_UNUSED(visible); }

} // namespace iec::platform

#endif
