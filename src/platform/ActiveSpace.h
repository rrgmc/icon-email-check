#pragma once

#include <QList>

class QWidget;

namespace iec::platform {

// On macOS, makes the given windows appear on the current desktop (Space)
// instead of on the Space they were last on. A hidden window comes there the
// next time it is shown, so call this before showing it. A window on screen on
// another Space moves there at once. Elsewhere it does nothing.
void showOnActiveSpace(const QList<QWidget *> &windows);

} // namespace iec::platform
