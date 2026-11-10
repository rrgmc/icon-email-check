#include "ui/MainWindow.h"

#include "Settings.h"
#include "accounts/SecretStore.h"
#include "mail/MailMonitor.h"
#include "platform/ActiveSpace.h"
#include "platform/LoginItem.h"
#include "platform/Tray.h"
#include "ui/AccountDialog.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

namespace iec {

namespace {

enum Column { NameColumn, UnreadColumn, StatusColumn };

const QString readmeGmail = QStringLiteral("https://github.com/rrgmc/icon-email-check#gmail");

QString statusText(const Account &account, const AccountState &state)
{
    if (!account.enabled)
        return MainWindow::tr("Not checked");
    if (state.checking)
        return MainWindow::tr("Checking…");
    if (!state.checkedOnce)
        return MainWindow::tr("Waiting");
    if (!state.last.ok) {
        const QString prefix = state.paused ? MainWindow::tr("Paused: ") : QString();
        return prefix + state.last.error;
    }
    return MainWindow::tr("Checked at %1").arg(QLocale().toString(state.checkedAt.toLocalTime().time(), QLocale::ShortFormat));
}

} // namespace

MainWindow::MainWindow(Settings *settings, SecretStore *secrets, MailMonitor *monitor, QWidget *parent)
    : QMainWindow(parent)
    , m_settings(settings)
    , m_secrets(secrets)
    , m_monitor(monitor)
{
    setWindowTitle(QApplication::applicationDisplayName());
    buildUi();
    connect(m_monitor, &MailMonitor::changed, this, &MainWindow::refresh);
    qApp->installEventFilter(this);
    refresh();
    if (!restoreGeometry(m_settings->windowGeometry()))
        resize(720, 520);
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);

    auto *accounts = new QGroupBox(tr("Accounts"));
    auto *accountsLayout = new QHBoxLayout(accounts);
    m_list = new QTreeWidget;
    m_list->setRootIsDecorated(false);
    m_list->setUniformRowHeights(true);
    m_list->setHeaderLabels({tr("Account"), tr("Unread"), tr("Status")});
    m_list->header()->setStretchLastSection(true);
    m_list->header()->setSectionResizeMode(NameColumn, QHeaderView::ResizeToContents);
    m_list->header()->setSectionResizeMode(UnreadColumn, QHeaderView::ResizeToContents);
    accountsLayout->addWidget(m_list, 1);
    auto *buttons = new QVBoxLayout;
    m_add = new QPushButton(tr("Add…"));
    m_edit = new QPushButton(tr("Edit…"));
    m_remove = new QPushButton(tr("Remove"));
    m_up = new QPushButton(tr("Move Up"));
    m_down = new QPushButton(tr("Move Down"));
    m_check = new QPushButton(tr("Check Now"));
    m_open = new QPushButton(tr("Open Inbox"));
    for (QPushButton *b : {m_add, m_edit, m_remove, m_up, m_down})
        buttons->addWidget(b);
    buttons->addSpacing(12);
    buttons->addWidget(m_check);
    buttons->addWidget(m_open);
    buttons->addStretch(1);
    accountsLayout->addLayout(buttons);
    layout->addWidget(accounts, 1);

    connect(m_add, &QPushButton::clicked, this, &MainWindow::addAccount);
    connect(m_edit, &QPushButton::clicked, this, [this] { editAccount(selectedId()); });
    connect(m_remove, &QPushButton::clicked, this, [this] { removeAccount(selectedId()); });
    connect(m_up, &QPushButton::clicked, this, [this] { moveAccount(-1); });
    connect(m_down, &QPushButton::clicked, this, [this] { moveAccount(1); });
    connect(m_check, &QPushButton::clicked, this, [this] {
        const QString id = selectedId();
        if (id.isEmpty())
            m_monitor->checkAllNow();
        else
            m_monitor->checkNow(id);
    });
    connect(m_open, &QPushButton::clicked, this, [this] {
        for (const Account &a : m_monitor->accounts()) {
            if (a.id == selectedId() && !a.inboxUrl().isEmpty())
                QDesktopServices::openUrl(QUrl(a.inboxUrl()));
        }
    });
    connect(m_list, &QTreeWidget::itemSelectionChanged, this, &MainWindow::updateButtons);
    connect(m_list, &QTreeWidget::itemDoubleClicked, this, [this] { editAccount(selectedId()); });

