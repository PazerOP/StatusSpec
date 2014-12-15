/*
 *  killstreaks.h
 *  StatusSpec project
 *
 *  Copyright (c) 2014 thesupremecommander
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#pragma once

#include "../stdafx.h"

#include <map>
#include <string>

#include "../entities.h"
#include "../ifaces.h"
#include "../player.h"
#include "../tfdefs.h"

class Killstreaks {
public:
	Killstreaks();

	bool FireEventClientSideOverride(IGameEvent *event);
	void FrameHook(ClientFrameStage_t curStage);
private:
	int bluTopKillstreak;
	int bluTopKillstreakPlayer;
	std::map<int, std::map<int, int>> currentKillstreaks;
	int fireEventClientSideHook;
	int frameHook;
	EHANDLE gameResourcesEntity;
	int redTopKillstreak;
	int redTopKillstreakPlayer;

	int GetCurrentKillstreak(int userid);

	ConVar *enabled;
	ConVar *total_killfeed;
	void ToggleEnabled(IConVar *var, const char *pOldValue, float flOldValue);
};

extern Killstreaks *g_Killstreaks;