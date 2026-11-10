#include "ui/AccountDialog.h"

#include "accounts/SecretStore.h"
#include "mail/GmailChecker.h"
#include "mail/GoogleSignIn.h"
#include "mail/ImapChecker.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QUuid>
#include <QVBoxLayout>

namespace iec {

namespace {

struct Preset
{
    const char *name;
    const char *host;
    int port;
    ImapSecurity security;
    const char *help;
};

// Common providers. Most need an app password, made in the provider's
// account settings, instead of the account's own password.
const Preset presets[] = {
    {QT_TRANSLATE_NOOP("AccountDialog", "Other server"), "", 993, ImapSecurity::Tls, ""},
    {QT_TRANSLATE_NOOP("AccountDialog", "Gmail (app password)"), "imap.gmail.com", 993, ImapSecurity::Tls,
     QT_TRANSLATE_NOOP("AccountDialog",
                       "Use an app password: Google Account > Security > 2-Step Verification > App passwords.")},
    {QT_TRANSLATE_NOOP("AccountDialog", "iCloud Mail"), "imap.mail.me.com", 993, ImapSecurity::Tls,
     QT_TRANSLATE_NOOP("AccountDialog", "Use an app-specific password from account.apple.com.")},
    {QT_TRANSLATE_NOOP("AccountDialog", "Yahoo Mail"), "imap.mail.yahoo.com", 993, ImapSecurity::Tls,
     QT_TRANSLATE_NOOP("AccountDialog", "Use an app password from Yahoo's account security page.")},
    {QT_TRANSLATE_NOOP("AccountDialog", "Fastmail"), "imap.fastmail.com", 993, ImapSecurity::Tls,
     QT_TRANSLATE_NOOP("AccountDialog", "Use an app password from Fastmail's settings.")},
};

} // namespace

AccountDialog::AccountDialog(const Account &account, bool hasSecret, const gmail::OAuthClient &googleClient,
                             SecretStore *secrets, QWidget *parent)
    : QDialog(parent)
    , m_account(account)
    , m_hasSecret(hasSecret)
    , m_googleClient(googleClient)
    , m_secrets(secrets)
    , m_gmailEmail(account.type == AccountType::Gmail ? account.email : QString())
{
    setWindowTitle(account.id.isEmpty() ? tr("Add Account") : tr("Edit Account"));
    if (m_account.id.isEmpty())
        m_account.id = QUuid::createUuid().toString(QUuid::WithoutBraces);

    auto *layout = new QVBoxLayout(this);
    auto *top = new QFormLayout;
    m_type = new QComboBox;
    m_type->addItem(tr("Gmail: sign in with Google"), int(AccountType::Gmail));
    m_type->addItem(tr("IMAP: any mail server"), int(AccountType::Imap));
    m_type->setCurrentIndex(account.type == AccountType::Imap ? 1 : 0);
    top->addRow(tr("Type:"), m_type);
    m_name = new QLineEdit(account.name);
    m_name->setPlaceholderText(tr("Shown in the menu, such as \"Personal\""));
    top->addRow(tr("Name:"), m_name);
    layout->addLayout(top);

    m_pages = new QStackedWidget;
    // Gmail page.
    {
        auto *page = new QWidget;
        auto *form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        m_gmailStatus = new QLabel;
        m_gmailStatus->setWordWrap(true);
        m_signIn = new QPushButton(tr("Sign in with Google…"));
        auto *row = new QHBoxLayout;
        row->addWidget(m_gmailStatus, 1);
        row->addWidget(m_signIn);
        form->addRow(tr("Account:"), row);
        m_gmailQuery = new QLineEdit(account.gmailQuery);
        m_gmailQuery->setPlaceholderText(tr("Empty: unread messages in the inbox"));
        m_gmailQuery->setToolTip(tr("A Gmail search, such as \"is:unread category:primary\" or \"is:unread label:work\"."));
        form->addRow(tr("Count:"), m_gmailQuery);
        m_pages->addWidget(page);
        connect(m_signIn, &QPushButton::clicked, this, &AccountDialog::signIn);
    }
    // IMAP page.
    {
        auto *page = new QWidget;
        auto *form = new QFormLayout(page);
        form->setContentsMargins(0, 0, 0, 0);
        m_preset = new QComboBox;
        for (const Preset &preset : presets)
            m_preset->addItem(tr(preset.name));
        form->addRow(tr("Provider:"), m_preset);
        m_presetHelp = new QLabel;
        m_presetHelp->setWordWrap(true);
        m_presetHelp->setVisible(false);
        form->addRow(QString(), m_presetHelp);
        m_host = new QLineEdit(account.host);
        m_host->setPlaceholderText(tr("imap.example.com"));
        form->addRow(tr("Server:"), m_host);
        auto *portRow = new QHBoxLayout;
        m_port = new QSpinBox;
        m_port->setRange(1, 65535);
        m_port->setValue(account.port);
        m_security = new QComboBox;
        m_security->addItem(tr("TLS"), int(ImapSecurity::Tls));
        m_security->addItem(tr("STARTTLS"), int(ImapSecurity::StartTls));
        m_security->addItem(tr("None (unsafe)"), int(ImapSecurity::None));
        m_security->setCurrentIndex(m_security->findData(int(account.security)));
        portRow->addWidget(m_port);
        portRow->addWidget(m_security, 1);
        form->addRow(tr("Port:"), portRow);
        m_username = new QLineEdit(account.username);
        m_username->setPlaceholderText(tr("Usually the email address"));
        form->addRow(tr("User name:"), m_username);
        m_password = new QLineEdit;
        m_password->setEchoMode(QLineEdit::Password);
        if (hasSecret && account.type == AccountType::Imap)
            m_password->setPlaceholderText(tr("Saved; type to change"));
        form->addRow(tr("Password:"), m_password);
        m_folder = new QLineEdit(account.folder);
        form->addRow(tr("Folder:"), m_folder);
        m_pages->addWidget(page);
        // Pick the preset that matches the saved server.
        for (int i = 1; i < int(std::size(presets)); ++i) {
            if (account.host.compare(QLatin1String(presets[i].host), Qt::CaseInsensitive) == 0)
                m_preset->setCurrentIndex(i);
        }
        connect(m_preset, &QComboBox::currentIndexChanged, this, &AccountDialog::presetChanged);
        connect(m_security, &QComboBox::currentIndexChanged, this, [this] {
            const auto security = ImapSecurity(m_security->currentData().toInt());
            if (security == ImapSecurity::Tls && m_port->value() == 143)
                m_port->setValue(993);
            else if (security != ImapSecurity::Tls && m_port->value() == 993)
                m_port->setValue(143);
        });
    }
    layout->addWidget(m_pages);

    auto *common = new QGroupBox(tr("Checking"));
    auto *form = new QFormLayout(common);
    m_interval = new QSpinBox;
    m_interval->setRange(1, 24 * 60);
    m_interval->setSuffix(tr(" min"));
    m_interval->setValue(account.intervalMinutes);
    form->addRow(tr("Check every:"), m_interval);
    m_notify = new QCheckBox(tr("Notify me of new email"));
    m_notify->setChecked(account.notify);
    form->addRow(QString(), m_notify);
    m_enabled = new QCheckBox(tr("Check this account"));
    m_enabled->setChecked(account.enabled);
    form->addRow(QString(), m_enabled);
    m_webUrl = new QLineEdit(account.webUrl);
    m_webUrl->setPlaceholderText(tr("Empty: Gmail's inbox for Gmail accounts"));
    m_webUrl->setToolTip(tr("Opened when you click the account in the menu or a notification."));
    form->addRow(tr("Web page:"), m_webUrl);
    layout->addWidget(common);

    m_testResult = new QLabel;
    m_testResult->setWordWrap(true);
    m_testResult->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_testResult);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_test = buttons->addButton(tr("Test"), QDialogButtonBox::ActionRole);
    connect(m_test, &QPushButton::clicked, this, &AccountDialog::test);
    connect(buttons, &QDialogButtonBox::accepted, this, &AccountDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &AccountDialog::reject);
    layout->addWidget(buttons);

