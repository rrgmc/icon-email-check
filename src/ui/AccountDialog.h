#pragma once

#include "accounts/Account.h"
#include "mail/GmailApi.h"

#include <QDialog>
#include <QPointer>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;

namespace iec {

class GoogleSignIn;
class MailChecker;
class SecretStore;

// Adds or edits one account. The secret (password or refresh token) is kept
// apart from the account and saved by the caller.
class AccountDialog : public QDialog
{
    Q_OBJECT

public:
    // hasSecret: an edited account already has a secret in the keychain.
    AccountDialog(const Account &account, bool hasSecret, const gmail::OAuthClient &googleClient,
                  SecretStore *secrets, QWidget *parent = nullptr);
    ~AccountDialog() override;

    Account account() const;
    // The new password or refresh token, or empty to keep the saved one.
    QString newSecret() const { return m_newSecret; }

    void accept() override;

private:
    void typeChanged();
    void presetChanged();
    void signIn();
    void test();
    void updateGmailStatus();
    QString validate() const;

    Account m_account;
    bool m_hasSecret;
    gmail::OAuthClient m_googleClient;
    SecretStore *m_secrets;
    QString m_newSecret;
    QString m_gmailEmail;

    QComboBox *m_type = nullptr;
    QLineEdit *m_name = nullptr;
    QStackedWidget *m_pages = nullptr;
    // Gmail
    QLabel *m_gmailStatus = nullptr;
    QPushButton *m_signIn = nullptr;
    QLineEdit *m_gmailQuery = nullptr;
    // IMAP
    QComboBox *m_preset = nullptr;
    QLineEdit *m_host = nullptr;
    QSpinBox *m_port = nullptr;
    QComboBox *m_security = nullptr;
    QLineEdit *m_username = nullptr;
    QLineEdit *m_password = nullptr;
    QLineEdit *m_folder = nullptr;
    QLabel *m_presetHelp = nullptr;
    // Both
    QSpinBox *m_interval = nullptr;
    QCheckBox *m_notify = nullptr;
    QCheckBox *m_enabled = nullptr;
    QLineEdit *m_webUrl = nullptr;
    QPushButton *m_test = nullptr;
    QLabel *m_testResult = nullptr;

    QPointer<GoogleSignIn> m_signInFlow;
    QPointer<MailChecker> m_testChecker;
};

} // namespace iec