    auto *general = new QGroupBox(tr("General"));
    auto *generalLayout = new QVBoxLayout(general);
    m_startAtSignIn = new QCheckBox(tr("Start when I sign in"));
    m_signInApproval = new QLabel(
        tr("macOS needs your approval: <a href=\"open\">open Login Items in System Settings</a>."));
    m_signInApproval->setVisible(false);
    connect(m_signInApproval, &QLabel::linkActivated, this, [] { platform::loginitem::openSystemSettings(); });
    m_startHidden = new QCheckBox(tr("Start in the tray, without this window"));
    m_startHidden->setChecked(m_settings->startHidden());
    m_notifications = new QCheckBox(tr("Show a notification when new email arrives"));
    m_notifications->setChecked(m_settings->notifications());
    generalLayout->addWidget(m_startAtSignIn);
    generalLayout->addWidget(m_signInApproval);
    generalLayout->addWidget(m_startHidden);
    generalLayout->addWidget(m_notifications);
    layout->addWidget(general);
    if (platform::loginitem::isSupported()) {
        updateStartAtSignIn();
        connect(m_startAtSignIn, &QCheckBox::toggled, this, &MainWindow::setStartAtSignIn);
    } else {
        m_startAtSignIn->setEnabled(false);
        m_startAtSignIn->setToolTip(tr("Starting at sign-in needs macOS 13 or later."));
    }
    connect(m_startHidden, &QCheckBox::toggled, this, [this](bool on) { m_settings->setStartHidden(on); });
    connect(m_notifications, &QCheckBox::toggled, this, [this](bool on) { m_settings->setNotifications(on); });

    if (Settings::googleClientSettings)
        buildGoogleSettings(layout);

    setCentralWidget(central);


    // macOS puts Quit and About in the app menu by their roles.
    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(tr("&Add Account…"), QKeySequence::New, this, &MainWindow::addAccount);
    file->addAction(tr("&Check All Now"), QKeySequence::Refresh, m_monitor, &MailMonitor::checkAllNow);
    file->addSeparator();
    file->addAction(tr("&Close Window"), QKeySequence::Close, this, &QWidget::close);
    QAction *quit = file->addAction(tr("&Quit"), QKeySequence::Quit, qApp, &QApplication::quit);
    quit->setMenuRole(QAction::QuitRole);
    QMenu *help = menuBar()->addMenu(tr("&Help"));
    QAction *about = help->addAction(tr("&About"), this, &MainWindow::showAbout);
    about->setMenuRole(QAction::AboutRole);
}

void MainWindow::buildGoogleSettings(QVBoxLayout *layout)
{
    auto *google = new QGroupBox(tr("Google sign-in"));
    auto *googleLayout = new QFormLayout(google);
    googleLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto *googleHelp = new QLabel(
        tr("Gmail accounts sign in through a Google OAuth client of type \"Desktop app\". "
           "<a href=\"%1\">How to make one</a>. Leave empty to use the built-in client, if this build has one.")
            .arg(readmeGmail));
    googleHelp->setWordWrap(true);
    googleHelp->setOpenExternalLinks(true);
    googleLayout->addRow(googleHelp);
    const gmail::OAuthClient custom = m_settings->customGoogleClient();
    m_clientId = new QLineEdit(custom.id);
    m_clientSecret = new QLineEdit(custom.secret);
    m_clientSecret->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    const bool builtIn = Settings::builtInGoogleClient().isValid();
    m_clientId->setPlaceholderText(builtIn ? tr("Built-in client") : tr("Not set"));
    googleLayout->addRow(tr("Client ID:"), m_clientId);
    googleLayout->addRow(tr("Client secret:"), m_clientSecret);
    // Saved as it is typed: on macOS a click on a button leaves the focus in
    // the field, so a save when the field loses focus would come too late for
    // the Add Account dialog the click opens.
    connect(m_clientId, &QLineEdit::textChanged, this, &MainWindow::saveGoogleClient);
    connect(m_clientSecret, &QLineEdit::textChanged, this, &MainWindow::saveGoogleClient);
    layout->addWidget(google);
}

