/**
 *  ifaces.cpp
 *  StatusSpec project
 *  
 *  Copyright (c) 2014 thesupremecommander
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#include "ifaces.h"

#include "cbase.h"
#include "cdll_int.h"
#include "engine/ivmodelinfo.h"
#include "entitylist_base.h"
#include "filesystem_init.h"
#include "icliententitylist.h"
#include "igameevents.h"
#include "ivrenderview.h"
#include "steam/steam_api.h"
#include "teamplayroundbased_gamerules.h"
#include "tier3/tier3.h"
#include "vgui_controls/Controls.h"

#include "exceptions.h"
#include "gamedata.h"

IBaseClientDLL *Interfaces::pClientDLL = nullptr;
IClientEntityList *Interfaces::pClientEntityList = nullptr;
CDllDemandLoader *Interfaces::pClientModule = nullptr;
IVEngineClient *Interfaces::pEngineClient = nullptr;
IFileSystem *Interfaces::pFileSystem = nullptr;
IGameEventManager2 *Interfaces::pGameEventManager = nullptr;
IVModelInfoClient *Interfaces::pModelInfoClient = nullptr;
IVRenderView *Interfaces::pRenderView = nullptr;
CSteamAPIContext *Interfaces::pSteamAPIContext = nullptr;

bool Interfaces::steamLibrariesAvailable = false;
bool Interfaces::vguiLibrariesAvailable = false;

CBaseEntityList *g_pEntityList;

IClientMode *Interfaces::GetClientMode() {
#if defined _WIN32
	static DWORD pointer = NULL;

	if (!pointer) {
		pointer = SignatureScan("client", CLIENTMODE_SIG, CLIENTMODE_MASK) + CLIENTMODE_OFFSET;

		if (!pointer) {
			throw bad_pointer("IClientMode");
		}
	}

	if (!**(IClientMode***)pointer) {
		throw bad_pointer("IClientMode");
	}

	return **(IClientMode***)(pointer);
#else
	throw bad_pointer("IClientMode");

	return nullptr;
#endif
}

IGameResources *Interfaces::GetGameResources() {
#if defined _WIN32
	static DWORD pointer = NULL;

	if (!pointer) {
		pointer = SignatureScan("client", GAMERESOURCES_SIG, GAMERESOURCES_MASK);

		if (!pointer) {
			throw bad_pointer("IGameResources");
		}
	}

	GGR_t GGR = (GGR_t) pointer;
	IGameResources *gr = GGR();

	if (!gr) {
		throw bad_pointer("IGameResources");
	}

	return gr;
#else
	throw bad_pointer("IGameResources");

	return nullptr;
#endif
}

CGlobalVarsBase *Interfaces::GetGlobalVars() {
#if defined _WIN32
	static DWORD pointer = NULL;

	if (!pointer) {
		pointer = SignatureScan("client", GPGLOBALS_SIG, GPGLOBALS_MASK) + GPGLOBALS_OFFSET;

		if (!pointer) {
			throw bad_pointer("CGlobalVarsBase");
		}
	}

	if (!**(CGlobalVarsBase***)pointer) {
		throw bad_pointer("CGlobalVarsBase");
	}

	return **(CGlobalVarsBase***)(pointer);
#else
	throw bad_pointer("CGlobalVarsBase");

	return nullptr;
#endif
}

C_HLTVCamera *Interfaces::GetHLTVCamera() {
#if defined _WIN32
	static DWORD pointer = NULL;

	if (!pointer) {
		pointer = SignatureScan("client", HLTVCAMERA_SIG, HLTVCAMERA_MASK) + HLTVCAMERA_OFFSET;

		if (!pointer) {
			throw bad_pointer("C_HLTVCamera");
		}
	}

	if (!*(C_HLTVCamera**)pointer) {
		throw bad_pointer("C_HLTVCamera");
	}

	return *(C_HLTVCamera**)(pointer);
#else
	throw bad_pointer("C_HLTVCamera");

	return nullptr;
#endif
}

C_TeamplayRoundBasedRules *Interfaces::GetTeamplayRoundBasedRules() {
#if defined _WIN32
	static DWORD pointer = NULL;

	if (!pointer) {
		pointer = SignatureScan("client", TEAMPLAYROUNDBASEDRULES_SIG, TEAMPLAYROUNDBASEDRULES_MASK) + TEAMPLAYROUNDBASEDRULES_OFFSET;

		if (!pointer) {
			throw bad_pointer("C_TeamplayRoundBasedRules");
		}
	}

	if (!**(C_TeamplayRoundBasedRules***)pointer) {
		throw bad_pointer("C_TeamplayRoundBasedRules");
	}

	return **(C_TeamplayRoundBasedRules***)(pointer);
#else
	throw bad_pointer("C_TeamplayRoundBasedRules");

	return nullptr;
#endif
}

void Interfaces::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) {
	ConnectTier1Libraries(&interfaceFactory, 1);
	ConnectTier2Libraries(&interfaceFactory, 1);
	ConnectTier3Libraries(&interfaceFactory, 1);
	
	vguiLibrariesAvailable = vgui::VGui_InitInterfacesList("statusspec", &interfaceFactory, 1);
	
	pEngineClient = (IVEngineClient *)interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, nullptr);
	pGameEventManager = (IGameEventManager2 *)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2, nullptr);
	pModelInfoClient = (IVModelInfoClient *)interfaceFactory(VMODELINFO_CLIENT_INTERFACE_VERSION, nullptr);
	pRenderView = (IVRenderView *)interfaceFactory(VENGINE_RENDERVIEW_INTERFACE_VERSION, nullptr);
	
	pClientModule = new CDllDemandLoader(CLIENT_MODULE_FILE);

	CreateInterfaceFn gameClientFactory = pClientModule->GetFactory();
	
	pClientDLL = (IBaseClientDLL*)gameClientFactory(CLIENT_DLL_INTERFACE_VERSION, nullptr);
	pClientEntityList = (IClientEntityList*)gameClientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, nullptr);

	pSteamAPIContext = new CSteamAPIContext();
	steamLibrariesAvailable = SteamAPI_InitSafe() && pSteamAPIContext->Init();

	g_pEntityList = dynamic_cast<CBaseEntityList *>(Interfaces::pClientEntityList);

	char dll[MAX_PATH];
	bool steam;
	if (FileSystem_GetFileSystemDLLName(dll, sizeof(dll), steam) == FS_OK) {
		CFSLoadModuleInfo fsLoadModuleInfo;
		fsLoadModuleInfo.m_bSteam = steam;
		fsLoadModuleInfo.m_pFileSystemDLLName = dll;
		fsLoadModuleInfo.m_ConnectFactory = interfaceFactory;

		if (FileSystem_LoadFileSystemModule(fsLoadModuleInfo) == FS_OK) {
			CFSMountContentInfo fsMountContentInfo;
			fsMountContentInfo.m_bToolsMode = fsLoadModuleInfo.m_bToolsMode;
			fsMountContentInfo.m_pDirectoryName = fsLoadModuleInfo.m_GameInfoPath;
			fsMountContentInfo.m_pFileSystem = fsLoadModuleInfo.m_pFileSystem;

			if (FileSystem_MountContent(fsMountContentInfo) == FS_OK) {
				CFSSearchPathsInit fsSearchPathsInit;
				fsSearchPathsInit.m_pDirectoryName = fsLoadModuleInfo.m_GameInfoPath;
				fsSearchPathsInit.m_pFileSystem = fsLoadModuleInfo.m_pFileSystem;

				if (FileSystem_LoadSearchPaths(fsSearchPathsInit) == FS_OK) {
					Interfaces::pFileSystem = fsLoadModuleInfo.m_pFileSystem;
				}
			}
		}
	}
}

void Interfaces::Unload() {
	DisconnectTier3Libraries();
	DisconnectTier2Libraries();
	DisconnectTier1Libraries();
	
	pSteamAPIContext->Clear();

	pClientModule->Unload();
	pClientModule = nullptr;
	
	pClientDLL = nullptr;
	pClientEntityList = nullptr;
	pEngineClient = nullptr;
	pFileSystem = nullptr;
	pGameEventManager = nullptr;
	pRenderView = nullptr;
}