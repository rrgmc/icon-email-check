#pragma once

#include <QHash>
#include <QObject>
#include <QString>

#include <functional>

namespace iec {

// The accounts' passwords and refresh tokens, in the system keychain: the
// macOS Keychain, the Windows Credential Manager, or the Secret Service on
// Linux. Reads are cached, so the keychain is asked once per account and run.
class SecretStore : public QObject
{
    Q_OBJECT

public:
    explicit SecretStore(QObject *parent = nullptr);

    // Calls done with the secret, or with an empty secret and an error.
    void read(const QString &accountId, std::function<void(const QString &secret, const QString &error)> done);
    void write(const QString &accountId, const QString &secret, std::function<void(const QString &error)> done = {});
    void remove(const QString &accountId);

private:
    QHash<QString, QString> m_cache;
};

} // namespace iec
