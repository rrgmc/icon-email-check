#include "platform/ActiveSpace.h"

// macOS has its own implementation in ActiveSpaceMac.mm.
#if !defined(Q_OS_MACOS)
namespace iec::platform {

void showOnActiveSpace(const QList<QWidget *> &windows) { Q_UNUSED(windows); }

} // namespace iec::platform
#endif
