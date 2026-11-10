#include "platform/Tray.h"

#include "platform/TrayBadge.h"

// macOS has its own implementation in TrayMac.mm.
#if !defined(Q_OS_MACOS)

#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>

namespace iec::platform {

struct Tray::Private
{
    QSystemTrayIcon *icon = nullptr;
    QIcon normal;
    QIcon problem;
    State state = State::NoMail;
    int count = 0;
};

bool Tray::isAvailable() { return QSystemTrayIcon::isSystemTrayAvailable(); }

Tray::Tray(const QString &toolTip, QMenu *menu, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Private>())
{
    d->normal = QIcon(QStringLiteral(":/icons/tray.svg"));
    d->problem = QIcon(QStringLiteral(":/icons/tray-error.svg"));
    d->icon = new QSystemTrayIcon(d->normal, this);
    d->icon->setToolTip(toolTip);
    d->icon->setContextMenu(menu);
    connect(d->icon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            emit triggered();
    });
    connect(d->icon, &QSystemTrayIcon::messageClicked, this, &Tray::messageClicked);
    d->icon->show();
}

Tray::~Tray() = default;

void Tray::setState(State state, int count)
{
    if (state == d->state && count == d->count)
        return;
    d->state = state;
    d->count = count;
    const QIcon &base = state == State::Problem ? d->problem : d->normal;
    d->icon->setIcon(withTrayBadge(base, count));
}

void Tray::setToolTip(const QString &toolTip) { d->icon->setToolTip(toolTip); }

void Tray::showMessage(const QString &title, const QString &text)
{
    d->icon->showMessage(title, text, QSystemTrayIcon::Information);
}

void Tray::setDockIconVisible(bool visible) { Q_UNUSED(visible); }

} // namespace iec::platform

#endif