void MainWindow::setTray(platform::Tray *tray)
{
    m_tray = tray;
    // With a tray, the app keeps running with no window open.
    QApplication::setQuitOnLastWindowClosed(tray == nullptr);
}

void MainWindow::refresh()
{
    const QString selected = selectedId();
    m_list->clear();
    for (const Account &account : m_monitor->accounts()) {
        const AccountState state = m_monitor->state(account.id);
        auto *item = new QTreeWidgetItem(m_list);
        item->setData(NameColumn, Qt::UserRole, account.id);
        item->setText(NameColumn, QStringLiteral("%1 (%2)").arg(account.displayName(), typeName(account.type)));
        item->setIcon(NameColumn, accountColourIcon(account.colour));
        item->setText(UnreadColumn, account.enabled && state.lastGood.isValid() ? QString::number(state.unread)
                                                                               : QStringLiteral("–"));
        item->setTextAlignment(UnreadColumn, Qt::AlignCenter);
        item->setText(StatusColumn, statusText(account, state));
        item->setToolTip(StatusColumn, item->text(StatusColumn));
        if (account.enabled && state.checkedOnce && !state.last.ok)
            item->setForeground(StatusColumn, QColor(0xc0, 0x39, 0x2b));
        if (!account.enabled) {
            for (int c = 0; c < m_list->columnCount(); ++c)
                item->setForeground(c, palette().color(QPalette::Disabled, QPalette::Text));
        }
        if (account.id == selected)
            item->setSelected(true);
    }
    updateButtons();
}

QString MainWindow::selectedId() const
{
    const QList<QTreeWidgetItem *> items = m_list->selectedItems();
    return items.isEmpty() ? QString() : items.first()->data(NameColumn, Qt::UserRole).toString();
}

void MainWindow::updateButtons()
{
    const QString id = selectedId();
    const bool has = !id.isEmpty();
    m_edit->setEnabled(has);
    m_remove->setEnabled(has);
    const int row = has ? m_list->indexOfTopLevelItem(m_list->selectedItems().first()) : -1;
    m_up->setEnabled(row > 0);
    m_down->setEnabled(has && row < m_list->topLevelItemCount() - 1);
    m_check->setText(has ? tr("Check Now") : tr("Check All Now"));
    bool canOpen = false;
    for (const Account &a : m_monitor->accounts())
        canOpen |= a.id == id && !a.inboxUrl().isEmpty();
    m_open->setEnabled(canOpen);
}

