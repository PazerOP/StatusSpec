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

#include <functional>
#include <map>

#include "sourcehook.h"

#include "cdll_int.h"
#include "vgui/VGUI.h"

#include "gamedata.h"

class IClientMode;
class IGameEvent;
class IGameEventManager2;
class IMaterialSystem;

namespace vgui {
	class IPanel;
};

class Funcs {
public:
	static bool AddDetour_GetLocalPlayerIndex(GLPI_t detour);

	static int AddGlobalHook_C_TFPlayer_GetFOV(C_TFPlayer *instance, fastdelegate::FastDelegate0<float> hook, bool post);
	static int AddHook_C_BaseEntity_SetModel(std::function<void(C_BaseEntity *, const model_t *&)> hook);
	static int AddHook_IBaseClientDLL_FrameStageNotify(IBaseClientDLL *instance, fastdelegate::FastDelegate1<ClientFrameStage_t> hook, bool post);
	static int AddHook_IClientMode_DoPostScreenSpaceEffects(IClientMode *instance, fastdelegate::FastDelegate1<const CViewSetup *, bool> hook, bool post);
	static int AddHook_IGameEventManager2_FireEventClientSide(IGameEventManager2 *instance, fastdelegate::FastDelegate1<IGameEvent *, bool> hook, bool post);
	static int AddHook_IMaterialSystem_FindMaterial(IMaterialSystem *instance, fastdelegate::FastDelegate4<char const *, const char *, bool, const char *, IMaterial *> hook, bool post);
	static int AddHook_IPanel_SendMessage(vgui::IPanel *instance, fastdelegate::FastDelegate3<vgui::VPANEL, KeyValues *, vgui::VPANEL> hook, bool post);
	static int AddHook_IPanel_SetPos(vgui::IPanel *instance, fastdelegate::FastDelegate3<vgui::VPANEL, int, int> hook, bool post);
	static int AddHook_IVEngineClient_GetPlayerInfo(IVEngineClient *instance, fastdelegate::FastDelegate2<int, player_info_t *, bool> hook, bool post);

	static int CallFunc_GetLocalPlayerIndex();
	static void CallFunc_C_BaseEntity_SetModelIndex(C_BaseEntity *instance, int index);
	static void CallFunc_C_BaseEntity_SetModelPointer(C_BaseEntity *instance, const model_t *pModel);
	static void CallFunc_C_HLTVCamera_SetPrimaryTarget(C_HLTVCamera *instance, int nEntity);

	static float CallFunc_C_TFPlayer_GetFOV(C_TFPlayer *instance);
	static void CallFunc_C_TFPlayer_GetGlowEffectColor(C_TFPlayer *instance, float *r, float *g, float *b);
	static int CallFunc_C_TFPlayer_GetHealth(C_TFPlayer *instance);
	static int CallFunc_C_TFPlayer_GetMaxHealth(C_TFPlayer *instance);
	static int CallFunc_C_TFPlayer_GetObserverMode(C_TFPlayer *instance);
	static C_BaseEntity *CallFunc_C_TFPlayer_GetObserverTarget(C_TFPlayer *instance);
	static bool CallFunc_IVEngineClient_GetPlayerInfo(IVEngineClient *instance, int ent_num, player_info_t *pinfo);

	static GLPI_t GetFunc_GetLocalPlayerIndex();
	static SMI_t GetFunc_C_BaseEntity_SetModelIndex();
	static SMP_t GetFunc_C_BaseEntity_SetModelPointer();
	static SPT_t GetFunc_C_HLTVCamera_SetPrimaryTarget();

	static bool RemoveDetour_GetLocalPlayerIndex();

	static bool RemoveHook(int hookID);
	static void RemoveHook_C_BaseEntity_SetModel(int hookID);

	static bool Load();

	static bool Unload();

	static bool Pause();

	static bool Unpause();
private:
	static int setModelLastHookRegistered;
	static std::map<int, std::function<void(C_BaseEntity *, const model_t *&)>> setModelHooks;

	static GLPI_t getLocalPlayerIndexOriginal;
	static SMI_t setModelIndexOriginal;
	static SMP_t setModelPointerOriginal;

	static bool AddDetour(void *target, void *detour, void *&original);

	static bool AddDetour_C_BaseEntity_SetModelIndex(SMIH_t detour);
	static bool AddDetour_C_BaseEntity_SetModelPointer(SMPH_t detour);

	static void __fastcall Detour_C_BaseEntity_SetModelIndex(C_BaseEntity *, void *, int);
	static void __fastcall Detour_C_BaseEntity_SetModelPointer(C_BaseEntity *, void *, const model_t *);

	static bool RemoveDetour_C_BaseEntity_SetModelIndex();
	static bool RemoveDetour_C_BaseEntity_SetModelPointer();

	static bool RemoveDetour(void *target);
};

extern SourceHook::ISourceHook *g_SHPtr;