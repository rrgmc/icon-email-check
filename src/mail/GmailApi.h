#pragma once

#include "mail/CheckResult.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace iec::gmail {

// The OAuth client that signs in to Google. A "Desktop app" client from the
// Google Cloud console. Google calls its secret a secret, but for a desktop
// app it is not one: it ships inside the app.
struct OAuthClient
{
    QString id;
    QString secret;
    bool isValid() const { return !id.trimmed().isEmpty(); }
};

// The access to ask for: read the mailbox, without changing it.
inline constexpr const char *scope = "https://www.googleapis.com/auth/gmail.readonly";

// The answer to a token request.
struct TokenResponse
{
    QString accessToken;
    QString refreshToken;   // only after a sign-in, not after a refresh
    int expiresIn = 0;      // seconds
    QString error;          // Google's error code, such as "invalid_grant"
    QString errorDescription;
};
TokenResponse parseTokenResponse(const QByteArray &json);

// messagesUnread of a users.labels.get answer, or -1.
int parseLabelUnread(const QByteArray &json);

// The ids of a users.messages.list answer, appended to ids, and the token of
// the next page, or empty.
void parseMessageList(const QByteArray &json, QStringList *ids, QString *nextPageToken);

// The From and Subject of a users.messages.get answer in metadata format.
MailSummary parseMessageMetadata(const QByteArray &json);

// The emailAddress of a users.getProfile answer.
QString parseProfileEmail(const QByteArray &json);

// The message of a Google API error answer, or empty.
QString parseApiError(const QByteArray &json);

// Random text for a PKCE code verifier or an OAuth state: 64 characters of
// [A-Za-z0-9-._~].
QByteArray randomToken();

// The PKCE S256 challenge of a verifier: base64url(SHA-256), without padding.
QByteArray pkceChallenge(const QByteArray &verifier);

} // namespace iec::gmail