void MainWindow::addAccount()
{
    showAndRaise();
    Account account;
    account.colour = nextAccountColour(m_monitor->accounts());
    AccountDialog dialog(account, false, [this] { return m_settings->googleClient(); }, m_secrets, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const Account added = dialog.account();
    QList<Account> accounts = m_monitor->accounts();
    accounts << added;
    m_settings->setAccounts(accounts);
    // The secret is cached at once, so the first check does not wait for the
    // keychain write.
    m_secrets->write(added.id, dialog.newSecret(), [this](const QString &error) {
        if (!error.isEmpty())
            QMessageBox::warning(this, windowTitle(), tr("The password could not be saved in the keychain: %1").arg(error));
    });
    m_monitor->setAccounts(accounts, {added.id});
}

void MainWindow::editAccount(const QString &id)
{
    QList<Account> accounts = m_monitor->accounts();
    const auto it = std::find_if(accounts.begin(), accounts.end(), [&](const Account &a) { return a.id == id; });
    if (it == accounts.end())
        return;
    showAndRaise();
    AccountDialog dialog(*it, true, [this] { return m_settings->googleClient(); }, m_secrets, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    *it = dialog.account();
    m_settings->setAccounts(accounts);
    if (!dialog.newSecret().isEmpty()) {
        m_secrets->write(id, dialog.newSecret(), [this](const QString &error) {
            if (!error.isEmpty())
                QMessageBox::warning(this, windowTitle(),
                                     tr("The password could not be saved in the keychain: %1").arg(error));
        });
    }
    m_monitor->setAccounts(accounts, {id});
}

void MainWindow::removeAccount(const QString &id)
{
    QList<Account> accounts = m_monitor->accounts();
    const auto it = std::find_if(accounts.begin(), accounts.end(), [&](const Account &a) { return a.id == id; });
    if (it == accounts.end())
        return;
    if (QMessageBox::question(this, tr("Remove Account"),
                              tr("Remove \"%1\"? Its saved password or sign-in is removed from the keychain too.")
                                  .arg(it->displayName()))
        != QMessageBox::Yes)
        return;
    accounts.erase(it);
    m_settings->setAccounts(accounts);
    m_secrets->remove(id);
    m_monitor->setAccounts(accounts);
}

void MainWindow::moveAccount(int delta)
{
    QList<Account> accounts = m_monitor->accounts();
    const QString id = selectedId();
    const auto it = std::find_if(accounts.begin(), accounts.end(), [&](const Account &a) { return a.id == id; });
    if (it == accounts.end())
        return;
    const int from = int(it - accounts.begin());
    const int to = from + delta;
    if (to < 0 || to >= accounts.size())
        return;
    accounts.move(from, to);
    m_settings->setAccounts(accounts);
    m_monitor->setAccounts(accounts);
}

void MainWindow::updateStartAtSignIn()
{
    const QSignalBlocker blocker(m_startAtSignIn);
    m_startAtSignIn->setChecked(platform::loginitem::isEnabled());
    m_signInApproval->setVisible(platform::loginitem::needsApproval());
}

void MainWindow::setStartAtSignIn(bool on)
{
    QString error;
    if (!platform::loginitem::setEnabled(on, &error))
        QMessageBox::warning(this, windowTitle(), tr("Starting at sign-in could not be changed: %1").arg(error));
    updateStartAtSignIn();
}

void MainWindow::saveGoogleClient()
{
    if (!m_clientId)
        return;
    const gmail::OAuthClient client{m_clientId->text().trimmed(), m_clientSecret->text().trimmed()};
    const gmail::OAuthClient before = m_settings->customGoogleClient();
    if (client.id == before.id && client.secret == before.secret)
        return;
    m_settings->setCustomGoogleClient(client);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About %1").arg(QApplication::applicationDisplayName()),
                       tr("<h3>%1 %2</h3><p>Counts unread email in the tray.</p>"
                          "<p>Author: Rangel Reale &lt;<a href=\"mailto:realerangel@gmail.com\">realerangel@gmail.com</a>&gt;</p>"
                          "<p><a href=\"https://github.com/rrgmc/icon-email-check\">github.com/rrgmc/icon-email-check</a></p>"
                          "<p><a href=\"https://rrgmc.github.io/icon-email-check/privacy.html\">Privacy policy</a></p>")
                           .arg(QApplication::applicationDisplayName(), QApplication::applicationVersion()));
}

void MainWindow::showAndRaise()
{
    // The window comes to the current desktop, whether it was hidden to the
    // tray or left open on another one. Before the Dock icon returns, because
    // that activates the app.
    QList<QWidget *> windows{this};
    if (QWidget *modal = QApplication::activeModalWidget())
        windows << modal;
    platform::showOnActiveSpace(windows);
    if (m_tray)
        m_tray->setDockIconVisible(true);
    if (platform::loginitem::isSupported())
        updateStartAtSignIn();
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
    if (QWidget *modal = QApplication::activeModalWidget()) {
        modal->raise();
        modal->activateWindow();
    }
}

void MainWindow::hideToTray()
{
    saveState();
    if (!m_tray) {
        showMinimized();
        return;
    }
    hide();
    m_tray->setDockIconVisible(false);
}

void MainWindow::saveState()
{
    saveGoogleClient();
    if (isVisible())
        m_settings->setWindowGeometry(saveGeometry());
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == qApp && event->type() == QEvent::Quit && !m_quitting) {
        m_quitting = true;
        saveState();
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tray && !m_quitting) {
        event->ignore();
        hideToTray();
        return;
    }
    saveState();
    QMainWindow::closeEvent(event);
}

} // namespace iec
