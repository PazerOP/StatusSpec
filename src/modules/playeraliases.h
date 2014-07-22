/*
 *  playeraliases.h
 *  StatusSpec project
 *  
 *  Copyright (c) 2014 thesupremecommander
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#pragma once

#include "../stdafx.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>

#include "convar.h"

#undef null
#include "json/json.h"

#include "../funcs.h"
#include "../ifaces.h"

#if defined _WIN32
#define strtoull _strtoui64
#endif

#define ETF2L_PLAYER_API_URL "http://api.etf2l.org/player/%llu.json"
#define MAX_URL_LENGTH 2048

typedef enum APIStatus_s {
	API_UNKNOWN,
	API_REQUESTED,
	API_FAILED,
	API_NO_RESULT,
	API_SUCCESSFUL
} APIStatus_t;

class PlayerAliases;

typedef struct APIAlias_s {
	std::string name;
	APIStatus_t status = API_UNKNOWN;
	CCallResult<PlayerAliases, HTTPRequestCompleted_t> call;
} APIAlias_t;

class PlayerAliases {
public:
	PlayerAliases();

	bool IsEnabled();

	bool GetPlayerInfoOverride(int ent_num, player_info_t *pinfo);

	bool GetAlias(CSteamID player, std::string &alias);
	void GetETF2LPlayerInfo(HTTPRequestCompleted_t *requestCompletionInfo, bool bIOFailure);
	void RequestETF2LPlayerInfo(CSteamID player);
private:
	std::map<CSteamID, std::string> customAliases;
	std::map<CSteamID, APIAlias_t> etf2lAliases;

	ConVar *enabled;
	ConVar *etf2l;
	ConCommand *get;
	ConCommand *remove;
	ConCommand *set;
	static int GetCurrentAliasedPlayers(const char *partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
	static int GetCurrentGamePlayers(const char *partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
	static void GetCustomPlayerAlias(const CCommand &command);
	static void RemoveCustomPlayerAlias(const CCommand &command);
	static void SetCustomPlayerAlias(const CCommand &command);
};

extern PlayerAliases *g_PlayerAliases;