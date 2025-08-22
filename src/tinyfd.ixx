module;

#include <tinyfiledialogs/tinyfiledialogs.h>

export module sandbox.tinyfd;

namespace sandbox::tinyfd {
    export void errorMessageBox(const char *aMessage) {
        tinyfd_messageBox("sandbox error", aMessage, "ok", "error", 1);
    }
}
