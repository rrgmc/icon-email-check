#include "accounts/SecretStore.h"

#include <qtkeychain/keychain.h>

namespace iec {

namespace {

const QString service = QStringLiteral("icon-email-check");

QString keyFor(const QString &accountId) { return QStringLiteral("account/") + accountId; }

} // namespace

SecretStore::SecretStore(QObject *parent)
    : QObject(parent)
{
}

void SecretStore::read(const QString &accountId,
                       std::function<void(const QString &secret, const QString &error)> done)
{
    const auto cached = m_cache.constFind(accountId);
    if (cached != m_cache.constEnd()) {
        done(*cached, {});
        return;
    }
    auto *job = new QKeychain::ReadPasswordJob(service, this);
    job->setKey(keyFor(accountId));
    connect(job, &QKeychain::Job::finished, this, [this, job, accountId, done = std::move(done)] {
        if (job->error() == QKeychain::NoError) {
            m_cache.insert(accountId, job->textData());
            done(job->textData(), {});
        } else if (job->error() == QKeychain::EntryNotFound) {
            done({}, tr("No password or sign-in is saved for this account."));
        } else {
            done({}, job->errorString());
        }
    });
    job->start();
}

void SecretStore::write(const QString &accountId, const QString &secret, std::function<void(const QString &)> done)
{
    m_cache.insert(accountId, secret);
    auto *job = new QKeychain::WritePasswordJob(service, this);
    job->setKey(keyFor(accountId));
    job->setTextData(secret);
    connect(job, &QKeychain::Job::finished, this, [job, done = std::move(done)] {
        if (done)
            done(job->error() == QKeychain::NoError ? QString() : job->errorString());
    });
    job->start();
}

void SecretStore::remove(const QString &accountId)
{
    m_cache.remove(accountId);
    auto *job = new QKeychain::DeletePasswordJob(service, this);
    job->setKey(keyFor(accountId));
    job->start();
}

} // namespace iec
