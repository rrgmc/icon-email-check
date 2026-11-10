#pragma once

#include "accounts/Account.h"
#include "mail/GmailApi.h"

#include <QByteArray>
#include <QList>

namespace iec {

// What the app keeps between runs, in the platform's settings store (the
// registry on Windows, a .plist on macOS, an .ini file on Linux). Secrets are
// not here; they are in SecretStore.
class Settings
{
public:
    QList<Account> accounts() const;
    void setAccounts(const QList<Account> &accounts);

    // Start in the tray without showing the window. A start at sign-in is
    // always hidden.
    bool startHidden() const;
    void setStartHidden(bool on);

    // Show a desktop notification when new email arrives.
    bool notifications() const;
    void setNotifications(bool on);

    // The OAuth client set in the settings window, or the one built in with
    // IEC_GOOGLE_CLIENT_ID when none is set.
    gmail::OAuthClient googleClient() const;
    // Only what the user typed; empty when the built-in client is used.
    gmail::OAuthClient customGoogleClient() const;
    void setCustomGoogleClient(const gmail::OAuthClient &client);
    static gmail::OAuthClient builtInGoogleClient();

    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);
};

} // namespace iec
