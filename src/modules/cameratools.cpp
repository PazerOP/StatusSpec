/*
 *  cameratools.cpp
 *  StatusSpec project
 *
 *  Copyright (c) 2014-2015 Forward Command Post
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#include "cameratools.h"

#include <functional>

#include "cbase.h"
#include "filesystem.h"
#include "hltvcamera.h"
#include "icliententity.h"
#include "icliententitylist.h"
#include "iclientmode.h"
#include "KeyValues.h"
#include "shareddefs.h"
#include "tier3/tier3.h"
#include "toolframework/ienginetool.h"
#include "vgui/IScheme.h"
#include "vgui/IVGui.h"
#include "vgui_controls/EditablePanel.h"
#include <characterset.h>

#include "../common.h"
#include "../exceptions.h"
#include "../funcs.h"
#include "../ifaces.h"
#include "../player.h"

class CameraTools::HLTVCameraOverride : public C_HLTVCamera {
public:
	using C_HLTVCamera::m_nCameraMode;
	using C_HLTVCamera::m_iCameraMan;
	using C_HLTVCamera::m_vCamOrigin;
	using C_HLTVCamera::m_aCamAngle;
	using C_HLTVCamera::m_iTraget1;
	using C_HLTVCamera::m_iTraget2;
	using C_HLTVCamera::m_flFOV;
	using C_HLTVCamera::m_flOffset;
	using C_HLTVCamera::m_flDistance;
	using C_HLTVCamera::m_flLastDistance;
	using C_HLTVCamera::m_flTheta;
	using C_HLTVCamera::m_flPhi;
	using C_HLTVCamera::m_flInertia;
	using C_HLTVCamera::m_flLastAngleUpdateTime;
	using C_HLTVCamera::m_bEntityPacketReceived;
	using C_HLTVCamera::m_nNumSpectators;
	using C_HLTVCamera::m_szTitleText;
	using C_HLTVCamera::m_LastCmd;
	using C_HLTVCamera::m_vecVelocity;
};

CameraTools::CameraTools()
{
	setModeHook = 0;
	setPrimaryTargetHook = 0;
	specguiSettings = new KeyValues("Resource/UI/SpectatorTournament.res");
	specguiSettings->LoadFromFile(g_pFullFileSystem, "resource/ui/spectatortournament.res", "mod");

	force_mode = new ConVar("statusspec_cameratools_force_mode", "0", FCVAR_NONE, "if a valid mode, force the camera mode to this", [](IConVar *var, const char *pOldValue, float flOldValue) { g_ModuleManager->GetModule<CameraTools>()->ChangeForceMode(var, pOldValue, flOldValue); });
	force_target = new ConVar("statusspec_cameratools_force_target", "-1", FCVAR_NONE, "if a valid target, force the camera target to this", [](IConVar *var, const char *pOldValue, float flOldValue) { g_ModuleManager->GetModule<CameraTools>()->ChangeForceTarget(var, pOldValue, flOldValue); });
	force_valid_target = new ConVar("statusspec_cameratools_force_valid_target", "0", FCVAR_NONE, "forces the camera to only have valid targets", [](IConVar *var, const char *pOldValue, float flOldValue) { g_ModuleManager->GetModule<CameraTools>()->ToggleForceValidTarget(var, pOldValue, flOldValue); });
	spec_player_alive = new ConVar("statusspec_cameratools_spec_player_alive", "1", FCVAR_NONE, "prevent speccing dead players");
	spec_pos = new ConCommand("statusspec_cameratools_spec_pos", [](const CCommand &command) { g_ModuleManager->GetModule<CameraTools>()->SpecPosition(command); }, "spec a certain camera position", FCVAR_NONE);

	spec_class = new ConCommand("statusspec_cameratools_spec_class", [](const CCommand& command) { g_ModuleManager->GetModule<CameraTools>()->SpecClass(command); }, "Spectates a specific class. statusspec_cameratools_spec_class <team> <class> [index]");
	spec_steamid = new ConCommand("statusspec_cameratools_spec_steamid", [](const CCommand& command) { g_ModuleManager->GetModule<CameraTools>()->SpecSteamID(command); }, "Spectates a player with the given steamid. statusspec_cameratools_spec_steamid <steamID>");
}

bool CameraTools::CheckDependencies() {
	bool ready = true;

	if (!Interfaces::pEngineTool) {
		PRINT_TAG();
		Warning("Required interface IEngineTool for module %s not available!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());

		ready = false;
	}

	if (!g_pFullFileSystem) {
		PRINT_TAG();
		Warning("Required interface IFileSystem for module %s not available!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());

		ready = false;
	}

	try {
		Funcs::GetFunc_C_HLTVCamera_SetCameraAngle();
	}
	catch (bad_pointer) {
		PRINT_TAG();
		Warning("Required function C_HLTVCamera::SetCameraAngle for module %s not available!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());

		ready = false;
	}

	try {
		Funcs::GetFunc_C_HLTVCamera_SetMode();
	}
	catch (bad_pointer) {
		PRINT_TAG();
		Warning("Required function C_HLTVCamera::SetMode for module %s not available!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());

		ready = false;
	}

	try {
		Funcs::GetFunc_C_HLTVCamera_SetPrimaryTarget();
	}
	catch (bad_pointer) {
		PRINT_TAG();
		Warning("Required function C_HLTVCamera::SetPrimaryTarget for module %s not available!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());

		ready = false;
	}

	if (!g_pVGuiPanel) {
		PRINT_TAG();
		Warning("Required interface vgui::IPanel for module %s not available!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());

		ready = false;
	}

	if (!g_pVGuiSchemeManager) {
		PRINT_TAG();
		Warning("Required interface vgui::ISchemeManager for module %s not available!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());

		ready = false;
	}

	if (!Player::CheckDependencies()) {
		PRINT_TAG();
		Warning("Required player helper class for module %s not available!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());

		ready = false;
	}

	if (!Player::nameRetrievalAvailable) {
		PRINT_TAG();
		Warning("Required player name retrieval for module %s not available!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());

		ready = false;
	}

	try {
		Interfaces::GetClientMode();
	}
	catch (bad_pointer) {
		PRINT_TAG();
		Warning("Module %s requires IClientMode, which cannot be verified at this time!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());
	}

	try {
		Interfaces::GetHLTVCamera();
	}
	catch (bad_pointer) {
		PRINT_TAG();
		Warning("Module %s requires C_HLTVCamera, which cannot be verified at this time!\n", g_ModuleManager->GetModuleName<CameraTools>().c_str());
	}

	return ready;
}

void CameraTools::SetModeOverride(C_HLTVCamera *hltvcamera, int &iMode) {
	int forceMode = force_mode->GetInt();

	if (forceMode == OBS_MODE_FIXED || forceMode == OBS_MODE_IN_EYE || forceMode == OBS_MODE_CHASE || forceMode == OBS_MODE_ROAMING) {
		iMode = forceMode;
	}
}

void CameraTools::SetPrimaryTargetOverride(C_HLTVCamera *hltvcamera, int &nEntity) {
	int forceTarget = force_target->GetInt();

	if (Interfaces::pClientEntityList->GetClientEntity(forceTarget)) {
		nEntity = forceTarget;
	}

	if (!Interfaces::pClientEntityList->GetClientEntity(nEntity)) {
		nEntity = ((HLTVCameraOverride *)hltvcamera)->m_iTraget1;
	}
}

void CameraTools::ChangeForceMode(IConVar *var, const char *pOldValue, float flOldValue) {
	int forceMode = force_mode->GetInt();

	if (forceMode == OBS_MODE_FIXED || forceMode == OBS_MODE_IN_EYE || forceMode == OBS_MODE_CHASE || forceMode == OBS_MODE_ROAMING) {
		if (!setModeHook) {
			setModeHook = Funcs::AddHook_C_HLTVCamera_SetMode(std::bind(&CameraTools::SetModeOverride, this, std::placeholders::_1, std::placeholders::_2));
		}

		try {
			Funcs::GetFunc_C_HLTVCamera_SetMode()(Interfaces::GetHLTVCamera(), forceMode);
		}
		catch (bad_pointer) {
			Warning("Error in setting mode.\n");
		}
	}
	else {
		var->SetValue(OBS_MODE_NONE);

		if (setModeHook) {
			Funcs::RemoveHook_C_HLTVCamera_SetMode(setModeHook);
			setModeHook = 0;
		}
	}
}

void CameraTools::ChangeForceTarget(IConVar *var, const char *pOldValue, float flOldValue) {
	int forceTarget = force_target->GetInt();

	if (Interfaces::pClientEntityList->GetClientEntity(forceTarget)) {
		if (!setPrimaryTargetHook) {
			setPrimaryTargetHook = Funcs::AddHook_C_HLTVCamera_SetPrimaryTarget(std::bind(&CameraTools::SetPrimaryTargetOverride, this, std::placeholders::_1, std::placeholders::_2));
		}

		try {
			Funcs::GetFunc_C_HLTVCamera_SetPrimaryTarget()(Interfaces::GetHLTVCamera(), forceTarget);
		}
		catch (bad_pointer) {
			Warning("Error in setting target.\n");
		}
	}
	else {
		if (!force_valid_target->GetBool()) {
			if (setPrimaryTargetHook) {
				Funcs::RemoveHook_C_HLTVCamera_SetPrimaryTarget(setPrimaryTargetHook);
				setPrimaryTargetHook = 0;
			}
		}
	}
}

void CameraTools::SpecPlayer(int playerIndex)
{
	Player* player = Player::GetPlayer(playerIndex, __FUNCSIG__);

	if (player)
	{
		if (!spec_player_alive->GetBool() || player->IsAlive())
		{
			try
			{
				Funcs::GetFunc_C_HLTVCamera_SetPrimaryTarget()(Interfaces::GetHLTVCamera(), player->GetEntity()->entindex());
			}
			catch (bad_pointer &e)
			{
				Warning("%s\n", e.what());
			}
		}
	}
}

void CameraTools::SpecClass(const CCommand& command)
{
	// Usage: <team> <class> [classIndex]
	if (command.ArgC() < 3 || command.ArgC() > 4)
	{
		PRINT_TAG();
		Warning("%s: Expected either 2 or 3 arguments\n", spec_class->GetName());
		goto Usage;
	}

	TFTeam team;
	if (!strnicmp(command.Arg(1), "blu", 3))
		team = TFTeam_Blue;
	else if (!strnicmp(command.Arg(1), "red", 3))
		team = TFTeam_Red;
	else
	{
		PRINT_TAG();
		Warning("%s: Unknown team \"%s\"\n", spec_class->GetName(), command.Arg(1));
		goto Usage;
	}

	TFClassType playerClass;
	if (!stricmp(command.Arg(2), "scout"))
		playerClass = TFClass_Scout;
	else if (!stricmp(command.Arg(2), "soldier") || !stricmp(command.Arg(2), "solly"))
		playerClass = TFClass_Soldier;
	else if (!stricmp(command.Arg(2), "pyro"))
		playerClass = TFClass_Pyro;
	else if (!strnicmp(command.Arg(2), "demo", 4))
		playerClass = TFClass_DemoMan;
	else if (!strnicmp(command.Arg(2), "heavy", 5))
		playerClass = TFClass_Heavy;
	else if (!stricmp(command.Arg(2), "engineer") || !stricmp(command.Arg(2), "engie"))
		playerClass = TFClass_Engineer;
	else if (!stricmp(command.Arg(2), "medic"))
		playerClass = TFClass_Medic;
	else if (!stricmp(command.Arg(2), "sniper"))
		playerClass = TFClass_Sniper;
	else if (!stricmp(command.Arg(2), "spy"))
		playerClass = TFClass_Spy;
	else
	{
		PRINT_TAG();
		Warning("%s: Unknown class \"%s\"\n", spec_class->GetName(), command.Arg(2));
		goto Usage;
	}

	int classIndex = -1;
	if (command.ArgC() > 3)
	{
		if (!IsInteger(command.Arg(3)))
		{
			PRINT_TAG();
			Warning("%s: class index \"%s\" is not an integer\n", spec_class->GetName(), command.Arg(3));
			goto Usage;
		}

		classIndex = atoi(command.Arg(3));
	}

	SpecClass(team, playerClass, classIndex);
	return;

Usage:
	PRINT_TAG();
	Warning("Usage: %s\n", spec_class->GetHelpText());
}

void CameraTools::SpecClass(TFTeam team, TFClassType playerClass, int classIndex)
{
	int validPlayersCount = 0;
	Player* validPlayers[MAX_PLAYERS];

	for (Player* player : Player::Iterable())
	{
		if (player->GetTeam() != team || player->GetClass() != playerClass)
			continue;

		validPlayers[validPlayersCount++] = player;
	}

	if (validPlayersCount == 0)
		return;	// Nobody to switch to

	// If classIndex was not specified, cycle through the available options
	if (classIndex < 0)
	{
		Player* localPlayer = Player::GetPlayer(Interfaces::pEngineClient->GetLocalPlayer(), __FUNCSIG__);
		if (!localPlayer)
			return;

		if (localPlayer->GetObserverMode() == OBS_MODE_FIXED ||
			localPlayer->GetObserverMode() == OBS_MODE_IN_EYE ||
			localPlayer->GetObserverMode() == OBS_MODE_CHASE)
		{
			Player* spectatingPlayer = Player::AsPlayer(localPlayer->GetObserverTarget());
			int currentIndex = -1;
			for (int i = 0; i < validPlayersCount; i++)
			{
				if (validPlayers[i] == spectatingPlayer)
				{
					currentIndex = i;
					break;
				}
			}

			classIndex = currentIndex + 1;
		}
	}

	if (classIndex < 0 || classIndex >= validPlayersCount)
		classIndex = 0;

	SpecPlayer(validPlayers[classIndex]->GetEntity()->entindex());
}

void CameraTools::SpecSteamID(const CCommand& command)
{
	characterset_t newSet;
	CharacterSetBuild(&newSet, "{}()'");	// Everything the default set has, minus the ':'
	CCommand newCommand;
	if (!newCommand.Tokenize(command.GetCommandString(), &newSet))
		return;
 
	CSteamID parsed;
	if (newCommand.ArgC() != 2)
	{
		PRINT_TAG();
		Warning("%s: Expected 1 argument\n", spec_steamid->GetName());
		goto Usage;
	}

	parsed = ParseSteamID(newCommand.Arg(1));
	if (!parsed.IsValid())
	{
		PRINT_TAG();
		Warning("%s: Unable to parse steamid\n", spec_steamid->GetName());
		goto Usage;
	}

	for (Player* player : Player::Iterable())
	{
		if (player->GetSteamID() == parsed)
			SpecPlayer(player->GetEntity()->entindex());
	}

	return;

Usage:
	PRINT_TAG();
	Warning("Usage: %s\n", spec_steamid->GetHelpText());
}

void CameraTools::SpecPosition(const CCommand &command) {
	if (command.ArgC() >= 6 && IsFloat(command.Arg(1)) && IsFloat(command.Arg(2)) && IsFloat(command.Arg(3)) && IsFloat(command.Arg(4)) && IsFloat(command.Arg(5))) {
		try {
			HLTVCameraOverride *hltvcamera = (HLTVCameraOverride *)Interfaces::GetHLTVCamera();

			hltvcamera->m_nCameraMode = OBS_MODE_FIXED;
			hltvcamera->m_iCameraMan = 0;
			hltvcamera->m_vCamOrigin.x = atof(command.Arg(1));
			hltvcamera->m_vCamOrigin.y = atof(command.Arg(2));
			hltvcamera->m_vCamOrigin.z = atof(command.Arg(3));
			hltvcamera->m_aCamAngle.x = atof(command.Arg(4));
			hltvcamera->m_aCamAngle.y = atof(command.Arg(5));
			hltvcamera->m_iTraget1 = 0;
			hltvcamera->m_iTraget2 = 0;
			hltvcamera->m_flLastAngleUpdateTime = Interfaces::pEngineTool->GetRealTime();

			Funcs::GetFunc_C_HLTVCamera_SetCameraAngle()(hltvcamera, hltvcamera->m_aCamAngle);
		}
		catch (bad_pointer &e) {
			Warning("%s\n", e.what());
		}
	}
	else {
		Warning("Usage: statusspec_cameratools_spec_pos <x> <y> <z> <yaw> <pitch>\n");

		return;
	}
}

void CameraTools::ToggleForceValidTarget(IConVar *var, const char *pOldValue, float flOldValue) {
	if (force_valid_target->GetBool()) {
		if (!setPrimaryTargetHook) {
			setPrimaryTargetHook = Funcs::AddHook_C_HLTVCamera_SetPrimaryTarget(std::bind(&CameraTools::SetPrimaryTargetOverride, this, std::placeholders::_1, std::placeholders::_2));
		}
	}
	else {
		if (!Interfaces::pClientEntityList->GetClientEntity(force_target->GetInt())) {
			if (setPrimaryTargetHook) {
				Funcs::RemoveHook_C_HLTVCamera_SetPrimaryTarget(setPrimaryTargetHook);
				setPrimaryTargetHook = 0;
			}
		}
	}
}