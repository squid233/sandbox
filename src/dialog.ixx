module;

#include <tinyfiledialogs/tinyfiledialogs.h>

export module sandbox.dialog;

namespace sandbox::dialog {
    export void errorMessageBox(const char *aMessage) {
        tinyfd_messageBox("sandbox error", aMessage, "ok", "error", 1);
    }
}
