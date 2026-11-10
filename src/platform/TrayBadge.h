#pragma once

#include <QIcon>
#include <QColor>
#include <QImage>
#include <QList>
#include <QString>

namespace iec::platform {

// The unread count drawn on the tray icon on Windows and Linux.

// The count as the badge shows it: "99+" above 99.
QString trayBadgeText(int count);

// The image with the count in a red badge at its lower right. A count of 0 or
// less gives the image unchanged.
QImage paintTrayBadge(const QImage &image, int count);

// The most account dots the tray icon shows.
constexpr int maxAccountDots = 3;

// The image with a row of dots along its top edge, one in each colour, up to
// maxAccountDots. Each dot is cut out of the image under it, so it stays
// apart from it. No colours give the image unchanged.
QImage paintAccountDots(const QImage &image, const QList<QColor> &colours);

// The icon with the account dots and the badge, at each size a tray may ask
// for. A count of 0 or less and no colours give the icon unchanged.
QIcon withTrayBadge(const QIcon &icon, int count, const QList<QColor> &colours = {});

} // namespace iec::platform
