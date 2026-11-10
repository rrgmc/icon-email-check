#pragma once

#include <QByteArray>
#include <QString>

namespace iec::mime {

// Decodes RFC 2047 encoded words, such as "=?UTF-8?B?w6k=?=", in a header
// value. Text outside encoded words is read as UTF-8. Whitespace between two
// encoded words is dropped, as the RFC says.
QString decodeWords(const QByteArray &value);

// The value of a header field in a block of header lines, unfolded and
// decoded. Empty when the field is missing. name is matched without case.
QString headerField(const QByteArray &headers, const QByteArray &name);

// The sender as a person reads it: the display name of "Name <a@b>" without
// quotes, else the address.
QString displayFrom(const QString &from);

} // namespace iec::mime
