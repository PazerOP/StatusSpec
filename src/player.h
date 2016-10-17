/*
 *  player.h
 *  StatusSpec project
 *
 *  Copyright (c) 2014-2015 Forward Command Post
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#pragma once

class C_BaseCombatWeapon;
class C_BaseEntity;
class CSteamID;
class IClientEntity;

#include "tfdefs.h"
#include "ehandle.h"
#include <shareddefs.h>
#include <memory>

class Player
{
public:
	static Player* AsPlayer(IClientEntity* entity);
	static Player* GetPlayer(int entIndex, const char* functionName = nullptr);

	bool operator==(const Player &player) const { return IsEqualTo(player); }
	bool operator!=(const Player &player) const { return IsNotEqualTo(player); }
	bool operator<(const Player &player) const { return IsLessThan(player); }
	bool operator<=(const Player &player) const { return IsLessThanOrEqualTo(player); }
	bool operator>(const Player &player) const { return IsGreaterThan(player); }
	bool operator>=(const Player &player) const { return IsGreaterThanOrEqualTo(player); }

	operator IClientEntity *() const { return playerEntity; }
	IClientEntity *operator->() const { return playerEntity; }
	IClientEntity *GetEntity() const { return playerEntity; }

	bool CheckCondition(TFCond condition) const;
	TFClassType GetClass() const;
	int GetHealth() const;
	int GetMaxHealth() const;
	std::string GetName() const;
	int GetObserverMode() const;
	C_BaseEntity *GetObserverTarget() const;
	CSteamID GetSteamID() const;
	TFTeam GetTeam() const;
	int GetUserID() const;
	C_BaseCombatWeapon *GetWeapon(int i) const;
	bool IsAlive() const;

	class Iterator
	{
		friend class Player;

	public:
		Iterator(const Iterator& old) { index = old.index; }
		~Iterator() = default;
		Iterator& operator=(const Iterator& old) { index = old.index; return *this; }
		Iterator& operator++();
		Player* operator*() const;
		friend void swap(Iterator& lhs, Iterator& rhs);
		Iterator operator++(int);
		//Player* operator->() const;
		friend bool operator==(const Iterator&, const Iterator&);
		friend bool operator!=(const Iterator&, const Iterator&);
		Iterator();
		Iterator& operator--();
		Iterator operator--(int);

	private:
		Iterator(int index);
		int index;
	};

	static Iterator begin();
	static Iterator end();

	class Iterable
	{
	public:
		Iterator begin();
		Iterator end();
	};

	static bool CheckDependencies();
	static bool classRetrievalAvailable;
	static bool comparisonAvailable;
	static bool conditionsRetrievalAvailable;
	static bool nameRetrievalAvailable;
	static bool steamIDRetrievalAvailable;
	static bool userIDRetrievalAvailable;

private:
	Player(const Player& other) { Assert(0); }
	Player& operator=(const Player &player) { Assert(0); }
	Player();

	CHandle<IClientEntity> playerEntity;
	mutable void* m_CachedPlayerEntity;

	class HLTVCameraOverride;

	void Init();
	bool CacheNeedsRefresh() const;
	mutable TFTeam* m_CachedTeam;
	mutable TFClassType* m_CachedClass;
	mutable CHandle<C_BaseCombatWeapon>* m_CachedWeapons[MAX_WEAPONS];
	mutable int* m_CachedHealth;
	mutable int* m_CachedObserverMode;
	mutable CHandle<C_BaseEntity>* m_CachedObserverTarget;

	bool IsValid() const;
	bool IsEqualTo(const Player &player) const;
	bool IsNotEqualTo(const Player &player) const { return !IsEqualTo(player); }
	bool IsLessThan(const Player &player) const;
	bool IsLessThanOrEqualTo(const Player &player) const { return IsEqualTo(player) || IsLessThan(player); }
	bool IsGreaterThan(const Player &player) const;
	bool IsGreaterThanOrEqualTo(const Player &player) const { return IsEqualTo(player) || IsGreaterThan(player); }

	static std::unique_ptr<Player> s_Players[MAX_PLAYERS];
};