/*
 *  funcs.h
 *  StatusSpec project
 *  
 *  Copyright (c) 2014 thesupremecommander
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#pragma once

#include "stdafx.h"

#include <map>

#define CLIENT_DLL
#define GLOWS_ENABLE

#include "cdll_int.h"
#include "igameevents.h"
#include "igameresources.h"
#include "vgui/vgui.h"
#include "vgui/IPanel.h"
#include "cbase.h"

#include <sourcehook_impl.h>
#include <sourcehook.h>
#include <MinHook.h>

using namespace vgui;

class C_TFPlayer;

typedef int(*GLPI_t)(void);

#if defined _WIN32
#define GetFuncAddress(pAddress, szFunction) ::GetProcAddress((HMODULE)pAddress, szFunction)
#define GetHandleOfModule(szModuleName) GetModuleHandleA(szModuleName".dll")
#elif defined __linux__
#define GetFuncAddress(pAddress, szFunction) dlsym(pAddress, szFunction)
#define GetHandleOfModule(szModuleName) dlopen(szModuleName".so", RTLD_NOLOAD)
#endif

#if defined _WIN32
#define OFFSET_GETGLOWEFFECTCOLOR 223
#define OFFSET_UPDATEGLOWEFFECT 224
#define OFFSET_DESTROYGLOWEFFECT 225
#define OFFSET_CALCVIEW 229
#define OFFSET_GETOBSERVERMODE 240
#define OFFSET_GETOBSERVERTARGET 241
#define CLIENT_MODULE_SIZE 0xC74EC0
#define GETLOCALPLAYERINDEX_SIG "\xE8\x00\x00\x00\x00\x85\xC0\x74\x08\x8D\x48\x08\x8B\x01\xFF\x60\x24\x33\xC0\xC3"
#define GETLOCALPLAYERINDEX_MASK "x????xxxxxxxxxxxxxxx"
#endif

extern SourceHook::ISourceHook *g_SHPtr;
extern int g_PLID;

class StatusSpecUnloader: public SourceHook::Impl::UnloadListener
{
public:
	virtual void ReadyToUnload(SourceHook::Plugin plug);
};

class Funcs {
public:
	static bool AddDetour_GetLocalPlayerIndex(GLPI_t detour);

	static int AddHook_C_TFPlayer_GetGlowEffectColor(C_TFPlayer *instance, void(*hook)(float *, float *, float *));
	static int AddHook_IBaseClientDLL_FrameStageNotify(IBaseClientDLL *instance, void(*hook)(ClientFrameStage_t));
	static int AddHook_IGameEventManager2_FireEvent(IGameEventManager2 *instance, bool(*hook)(IGameEvent *, bool));
	static int AddHook_IGameEventManager2_FireEventClientSide(IGameEventManager2 *instance, bool(*hook)(IGameEvent *));
	static int AddHook_IGameResources_GetPlayerName(IGameResources *instance, const char *(*hook)(int));
	static int AddHook_IPanel_PaintTraverse(vgui::IPanel *instance, void(*hook)(vgui::VPANEL, bool, bool));
	static int AddHook_IPanel_SendMessage(vgui::IPanel *instance, void(*hook)(vgui::VPANEL, KeyValues *, vgui::VPANEL));
	static int AddHook_IVEngineClient_GetPlayerInfo(IVEngineClient *instance, bool(*hook)(int, player_info_t *));

	static int CallFunc_GetLocalPlayerIndex();

	static void CallFunc_C_TFPlayer_CalcView(C_TFPlayer *instance, Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov);
	static int CallFunc_C_TFPlayer_GetObserverMode(C_TFPlayer *instance);
	static C_BaseEntity *CallFunc_C_TFPlayer_GetObserverTarget(C_TFPlayer *instance);
	static void CallFunc_C_TFPlayer_UpdateGlowEffect(C_TFPlayer *instance);
	static const char *CallFunc_IGameResources_GetPlayerName(IGameResources *instance, int client);
	static bool CallFunc_IVEngineClient_GetPlayerInfo(IVEngineClient *instance, int ent_num, player_info_t *pinfo);

	static bool RemoveDetour_GetLocalPlayerIndex();

	static bool RemoveHook(int hookID);

	static bool Load();

	static bool Unload();

	static bool Pause();

	static bool Unpause();
private:
	static GLPI_t getLocalPlayerIndexOriginal;

	static bool AddDetour(void *target, void *detour, void *&original);

	static bool RemoveDetour(void *target);
};