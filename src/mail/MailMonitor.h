#pragma once

#include "accounts/Account.h"
#include "mail/CheckResult.h"
#include "mail/GmailApi.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QTimer>

#include <functional>

namespace iec {

class SecretStore;

// What is known about one account.
struct AccountState
{
    CheckResult last;           // the last check, good or not
    int unread = 0;             // from the last good check
    QList<MailSummary> recent;  // from the last good check
    QStringList seenIds;        // the unread ids of the last good check
    QDateTime checkedAt;        // when the last check ended
    QDateTime lastGood;         // when the last good check ended
    bool checking = false;
    bool checkedOnce = false;
    // The server refused the password or sign-in. Checking pauses until the
    // account is saved again or checked by hand, so a wrong password is not
    // tried again and again, which can get the account locked.
    bool paused = false;
};

// Checks every enabled account at its interval, and tells when the counts
// change or new email arrives.
class MailMonitor : public QObject
{
    Q_OBJECT

public:
    MailMonitor(SecretStore *secrets, std::function<gmail::OAuthClient()> googleClient, QObject *parent = nullptr);

    // Replaces the accounts. An account that kept its id keeps its state,
    // unless edited names it; edited accounts are checked again.
    void setAccounts(const QList<Account> &accounts, const QStringList &edited = {});
    const QList<Account> &accounts() const { return m_accounts; }
    AccountState state(const QString &id) const { return m_states.value(id); }

    void checkNow(const QString &id);
    void checkAllNow();

    int totalUnread() const;
    // Some enabled account's last check failed.
    bool hasProblem() const;
    bool isChecking() const;

signals:
    void changed();
    // A check found messages that the last check of the account had not seen.
    // newMessages holds those of them among the newest few.
    void newMail(const iec::Account &account, int newCount, const QList<iec::MailSummary> &newMessages);

private:
    void tick();
    void startCheck(const Account &account);
    void finished(const QString &id, const CheckResult &result);

    SecretStore *m_secrets;
    std::function<gmail::OAuthClient()> m_googleClient;
    QList<Account> m_accounts;
    QHash<QString, AccountState> m_states;
    QHash<QString, QDateTime> m_due;
    QTimer m_tick;
};

} // namespace iec
