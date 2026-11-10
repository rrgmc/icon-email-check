#pragma once

class QNetworkAccessManager;

namespace iec {

// The app's one network access manager, made on first use on the main thread.
QNetworkAccessManager *network();

} // namespace iec
