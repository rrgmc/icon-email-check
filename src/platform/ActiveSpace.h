#pragma once

#include <QList>

class QWidget;

namespace iec::platform {

// On macOS, makes the given windows appear on the current desktop (Space) the
// next time they are shown, instead of on the Space they were hidden from.
// Call it before showing them. Elsewhere it does nothing.
void showOnActiveSpace(const QList<QWidget *> &windows);

} // namespace iec::platform
