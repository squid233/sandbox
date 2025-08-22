module;

#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>

export module sandbox.log;

import std;

namespace sandbox::log {
    export void configure() {
        log4cxx::PropertyConfigurator::configure("res/log4cxx.properties");
    }

    export using func_t = void(*)(const std::string_view&);

    export void info(const std::string_view& message) {
        static log4cxx::LoggerPtr file = log4cxx::Logger::getLogger("file");
        LOG4CXX_INFO(file, message);
#ifdef _DEBUG
        static log4cxx::LoggerPtr console = log4cxx::Logger::getLogger("console");
        LOG4CXX_INFO(console, message);
#endif
    }

    export void warn(const std::string_view& message) {
        static log4cxx::LoggerPtr file = log4cxx::Logger::getLogger("file");
        LOG4CXX_WARN(file, message);
#ifdef _DEBUG
        static log4cxx::LoggerPtr console = log4cxx::Logger::getLogger("console");
        LOG4CXX_WARN(console, message);
#endif
    }

    export void error(const std::string_view& message) {
        static log4cxx::LoggerPtr file = log4cxx::Logger::getLogger("file");
        LOG4CXX_ERROR(file, message);
#ifdef _DEBUG
        static log4cxx::LoggerPtr console = log4cxx::Logger::getLogger("console");
        LOG4CXX_ERROR(console, message);
#endif
    }
}
