#pragma once

#include <QtGlobal>

namespace iec::platform {

// Lets another process bring its window to the front. Windows keeps a
// background process from taking the foreground, so a second start calls this
// for the running instance before it asks it to show its window. With pid 0,
// any process may. Elsewhere it does nothing.
void allowForegroundFor(qint64 pid);

// Whether a window of this app is the one the user is working in. On Windows
// this asks the system, because Qt's activation state can lag behind.
bool appIsInFront();

} // namespace iec::platform
