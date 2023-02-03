/**
 * @file plugin.cpp
 * @brief The main file of the plugin
 */

#include <llapi/LoggerAPI.h>

#include "version.h"
#include <llapi/HookAPI.h>
#include <Nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <fstream>
#include <syncstream>

// We recommend using the global logger.
extern Logger logger;

namespace TransactionLogger {

    const inline nlohmann::json defaultData = {
        { "log_type", "both" }
    };

    struct Settings {
        enum LogFlag {
            NONE    = 0,
            FILE    = 1 << 0,
            CONSOLE = 1 << 1,
        };
        int32_t logFlags;
        std::string configName, logName;

        inline Settings() :
            configName("plugins\\transaction_log_config.json"),
            logName("plugins\\transaction_log.txt"),
            logFlags(LogFlag::NONE) {
        }
        inline void init() {
            if (!this->trySerializeConfig()) {
                this->generateDefaultConfig();
            }
            if (this->logFlags & LogFlag::FILE) {
                std::ofstream{ this->logName, std::ofstream::trunc };
            }
        }
        inline void setLogFlags(std::string_view str) {
            if (str == "file") {
                this->logFlags |= LogFlag::FILE;
            }
            else if (str == "console") {
                this->logFlags |= LogFlag::CONSOLE;
            }
            else { // default
                this->logFlags = (LogFlag::FILE | LogFlag::CONSOLE);
            }
        }
        inline void generateDefaultConfig() {
            std::string logTypeStr = defaultData["log_type"].get<std::string>();
            this->setLogFlags(logTypeStr);
            std::ofstream out{ this->configName, std::ofstream::trunc };
            if (out.is_open()) {
                out << TransactionLogger::defaultData.dump(4);
            }
        }
        inline bool trySerializeConfig() {
            std::ifstream in{ this->configName };
            if (in.is_open()) {
                try {
                    auto j = nlohmann::json::parse(in);

                    std::string logTypeStr = j["log_type"].get<std::string>();
                    std::transform(logTypeStr.begin(), logTypeStr.end(), logTypeStr.begin(), [](auto c) {
                        return std::tolower(static_cast<int32_t>(c));
                    });

                    this->setLogFlags(logTypeStr);
                    return true;
                }
                catch (...) {}
            }
            return false;
        }
    };
}

inline TransactionLogger::Settings cfg{};

/**
 * @brief The entrypoint of the plugin. DO NOT remove or rename this function.
 *        
 */
void PluginInit()
{
    // Your code here
    //logger.info("Hello, world!");
    cfg.init();
}

// void ItemTransactionLogger::log(const std::string &)
THook(void,
    "?log@ItemTransactionLogger@@YAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
    const std::string &msg) {

    using LogFlag = TransactionLogger::Settings::LogFlag;
    if (cfg.logFlags & LogFlag::CONSOLE) {
        logger.info("{}", msg);
    }

    if (cfg.logFlags & LogFlag::FILE) {
        std::ofstream out1{ cfg.logName, std::ofstream::app };
        std::osyncstream out2{ out1 };
        if (out2.good()) {
            out2 << msg << '\n';
        }
    }

    original(msg);
}