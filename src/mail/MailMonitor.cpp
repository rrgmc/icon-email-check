#include "mail/MailMonitor.h"

#include "accounts/SecretStore.h"
#include "mail/GmailChecker.h"
#include "mail/ImapChecker.h"

#include <QPointer>
#include <QSet>

#include <algorithm>

namespace iec {

namespace {

// After a failed check, try again sooner than the interval, but not at once.
constexpr int retryMinutes = 2;

} // namespace

MailMonitor::MailMonitor(SecretStore *secrets, std::function<gmail::OAuthClient()> googleClient, QObject *parent)
    : QObject(parent)
    , m_secrets(secrets)
    , m_googleClient(std::move(googleClient))
{
    // One timer for all accounts. After the computer sleeps, the next tick
    // finds every account due and checks it.
    m_tick.setInterval(15 * 1000);
    connect(&m_tick, &QTimer::timeout, this, &MailMonitor::tick);
    m_tick.start();
}

void MailMonitor::setAccounts(const QList<Account> &accounts, const QStringList &edited)
{
    QSet<QString> ids;
    for (const Account &account : accounts)
        ids.insert(account.id);
    for (auto it = m_states.begin(); it != m_states.end();) {
        if (!ids.contains(it.key()))
            it = m_states.erase(it);
        else
            ++it;
    }
    m_accounts = accounts;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const Account &account : accounts) {
        if (edited.contains(account.id)) {
            AccountState &state = m_states[account.id];
            const bool checking = state.checking;
            state = AccountState{};
            state.checking = checking;
            m_due.insert(account.id, now);
        } else if (!m_due.contains(account.id)) {
            m_due.insert(account.id, now);
        }
    }
    emit changed();
    QMetaObject::invokeMethod(this, &MailMonitor::tick, Qt::QueuedConnection);
}

void MailMonitor::checkNow(const QString &id)
{
    for (const Account &account : std::as_const(m_accounts)) {
        if (account.id == id && account.enabled) {
            m_states[id].paused = false;
            startCheck(account);
        }
    }
}

void MailMonitor::checkAllNow()
{
    for (const Account &account : std::as_const(m_accounts)) {
        if (account.enabled) {
            m_states[account.id].paused = false;
            startCheck(account);
        }
    }
}

int MailMonitor::totalUnread() const
{
    int total = 0;
    for (const Account &account : m_accounts) {
        if (account.enabled)
            total += m_states.value(account.id).unread;
    }
    return total;
}

bool MailMonitor::hasProblem() const
{
    for (const Account &account : m_accounts) {
        const AccountState state = m_states.value(account.id);
        if (account.enabled && state.checkedOnce && !state.last.ok)
            return true;
    }
    return false;
}

bool MailMonitor::isChecking() const
{
    for (const AccountState &state : m_states) {
        if (state.checking)
            return true;
    }
    return false;
}

void MailMonitor::tick()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    for (const Account &account : std::as_const(m_accounts)) {
        const AccountState state = m_states.value(account.id);
        if (!account.enabled || state.paused || state.checking)
            continue;
        if (m_due.value(account.id) <= now)
            startCheck(account);
    }
}

void MailMonitor::startCheck(const Account &account)
{
    AccountState &state = m_states[account.id];
    if (state.checking)
        return;
    state.checking = true;
    emit changed();

    const QString id = account.id;
    QPointer<MailMonitor> self(this);
    m_secrets->read(id, [self, account, id](const QString &secret, const QString &error) {
        if (!self)
            return;
        if (secret.isEmpty()) {
            CheckResult result;
            result.error = error.isEmpty() ? tr("No password or sign-in is saved for this account.") : error;
            result.authFailed = true;
            self->finished(id, result);
            return;
        }
        MailChecker *checker = nullptr;
        if (account.type == AccountType::Gmail)
            checker = new GmailChecker(self->m_googleClient(), self);
        else
            checker = new ImapChecker(self);
        connect(checker, &MailChecker::finished, self, [self, checker, id](const CheckResult &result) {
            checker->deleteLater();
            if (self)
                self->finished(id, result);
        });
        checker->start(account, secret);
    });
}

void MailMonitor::finished(const QString &id, const CheckResult &result)
{
    const auto found = std::find_if(m_accounts.cbegin(), m_accounts.cend(), [&](const Account &a) { return a.id == id; });
    if (found == m_accounts.cend())
        return;    // removed while it was checked
    const Account account = *found;
    AccountState &state = m_states[id];
    const QDateTime now = QDateTime::currentDateTimeUtc();

    state.checking = false;
    state.checkedOnce = true;
    state.checkedAt = now;
    state.last = result;

    if (!result.ok) {
        state.paused = result.authFailed;
        m_due.insert(id, now.addSecs(60 * qMin(account.intervalMinutes, retryMinutes)));
        emit changed();
        return;
    }

    // The first good check of a run only learns what is already there.
    const bool baseline = !state.lastGood.isValid();
    const QSet<QString> seen(state.seenIds.cbegin(), state.seenIds.cend());
    QSet<QString> fresh;
    for (const QString &messageId : result.unreadIds) {
        if (!seen.contains(messageId))
            fresh.insert(messageId);
    }
    state.seenIds = result.unreadIds;
    state.unread = result.unread;
    state.recent = result.recent;
    state.lastGood = now;
    state.paused = false;
    m_due.insert(id, now.addSecs(60 * account.intervalMinutes));
    emit changed();

    if (!baseline && !fresh.isEmpty()) {
        QList<MailSummary> messages;
        for (const MailSummary &summary : result.recent) {
            if (fresh.contains(summary.id))
                messages << summary;
        }
        emit newMail(account, int(fresh.size()), messages);
    }
}

} // namespace iec
