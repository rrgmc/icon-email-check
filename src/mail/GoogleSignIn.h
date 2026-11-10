#pragma once

#include "mail/GmailApi.h"

#include <QObject>
#include <QTimer>

class QTcpServer;

namespace iec {

// "Sign in with Google" for a desktop app: OAuth 2.0 with PKCE and a loopback
// redirect. It opens Google's consent page in the browser, waits for the
// browser to come back to http://127.0.0.1:<port>, trades the code for a
// refresh token, and reads the account's address.
class GoogleSignIn : public QObject
{
    Q_OBJECT

public:
    explicit GoogleSignIn(QObject *parent = nullptr);
    ~GoogleSignIn() override;

    // finished() follows, also when the user gives up: after five minutes,
    // or with cancel().
    void start(const gmail::OAuthClient &client);
    void cancel();

signals:
    void finished(bool ok, const QString &refreshToken, const QString &email, const QString &error);

private:
    void onConnection();
    void exchange(const QByteArray &code);
    void readProfile(const QString &accessToken, const QString &refreshToken);
    void done(bool ok, const QString &refreshToken, const QString &email, const QString &error);

    gmail::OAuthClient m_client;
    QTcpServer *m_server = nullptr;
    QTimer m_timeout;
    QByteArray m_verifier;
    QByteArray m_state;
    QString m_redirectUri;
    bool m_finished = false;
};

namespace google {

// Trades a refresh token for an access token. authFailed is set when Google
// no longer accepts the refresh token, so the user must sign in again.
void refreshAccessToken(const gmail::OAuthClient &client, const QString &refreshToken, QObject *context,
                        std::function<void(const QString &accessToken, int expiresIn, const QString &error,
                                           bool authFailed)>
                            done);

} // namespace google

} // namespace iec
