#pragma once

#include "accounts/Account.h"
#include "mail/GmailApi.h"

#include <QDialog>
#include <QIcon>
#include <QPointer>

#include <functional>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QFormLayout;

namespace iec {

class GoogleSignIn;

// A round swatch of the colour, for the account's colour in lists and buttons.
QIcon accountColourIcon(const QColor &colour);

class MailChecker;
class SecretStore;

// Adds or edits one account. The secret (password or refresh token) is kept
// apart from the account and saved by the caller.
class AccountDialog : public QDialog
{
    Q_OBJECT

public:
    // hasSecret: an edited account already has a secret in the keychain.
    // googleClient gives the Google OAuth client of the settings. It is asked
    // each time it is needed, so a change in the settings window applies at once.
    AccountDialog(const Account &account, bool hasSecret, std::function<gmail::OAuthClient()> googleClient,
                  SecretStore *secrets, QWidget *parent = nullptr);
    ~AccountDialog() override;

    Account account() const;
    // The new password or refresh token, or empty to keep the saved one.
    QString newSecret() const { return m_newSecret; }

    void accept() override;

private:
    void typeChanged();
    // Shows the rows of the chosen type, and the provider's help when it has one.
    void updateRows();
    void presetChanged();
    void signIn();
    void test();
    void updateGmailStatus();
    void chooseColour();
    QString validate() const;

    Account m_account;
    bool m_hasSecret;
    std::function<gmail::OAuthClient()> m_googleClient;
    SecretStore *m_secrets;
    QString m_newSecret;
    QString m_gmailEmail;

    QComboBox *m_type = nullptr;
    QLineEdit *m_name = nullptr;
    QColor m_colour;
    QPushButton *m_colourButton = nullptr;
    QFormLayout *m_form = nullptr;
    // Gmail
    QLabel *m_gmailStatus = nullptr;
    QPushButton *m_signIn = nullptr;
    QWidget *m_signInRow = nullptr;
    QLineEdit *m_gmailQuery = nullptr;
    // IMAP
    QComboBox *m_preset = nullptr;
    QWidget *m_presetRow = nullptr;
    QLineEdit *m_host = nullptr;
    QSpinBox *m_port = nullptr;
    QWidget *m_portRow = nullptr;
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
