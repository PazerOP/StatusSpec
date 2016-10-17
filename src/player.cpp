/*
 *  player.cpp
 *  StatusSpec project
 *
 *  Copyright (c) 2014-2015 Forward Command Post
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#include "player.h"

#include <cstdint>

#include "cbase.h"
#include "c_basecombatcharacter.h"
#include "c_baseentity.h"
#include "cdll_int.h"
#include "globalvars_base.h"
#include "icliententity.h"
#include "icliententitylist.h"
#include "steam/steam_api.h"
#include "toolframework/ienginetool.h"
#include <hltvcamera.h>
#include <functional>

#include "common.h"
#include "entities.h"
#include "exceptions.h"
#include "ifaces.h"
#include "../StatusSpec/TFPlayerResource.h"

std::unique_ptr<Player> Player::s_Players[MAX_PLAYERS];
Player* Player::GetPlayer(int entIndex, const char* functionName)
{
	if (entIndex < 1 || entIndex > Interfaces::pEngineTool->GetMaxClients())
	{
		if (!functionName)
			functionName = "<UNKNOWN>";

		PRINT_TAG();
		Warning("Out of range playerEntIndex %i in %s\n", entIndex, functionName);
		return nullptr;
	}

	Assert((entIndex - 1) >= 0 && (entIndex - 1) < MAX_PLAYERS);

	Player* p = s_Players[entIndex - 1].get();
	if (!p || !p->IsValid())
	{
		IClientEntity* playerEntity = Interfaces::pClientEntityList->GetClientEntity(entIndex);
		if (!playerEntity)
			return nullptr;

		s_Players[entIndex - 1] = std::unique_ptr<Player>(p = new Player());
		p->playerEntity = playerEntity;
	}

	if (!p || !p->IsValid())
		return nullptr;

	return p;
}

Player* Player::AsPlayer(IClientEntity* entity)
{
	if (!entity)
		return nullptr;

	int entIndex = entity->entindex();
	if (entIndex >= 1 && entIndex <= Interfaces::pEngineTool->GetMaxClients())
		return GetPlayer(entity->entindex());

	return nullptr;
}

Player::Player()
{
	Init();
}

bool Player::IsEqualTo(const Player &player) const {
	if (IsValid() && player.IsValid()) {
		return playerEntity == player.playerEntity;
	}

	return false;
}

bool Player::IsLessThan(const Player &player) const {
	if (!IsValid()) {
		return true;
	}

	if (!player.IsValid()) {
		return false;
	}

	if (GetTeam() < player.GetTeam()) {
		return true;
	}
	else if (GetTeam() > player.GetTeam()) {
		return false;
	}

	if (TFDefinitions::normalClassOrdinal.find(GetClass())->second < TFDefinitions::normalClassOrdinal.find(player.GetClass())->second) {
		return true;
	}
	else if (TFDefinitions::normalClassOrdinal.find(GetClass())->second > TFDefinitions::normalClassOrdinal.find(player.GetClass())->second) {
		return false;
	}

	if (this->GetEntity()->entindex() < player.GetEntity()->entindex()) {
		return true;
	}
	else if (this->GetEntity()->entindex() > player.GetEntity()->entindex()) {
		return false;
	}

	return false;
}

bool Player::IsGreaterThan(const Player &player) const {
	if (!IsValid()) {
		return false;
	}

	if (!player.IsValid()) {
		return true;
	}

	if (GetTeam() > player.GetTeam()) {
		return true;
	}
	else if (GetTeam() < player.GetTeam()) {
		return false;
	}

	if (TFDefinitions::normalClassOrdinal.find(GetClass())->second > TFDefinitions::normalClassOrdinal.find(player.GetClass())->second) {
		return true;
	}
	else if (TFDefinitions::normalClassOrdinal.find(GetClass())->second < TFDefinitions::normalClassOrdinal.find(player.GetClass())->second) {
		return false;
	}

	if (this->GetEntity()->entindex() > player.GetEntity()->entindex()) {
		return true;
	}
	else if (this->GetEntity()->entindex() < player.GetEntity()->entindex()) {
		return false;
	}

	return false;
}

bool Player::IsValid() const
{
	if (!playerEntity.IsValid())
		return false;

	if (!playerEntity.Get())
		return false;

	if (playerEntity->entindex() < 1 || playerEntity->entindex() > Interfaces::pEngineTool->GetMaxClients())
		return false;

	// pazer: this is slow, and theoretically it should be impossible to create
	// Player objects with incorrect entity types
	//if (!Entities::CheckEntityBaseclass(playerEntity, "TFPlayer"))
	//	return false;

	return true;
}

bool Player::CheckCondition(TFCond condition) const {
	if (IsValid()) {
		uint32_t playerCond = *Entities::GetEntityProp<uint32_t *>(playerEntity.Get(), { "m_nPlayerCond" });
		uint32_t condBits = *Entities::GetEntityProp<uint32_t *>(playerEntity.Get(), { "_condition_bits" });
		uint32_t playerCondEx = *Entities::GetEntityProp<uint32_t *>(playerEntity.Get(), { "m_nPlayerCondEx" });
		uint32_t playerCondEx2 = *Entities::GetEntityProp<uint32_t *>(playerEntity.Get(), { "m_nPlayerCondEx2" });
		uint32_t playerCondEx3 = *Entities::GetEntityProp<uint32_t *>(playerEntity.Get(), { "m_nPlayerCondEx3" });

		uint32_t conditions[4];
		conditions[0] = playerCond | condBits;
		conditions[1] = playerCondEx;
		conditions[2] = playerCondEx2;
		conditions[3] = playerCondEx3;

		if (condition < 32) {
			if (conditions[0] & (1 << condition)) {
				return true;
			}
		}
		else if (condition < 64) {
			if (conditions[1] & (1 << (condition - 32))) {
				return true;
			}
		}
		else if (condition < 96) {
			if (conditions[2] & (1 << (condition - 64))) {
				return true;
			}
		}
		else if (condition < 128) {
			if (conditions[3] & (1 << (condition - 96))) {
				return true;
			}
		}
	}

	return false;
}

TFClassType Player::GetClass() const
{
	if (IsValid())
	{
		if (CacheNeedsRefresh() || !m_CachedClass)
			m_CachedClass = (TFClassType*)Entities::GetEntityProp<int*>(playerEntity.Get(), { "m_iClass" });

		if (m_CachedClass)
			return *m_CachedClass;
	}

	return TFClass_Unknown;
}

int Player::GetHealth() const
{
	if (IsValid())
	{
		if (CacheNeedsRefresh() || !m_CachedHealth)
			m_CachedHealth = Entities::GetEntityProp<int*>(playerEntity.Get(), { "m_iHealth" });

		if (m_CachedHealth)
			return *m_CachedHealth;
	}

	return 0;
}

int Player::GetMaxHealth() const
{
	if (IsValid())
	{
		auto playerResource = TFPlayerResource::GetPlayerResource();
		if (playerResource)
			return playerResource->GetMaxHealth(playerEntity.GetEntryIndex());
	}

	return 0;
}

std::string Player::GetName() const {
	if (IsValid()) {
		player_info_t playerInfo;

		if (Interfaces::pEngineClient->GetPlayerInfo(playerEntity->entindex(), &playerInfo)) {
			return playerInfo.name;
		}
	}
	
	return "";
}

int Player::GetObserverMode() const
{
	if (IsValid())
	{
		if (CacheNeedsRefresh() || !m_CachedObserverMode)
			 m_CachedObserverMode = Entities::GetEntityProp<int*>(playerEntity.Get(), { "m_iObserverMode" });

		if (m_CachedObserverMode)
			return *m_CachedObserverMode;
	}

	return OBS_MODE_NONE;
}

class Player::HLTVCameraOverride : public C_HLTVCamera
{
public:
	static C_BaseEntity* GetPrimaryTargetReimplementation()
	{
		HLTVCameraOverride* hltvCamera = (HLTVCameraOverride*)Interfaces::GetHLTVCamera();
		if (!hltvCamera)
			return nullptr;

		if (hltvCamera->m_iCameraMan > 0)
		{
			Player *pCameraMan = Player::GetPlayer(hltvCamera->m_iCameraMan, __FUNCSIG__);
			if (pCameraMan)
				return pCameraMan->GetObserverTarget();
		}

		if (hltvCamera->m_iTraget1 <= 0)
			return nullptr;

		IClientEntity* target = Interfaces::pClientEntityList->GetClientEntity(hltvCamera->m_iTraget1);
		return target ? target->GetBaseEntity() : nullptr;
	}

	using C_HLTVCamera::m_iCameraMan;
	using C_HLTVCamera::m_iTraget1;
};

C_BaseEntity *Player::GetObserverTarget() const
{
	if (IsValid())
	{
		if (Interfaces::pEngineClient->IsHLTV())
			return HLTVCameraOverride::GetPrimaryTargetReimplementation();

		if (CacheNeedsRefresh() || !m_CachedObserverTarget)
			m_CachedObserverTarget = Entities::GetEntityProp<EHANDLE*>(playerEntity.Get(), { "m_hObserverTarget" });

		if (m_CachedObserverTarget)
			return m_CachedObserverTarget->Get();
	}

	return playerEntity->GetBaseEntity();
}

bool Player::CacheNeedsRefresh() const
{
	void* current = playerEntity.Get();
	if (current != m_CachedPlayerEntity)
	{
		m_CachedPlayerEntity = current;
		return true;
	}

	return false;
}

void Player::Init()
{
	m_CachedPlayerEntity = nullptr;
	m_CachedTeam = nullptr;
	m_CachedClass = nullptr;
	m_CachedHealth = nullptr;
	m_CachedObserverMode = nullptr;
	m_CachedObserverTarget = nullptr;

	for (int i = 0; i < MAX_WEAPONS; i++)
		m_CachedWeapons[i] = nullptr;
}

CSteamID Player::GetSteamID() const {
	if (IsValid()) {
		player_info_t playerInfo;

		if (Interfaces::pEngineClient->GetPlayerInfo(playerEntity->entindex(), &playerInfo)) {
			if (playerInfo.friendsID) {
				static EUniverse universe = k_EUniverseInvalid;

				if (universe == k_EUniverseInvalid) {
					if (Interfaces::pSteamAPIContext->SteamUtils()) {
						universe = Interfaces::pSteamAPIContext->SteamUtils()->GetConnectedUniverse();
					}
					else {
						// let's just assume that it's public - what are the chances that there's a Valve employee testing this on another universe without Steam?

						PRINT_TAG();
						Warning("Steam libraries not available - assuming public universe for user Steam IDs!\n");

						universe = k_EUniversePublic;
					}
				}

				return CSteamID(playerInfo.friendsID, 1, universe, k_EAccountTypeIndividual);
			}
		}
	}

	return CSteamID();
}

TFTeam Player::GetTeam() const
{
	if (IsValid())
	{
		if (CacheNeedsRefresh() || !m_CachedTeam)
		{
			C_BaseEntity* entity = dynamic_cast<C_BaseEntity *>(playerEntity.Get());
			if (!entity)
				return TFTeam_Unassigned;

			m_CachedTeam = (TFTeam*)Entities::GetEntityProp<int*>(entity, { "m_iTeamNum" });
		}

		if (m_CachedTeam)
			return *m_CachedTeam;
	}

	return TFTeam_Unassigned;
}

int Player::GetUserID() const
{
	if (IsValid()) {
		player_info_t playerInfo;

		if (Interfaces::pEngineClient->GetPlayerInfo(playerEntity->entindex(), &playerInfo)) {
			return playerInfo.userID;
		}
	}

	return 0;
}

C_BaseCombatWeapon *Player::GetWeapon(int i) const
{
	if (i < 0 || i >= MAX_WEAPONS)
	{
		PRINT_TAG();
		Warning("Out of range index %i in %s\n", i, __FUNCTION__);
		return nullptr;
	}

	if (IsValid())
	{
		if (CacheNeedsRefresh() || !m_CachedWeapons[i])
		{
			char buffer[8];
			sprintf_s(buffer, "%.3i", i);
			m_CachedWeapons[i] = Entities::GetEntityProp<CHandle<C_BaseCombatWeapon>*>(playerEntity.Get(), { "m_hMyWeapons", buffer });
		}

		if (m_CachedWeapons[i])
			return m_CachedWeapons[i]->Get();
	}

	return nullptr;
}

bool Player::IsAlive() const
{
	if (IsValid()) 
	{
		auto playerResource = TFPlayerResource::GetPlayerResource();
		return playerResource->IsAlive(playerEntity.GetEntryIndex());
	}

	return false;
}

Player::Iterator& Player::Iterator::operator++()
{
	for (int i = index + 1; i < Interfaces::pEngineTool->GetMaxClients(); i++)
	{
		if (!GetPlayer(i) || !GetPlayer(i)->IsValid())
			continue;

		index = i;

		return *this;
	}

	index = Interfaces::pEngineTool->GetMaxClients() + 1;

	return *this;
};

void swap(Player::Iterator& lhs, Player::Iterator& rhs)
{
	std::swap(lhs.index, rhs.index);
}

Player::Iterator Player::Iterator::operator++(int)
{
	Player::Iterator current(*this);

	for (int i = index + 1; i <= Interfaces::pEngineTool->GetMaxClients(); i++) {
		if (GetPlayer(i)) {
			index = i;

			return current;
		}
	}

	index = Interfaces::pEngineTool->GetMaxClients() + 1;

	return current;
}

Player* Player::Iterator::operator*() const { return GetPlayer(index); }
//Player *Player::Iterator::operator->() const { return GetPlayer(index); }

bool operator==(const Player::Iterator& lhs, const Player::Iterator& rhs) { return lhs.index == rhs.index; }
bool operator!=(const Player::Iterator& lhs, const Player::Iterator& rhs) { return lhs.index != rhs.index; }

Player::Iterator::Iterator()
{
	for (int i = 1; i <= Interfaces::pEngineTool->GetMaxClients(); i++)
	{
		Player* p = GetPlayer(i);
		if (!p || !p->IsValid())
			continue;

		index = i;
		return;
	}

	index = Interfaces::pEngineTool->GetMaxClients() + 1;
	return;
}

Player::Iterator& Player::Iterator::operator--()
{
	for (int i = index - 1; i >= 0; i++)
	{
		Player* p = GetPlayer(i);
		if (!p || !p->IsValid())
			continue;

		index = i;
		return *this;
	}

	index = 0;
	return *this;
}

Player::Iterator Player::Iterator::operator--(int) {
	Player::Iterator current(*this);

	for (int i = index - 1; i >= 1; i++) {
		if (GetPlayer(i)) {
			index = i;

			return current;
		}
	}

	index = 0;

	return current;
}

Player::Iterator::Iterator(int startIndex) {
	index = startIndex;
}

Player::Iterator Player::begin() {
	return Player::Iterator();
}

Player::Iterator Player::end() {
	return Player::Iterator(Interfaces::pEngineTool->GetMaxClients() + 1);
}

Player::Iterator Player::Iterable::begin() {
	return Player::begin();
}

Player::Iterator Player::Iterable::end() {
	return Player::end();
}

bool Player::classRetrievalAvailable = false;
bool Player::comparisonAvailable = false;
bool Player::conditionsRetrievalAvailable = false;
bool Player::nameRetrievalAvailable = false;
bool Player::steamIDRetrievalAvailable = false;
bool Player::userIDRetrievalAvailable = false;

bool Player::CheckDependencies() {
	bool ready = true;

	if (!Interfaces::pClientEntityList) {
		PRINT_TAG();
		Warning("Required interface IClientEntityList for player helper class not available!\n");

		ready = false;
	}

	if (!Interfaces::pEngineTool) {
		PRINT_TAG();
		Warning("Required interface IEngineTool for player helper class not available!\n");

		ready = false;
	}

	classRetrievalAvailable = true;
	comparisonAvailable = true;
	conditionsRetrievalAvailable = true;
	nameRetrievalAvailable = true;
	steamIDRetrievalAvailable = true;
	userIDRetrievalAvailable = true;

	if (!Interfaces::pEngineClient) {
		PRINT_TAG();
		Warning("Interface IVEngineClient for player helper class not available (required for retrieving certain info)!\n");

		nameRetrievalAvailable = false;
		steamIDRetrievalAvailable = false;
		userIDRetrievalAvailable = false;
	}

	if (!Interfaces::GetHLTVCamera())
	{
		PRINT_TAG();
		Warning("Interface C_HLTVCamera for player helper class not available (required for retrieving spectated player)!\n");
		ready = false;
	}

	if (!Interfaces::steamLibrariesAvailable) {
		PRINT_TAG();
		Warning("Steam libraries for player helper class not available (required for accuracy in retrieving Steam IDs)!\n");
	}

	if (!Entities::RetrieveClassPropOffset("CTFPlayer", { "m_nPlayerCond" })) {
		PRINT_TAG();
		Warning("Required property m_nPlayerCond for CTFPlayer for player helper class not available!\n");

		conditionsRetrievalAvailable = false;
	}
		
	if (!Entities::RetrieveClassPropOffset("CTFPlayer", { "_condition_bits" })) {
		PRINT_TAG();
		Warning("Required property _condition_bits for CTFPlayer for player helper class not available!\n");

		conditionsRetrievalAvailable = false;
	}
		
	if (!Entities::RetrieveClassPropOffset("CTFPlayer", { "m_nPlayerCondEx" })) {
		PRINT_TAG();
		Warning("Required property m_nPlayerCondEx for CTFPlayer for player helper class not available!\n");

		conditionsRetrievalAvailable = false;
	}
			
	if (!Entities::RetrieveClassPropOffset("CTFPlayer", { "m_nPlayerCondEx2" })) {
		PRINT_TAG();
		Warning("Required property m_nPlayerCondEx2 for CTFPlayer for player helper class not available!\n");

		conditionsRetrievalAvailable = false;
	}

	if (!Entities::RetrieveClassPropOffset("CTFPlayer", { "m_nPlayerCondEx3" })) {
		PRINT_TAG();
		Warning("Required property m_nPlayerCondEx3 for CTFPlayer for player helper class not available!\n");

		conditionsRetrievalAvailable = false;
	}

	if (!Entities::RetrieveClassPropOffset("CTFPlayer", { "m_iClass" })) {
		PRINT_TAG();
		Warning("Required property m_iClass for CTFPlayer for player helper class not available!\n");

		classRetrievalAvailable = false;
		comparisonAvailable = false;
	}

	return ready;
}