    connect(m_type, &QComboBox::currentIndexChanged, this, &AccountDialog::typeChanged);
    typeChanged();
    updateGmailStatus();
    if (m_preset->currentIndex() > 0) {
        m_presetHelp->setText(tr(presets[m_preset->currentIndex()].help));
        m_presetHelp->setVisible(true);
    }
    resize(sizeHint().width() + 80, sizeHint().height());
}

AccountDialog::~AccountDialog()
{
    if (m_signInFlow)
        m_signInFlow->disconnect(this);
}

void AccountDialog::typeChanged()
{
    m_pages->setCurrentIndex(m_type->currentIndex());
    // The hidden page must not decide the dialog's height.
    for (int i = 0; i < m_pages->count(); ++i)
        m_pages->widget(i)->setSizePolicy(QSizePolicy::Preferred,
                                          i == m_pages->currentIndex() ? QSizePolicy::Preferred : QSizePolicy::Ignored);
    m_pages->adjustSize();
    adjustSize();
}

void AccountDialog::presetChanged()
{
    const Preset &preset = presets[m_preset->currentIndex()];
    m_presetHelp->setText(tr(preset.help));
    m_presetHelp->setVisible(m_preset->currentIndex() > 0);
    if (m_preset->currentIndex() == 0)
        return;
    m_host->setText(QLatin1String(preset.host));
    m_port->setValue(preset.port);
    m_security->setCurrentIndex(m_security->findData(int(preset.security)));
}

