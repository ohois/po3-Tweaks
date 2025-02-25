#include "Experimental.h"
#include "Fixes.h"
#include "Papyrus.h"
#include "Settings.h"
#include "Tweaks.h"

void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{
			logger::info("{:*^30}", "BEGIN POST LOAD PATCH"sv);

			Fixes::PostLoad::Install();
			Tweaks::PostLoad::Install();

			Experimental::Install();
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			logger::info("{:*^30}", "BEGIN DATA LOADED PATCH"sv);

			Fixes::DataLoaded::Install();
			Tweaks::DataLoaded::Install();
		}
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;
	v.PluginVersion(Version::MAJOR);
	v.PluginName(Version::PROJECT);
	v.AuthorName("powerofthree");
	v.UsesAddressLibrary();
	v.UsesUpdatedStructs();
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();
#else
extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver <
#	ifdef SKYRIMVR
		SKSE::RUNTIME_VR_1_4_15
#	else
		SKSE::RUNTIME_1_5_39
#	endif
	) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}
#endif

void InitializeLog()
{
	auto path = logger::log_directory();
	if (!path) {
		stl::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= Version::PROJECT;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();

	logger::info("Game version : {}", a_skse->RuntimeVersion().string());

	SKSE::Init(a_skse);

	try {
		Settings::GetSingleton()->Load();
	} catch (...) {
		logger::error("Exception caught when loading settings! Default settings will be used");
	}

	logger::info("{:*^30}", "BEGIN PRELOAD PATCH"sv);

	Fixes::PreLoad::Install(a_skse->SKSEVersion());

	auto papyrus = SKSE::GetPapyrusInterface();
	papyrus->Register(Papyrus::Bind);

	auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(MessageHandler);

	return true;
}
