export module sandbox.file;

import std;
import sandbox.log;

namespace sandbox::file {
    export std::optional<std::string> readFile(const std::string& filename) {
        std::ifstream stream{filename};
        if (!stream.is_open()) {
            log::error(std::format("Could not open file '{}'", filename));
            return std::nullopt;
        }
        std::ostringstream stringBuffer;
        std::string buffer;
        while (std::getline(stream, buffer)) {
            stringBuffer << buffer << std::endl;
        }
        return {stringBuffer.str()};
    }
}
