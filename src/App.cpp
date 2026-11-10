#include "App.h"

#include "Settings.h"
#include "mail/MailMonitor.h"
#include "platform/Notifier.h"
#include "platform/Tray.h"
#include "ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QMenu>
#include <QTimer>
#include <QUrl>

namespace iec {

namespace {

// Menu entries are one line, so long subjects are cut.
QString elide(const QString &text, int length = 60)
{
    const QString simple = text.simplified();
    return simple.size() <= length ? simple : simple.left(length - 1) + QChar(0x2026);
}

QString messageLine(const MailSummary &m)
{
    const QString subject = m.subject.isEmpty() ? App::tr("(no subject)") : m.subject;
    return m.from.isEmpty() ? subject : QStringLiteral("%1: %2").arg(m.from, subject);
}

// QMenu treats & as a mnemonic; a literal one is written &&.
QString menuText(const QString &text) { return QString(text).replace(QLatin1Char('&'), QLatin1String("&&")); }

} // namespace

App::App(Settings *settings, MailMonitor *monitor, MainWindow *window, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_monitor(monitor)
    , m_window(window)
{
    m_menu = new QMenu;
    rebuildMenu();
    // Rebuilt from the event loop: a menu action can change the state, and
    // the rebuild would delete that action while it is still triggering.
    m_refresh.setSingleShot(true);
    m_refresh.setInterval(0);
    connect(&m_refresh, &QTimer::timeout, this, [this] {
        rebuildMenu();
        updateTray();
    });
    connect(m_monitor, &MailMonitor::changed, &m_refresh, qOverload<>(&QTimer::start));
    connect(m_monitor, &MailMonitor::newMail, this, &App::onNewMail);
}

App::~App()
{
    delete m_tray;
    delete m_menu;
}

void App::startTray()
{
    if (!platform::Tray::isAvailable()) {
        if (++m_trayAttempts < 30)
            QTimer::singleShot(2000, this, &App::startTray);
        else if (!m_window->isVisible())
            m_window->showAndRaise();
        return;
    }
    m_tray = new platform::Tray(QApplication::applicationDisplayName(), m_menu, this);
    connect(m_tray, &platform::Tray::reopened, m_window, &MainWindow::showAndRaise);
    m_notifier = new platform::Notifier(m_tray, this);
    connect(m_notifier, &platform::Notifier::clicked, this, &App::openInbox);
    m_window->setTray(m_tray);
    // An app started hidden shows no Dock icon on macOS.
    if (!m_window->isVisible())
        m_tray->setDockIconVisible(false);
    updateTray();
}

void App::rebuildMenu()
{
    m_menu->clear();
    const QList<Account> &accounts = m_monitor->accounts();
    bool any = false;
    for (const Account &account : accounts) {
        if (!account.enabled)
            continue;
        any = true;
        const AccountState state = m_monitor->state(account.id);
        QString label;
        if (state.checkedOnce && !state.last.ok)
            label = tr("%1: cannot check").arg(account.displayName());
        else if (!state.lastGood.isValid())
            label = tr("%1: checking…").arg(account.displayName());
        else if (state.unread == 0)
            label = tr("%1: no unread email").arg(account.displayName());
        else
            label = tr("%1: %n unread", nullptr, state.unread).arg(account.displayName());
        QAction *action = m_menu->addAction(menuText(label));
        const QString id = account.id;
        if (state.checkedOnce && !state.last.ok) {
            action->setToolTip(state.last.error);
            connect(action, &QAction::triggered, this, [this, id] { m_window->editAccount(id); });
        } else {
            action->setEnabled(!account.inboxUrl().isEmpty());
            connect(action, &QAction::triggered, this, [this, id] { openInbox(id); });
        }
        // The newest few unread messages, under their account.
        const int shown = qMin<int>(3, int(state.recent.size()));
        for (int i = 0; i < shown; ++i) {
            QAction *message = m_menu->addAction(menuText(QStringLiteral("    ") + elide(messageLine(state.recent.at(i)))));
            message->setEnabled(!account.inboxUrl().isEmpty());
            connect(message, &QAction::triggered, this, [this, id] { openInbox(id); });
        }
        if (state.unread > shown && shown > 0) {
            QAction *more = m_menu->addAction(QStringLiteral("    ") + tr("%n more…", nullptr, state.unread - shown));
            more->setEnabled(!account.inboxUrl().isEmpty());
            connect(more, &QAction::triggered, this, [this, id] { openInbox(id); });
        }
    }
    if (!any) {
        QAction *add = m_menu->addAction(tr("Add an Account…"));
        connect(add, &QAction::triggered, m_window, &MainWindow::addAccount);
    }
    m_menu->addSeparator();
    QAction *check = m_menu->addAction(m_monitor->isChecking() ? tr("Checking…") : tr("Check Now"));
    check->setEnabled(any && !m_monitor->isChecking());
    connect(check, &QAction::triggered, m_monitor, &MailMonitor::checkAllNow);
    m_menu->addAction(tr("Accounts and Settings…"), m_window, &MainWindow::showAndRaise);
    // The window comes first, so the About window has a parent on screen.
    m_menu->addAction(tr("About…"), m_window, [this] {
        m_window->showAndRaise();
        m_window->showAbout();
    });
    m_menu->addSeparator();
    // Under a modal dialog, which may hold unsaved input, the dialog comes to
    // the front instead.
    m_menu->addAction(tr("Quit"), this, [this] {
        if (QApplication::activeModalWidget())
            m_window->showAndRaise();
        else
            QApplication::quit();
    });
}

void App::updateTray()
{
    if (!m_tray)
        return;
    const int total = m_monitor->totalUnread();
    const bool problem = m_monitor->hasProblem();
    // A dot for each account with unread mail, in the order of the list.
    QList<QColor> dots;
    for (const Account &account : m_monitor->accounts()) {
        if (account.enabled && account.colour.isValid() && m_monitor->state(account.id).unread > 0)
            dots << account.colour;
    }
    m_tray->setState(problem ? platform::Tray::State::Problem
                             : (total > 0 ? platform::Tray::State::Unread : platform::Tray::State::NoMail),
                     total, dots);

    QStringList lines{QApplication::applicationDisplayName()};
    for (const Account &account : m_monitor->accounts()) {
        if (!account.enabled)
            continue;
        const AccountState state = m_monitor->state(account.id);
        if (state.checkedOnce && !state.last.ok)
            lines << tr("%1: cannot check").arg(account.displayName());
        else if (state.lastGood.isValid())
            lines << tr("%1: %n unread", nullptr, state.unread).arg(account.displayName());
    }
    if (lines.size() == 1)
        lines << tr("No accounts");
    m_tray->setToolTip(lines.join(QLatin1Char('\n')));
}

void App::onNewMail(const Account &account, int newCount, const QList<MailSummary> &messages)
{
    if (!m_notifier || !m_settings->notifications() || !account.notify)
        return;
    const QString title = newCount == 1 ? tr("New email: %1").arg(account.displayName())
                                        : tr("%n new emails: %1", nullptr, newCount).arg(account.displayName());
    QStringList lines;
    for (const MailSummary &m : messages.mid(0, 3))
        lines << elide(messageLine(m), 90);
    if (lines.isEmpty())
        lines << tr("%n unread", nullptr, m_monitor->state(account.id).unread);
    m_notifier->show(title, lines.join(QLatin1Char('\n')), account.id);
}

void App::openInbox(const QString &accountId)
{
    for (const Account &account : m_monitor->accounts()) {
        if (account.id != accountId)
            continue;
        if (!account.inboxUrl().isEmpty())
            QDesktopServices::openUrl(QUrl(account.inboxUrl()));
        else
            m_window->showAndRaise();
        // The count is likely to drop soon after the inbox is opened.
        QTimer::singleShot(60 * 1000, m_monitor, [monitor = QPointer<MailMonitor>(m_monitor), accountId] {
            if (monitor)
                monitor->checkNow(accountId);
        });
        return;
    }
}

} // namespace iec