void AccountDialog::updateGmailStatus()
{
    const bool signedIn = !m_newSecret.isEmpty() || (m_hasSecret && m_account.type == AccountType::Gmail);
    if (m_signInFlow) {
        m_gmailStatus->setText(tr("Waiting for the browser…"));
        m_signIn->setText(tr("Cancel Sign-in"));
        return;
    }
    m_signIn->setText(signedIn ? tr("Sign in Again…") : tr("Sign in with Google…"));
    if (!m_googleClient.isValid())
        m_gmailStatus->setText(tr("Set a Google OAuth client in the settings first."));
    else if (signedIn && !m_gmailEmail.isEmpty())
        m_gmailStatus->setText(tr("Signed in as %1").arg(m_gmailEmail));
    else
        m_gmailStatus->setText(tr("Not signed in"));
    m_signIn->setEnabled(m_googleClient.isValid());
}

void AccountDialog::signIn()
{
    if (m_signInFlow) {
        m_signInFlow->cancel();
        return;
    }
    m_signInFlow = new GoogleSignIn(this);
    connect(m_signInFlow, &GoogleSignIn::finished, this,
            [this](bool ok, const QString &refreshToken, const QString &email, const QString &error) {
                m_signInFlow->deleteLater();
                m_signInFlow = nullptr;
                if (ok) {
                    m_newSecret = refreshToken;
                    m_gmailEmail = email;
                    if (m_name->text().trimmed().isEmpty())
                        m_name->setText(email);
                    m_testResult->clear();
                } else {
                    m_testResult->setText(error);
                }
                updateGmailStatus();
                // Back from the browser.
                raise();
                activateWindow();
            });
    m_signInFlow->start(m_googleClient);
    updateGmailStatus();
}

Account AccountDialog::account() const
{
    Account a = m_account;
    a.type = AccountType(m_type->currentData().toInt());
    a.name = m_name->text().trimmed();
    a.intervalMinutes = m_interval->value();
    a.notify = m_notify->isChecked();
    a.enabled = m_enabled->isChecked();
    a.webUrl = m_webUrl->text().trimmed();
    if (a.type == AccountType::Gmail) {
        a.email = m_gmailEmail;
        a.gmailQuery = m_gmailQuery->text().trimmed();
    } else {
        a.host = m_host->text().trimmed();
        a.port = m_port->value();
        a.security = ImapSecurity(m_security->currentData().toInt());
        a.username = m_username->text().trimmed();
        a.folder = m_folder->text().trimmed().isEmpty() ? QStringLiteral("INBOX") : m_folder->text().trimmed();
        a.email = a.username.contains(QLatin1Char('@')) ? a.username : QString();
    }
    return a;
}

QString AccountDialog::validate() const
{
    const Account a = account();
    // A secret saved for the other type does not count.
    const bool savedForType = m_hasSecret && m_account.type == a.type;
    if (a.type == AccountType::Gmail) {
        if (m_newSecret.isEmpty() && !savedForType)
            return tr("Sign in with Google first.");
    } else {
        if (a.host.isEmpty())
            return tr("Enter the server's name.");
        if (a.username.isEmpty())
            return tr("Enter the user name.");
        if (m_password->text().isEmpty() && !savedForType)
            return tr("Enter the password.");
    }
    return {};
}

void AccountDialog::accept()
{
    const QString problem = validate();
    if (!problem.isEmpty()) {
        m_testResult->setText(problem);
        return;
    }
    if (account().type == AccountType::Imap) {
        if (!m_password->text().isEmpty())
            m_newSecret = m_password->text();
        else if (m_account.type != AccountType::Imap)
            m_newSecret.clear();
    }
    QDialog::accept();
}

void AccountDialog::test()
{
    const QString problem = validate();
    if (!problem.isEmpty()) {
        m_testResult->setText(problem);
        return;
    }
    if (m_testChecker)
        return;
    const Account a = account();
    m_test->setEnabled(false);
    m_testResult->setText(tr("Checking…"));

    auto run = [this, a](const QString &secret) {
        MailChecker *checker = a.type == AccountType::Gmail ? static_cast<MailChecker *>(new GmailChecker(m_googleClient, this))
                                                            : new ImapChecker(this);
        m_testChecker = checker;
        connect(checker, &MailChecker::finished, this, [this, checker](const CheckResult &result) {
            checker->deleteLater();
            m_test->setEnabled(true);
            if (result.ok)
                m_testResult->setText(tr("It works: %n unread.", nullptr, result.unread));
            else
                m_testResult->setText(tr("It failed: %1").arg(result.error));
        });
        // A test account id keeps the Gmail token cache of the saved account apart.
        Account test = a;
        test.id = QStringLiteral("test-") + a.id;
        checker->start(test, secret);
    };

    QString secret = a.type == AccountType::Gmail ? m_newSecret : m_password->text();
    if (!secret.isEmpty()) {
        run(secret);
        return;
    }
    QPointer<AccountDialog> self(this);
    m_secrets->read(m_account.id, [self, run](const QString &saved, const QString &error) {
        if (!self)
            return;
        if (saved.isEmpty()) {
            self->m_test->setEnabled(true);
            self->m_testResult->setText(error);
            return;
        }
        run(saved);
    });
}

} // namespace iec
