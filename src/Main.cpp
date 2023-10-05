#include "Papyrus.h"
#include "Hooks.h"
#include "Hider.h"
#include "NodeHider.h"
#include "UpdateHook.h"
#include "DeviceReader.h"
#include "Settings.h"
#include <stddef.h>

#if (DD_USEINVENTORYFILTER_S == 1U)
    #include "InventoryFilter.h"
#endif

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;
 
namespace {
#if (DD_LOGENABLED == 1U)
    void InitializeLogging() {
        auto path = log_directory();
        if (!path) {
            report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path += L"/DeviousDevicesNG.log";

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        log->set_level(spdlog::level::from_str("info"));
        log->flush_on(spdlog::level::from_str("trace"));

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }
#endif
    void InitializePapyrus() {
        log::trace("Initializing Papyrus binding...");
        if (GetPapyrusInterface()->Register(DeviousDevices::RegisterFunctions)) {
            log::info("Papyrus functions bound.");
        } else {
            stl::report_and_fail("Failure to register Papyrus bindings.");
        }
    }

    void InitializeMessaging() {
        const auto g_messaging = GetMessagingInterface();
        if (!g_messaging->RegisterListener([](MessagingInterface::Message* message) {
                switch (message->type) {
                    // Skyrim lifecycle events.
                    case MessagingInterface::kPostPostLoad:  // Called after all plugins have finished running
                        break;
                    case MessagingInterface::kInputLoaded:  // Called when all game data has been found.
                        break;
                    case MessagingInterface::kDataLoaded:  // All ESM/ESL/ESP plugins have loaded, main menu is now
                                                           // active.
                        break;
                    case MessagingInterface::kPostLoadGame:  // Player's selected save game has finished loading.
                                                             // Data will be a boolean indicating whether the load was
                                                             // successful.
                    case MessagingInterface::kNewGame: //also when player makes new game, as kPostLoadGame event is called too late on new game
                        DeviousDevices::Hooks::Install();
                        DeviousDevices::DeviceReader::GetSingleton()->Setup();
                        DeviousDevices::InventoryFilter::GetSingleton()->Setup();
                        DeviousDevices::DeviceHiderManager::GetSingleton()->Setup();
                        DeviousDevices::NodeHider::GetSingleton()->Setup();
                        DeviousDevices::UpdateHook::GetSingleton()->Setup();
                        break;
                }
            })) {
            stl::report_and_fail("Unable to register message listener.");
        }
    }
}

SKSEPluginLoad(const LoadInterface* skse) {
#if (DD_LOGENABLED == 1U)
    InitializeLogging();
#endif
    auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    LOG("{} {} is loading...", plugin->GetName(), version);

    Init(skse);
    InitializePapyrus();
    InitializeMessaging();

    LOG("{} has finished loading.", plugin->GetName());
    return true;
}
