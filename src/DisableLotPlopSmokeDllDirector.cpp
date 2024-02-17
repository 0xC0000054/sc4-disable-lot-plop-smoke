////////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-disable-lot-plop-smoke, a DLL Plugin for
// SimCity 4 that disables the smoke that is shown after plopping a lot.
//
// Copyright (c) 2024 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////////

#include "version.h"
#include "Logger.h"
#include "SC4VersionDetection.h"
#include "cIGZApp.h"
#include "cIGZCOM.h"
#include "cIGZFrameWork.h"
#include "cRZBaseString.h"
#include "cRZCOMDllDirector.h"

#include <filesystem>
#include <memory>
#include <string>
#include <Windows.h>
#include "wil/resource.h"
#include "wil/win32_helpers.h"

#include "EASTLConfigSC4.h"
#include "EASTL\vector.h"

static constexpr uint32_t kDisableLotPlopSmokeDirectorID = 0xCFE37849;

static constexpr std::string_view PluginLogFileName = "SC4DisableLotPlopSmoke.log";

namespace
{
	void OverwriteMemory(uintptr_t address, uint8_t newValue)
	{
		DWORD oldProtect;
		// Allow the executable memory to be written to.
		THROW_IF_WIN32_BOOL_FALSE(VirtualProtect(
			reinterpret_cast<LPVOID>(address),
			sizeof(newValue),
			PAGE_EXECUTE_READWRITE,
			&oldProtect));

		// Patch the memory at the specified address.
		*((uint8_t*)address) = newValue;
	}
}

class DisableLotPlopSmokeDllDirector : public cRZCOMDllDirector
{
public:

	DisableLotPlopSmokeDllDirector()
	{
		std::filesystem::path dllFolderPath = GetDllFolderPath();

		std::filesystem::path logFilePath = dllFolderPath;
		logFilePath /= PluginLogFileName;

		Logger& logger = Logger::GetInstance();
		logger.Init(logFilePath, LogLevel::Error);
		logger.WriteLogFileHeader("SC4DisableLotPlopSmoke v" PLUGIN_VERSION_STR);
	}

	uint32_t GetDirectorID() const
	{
		return kDisableLotPlopSmokeDirectorID;
	}

	bool OnStart(cIGZCOM* pCOM)
	{
		DisableLotPlopSmoke();

		return true;
	}

private:

	void DisableLotPlopSmoke()
	{
		Logger& logger = Logger::GetInstance();

		const uint16_t gameVersion = SC4VersionDetection::GetInstance().GetGameVersion();

		if (gameVersion == 641)
		{
			try
			{
				// The method used to place a lot (cSC4ViewInputControlPlaceLot::PlaceLot) takes
				// a Boolean parameter that controls whether the smoke/dust effects are used.
				//
				// It appears that this was intended to allow the PlaceLot command to skip
				// playing those effects based on the value of one of its parameters.
				//
				// When cSC4ViewInputControlPlaceLot::PlaceLot is called by the lot placement UI
				// tool it hard-codes that parameter to 1 (true), we patch the game's memory to
				// change it to 0 (false).
				OverwriteMemory(0x4C3182, 0);
				logger.WriteLine(LogLevel::Info, "Disabled the lot plop smoke.");
			}
			catch (const std::exception& e)
			{
				logger.WriteLineFormatted(
					LogLevel::Error,
					"Failed to disable the lot plop smoke: %s",
					e.what());
			}
		}
		else
		{
			logger.WriteLineFormatted(
				LogLevel::Error,
				"Unable to disable the lot plop smoke. Requires "
				"game version 641, found game version %d.",
				gameVersion);
		}

	}

	std::filesystem::path GetDllFolderPath()
	{
		wil::unique_cotaskmem_string modulePath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

		std::filesystem::path temp(modulePath.get());

		return temp.parent_path();
	}
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static DisableLotPlopSmokeDllDirector sDirector;
	return &sDirector;
}