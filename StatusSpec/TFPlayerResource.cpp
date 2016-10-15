#include "TFPlayerResource.h"

#include "../src/common.h"
#include "../src/ifaces.h"
#include "../src/entities.h"

#include <cbase.h>
#include <icliententitylist.h>
#include <icliententity.h>
#include <client_class.h>
#include <ienginetool.h>
#include <c_baseentity.h>

std::shared_ptr<TFPlayerResource> TFPlayerResource::m_PlayerResource;

std::shared_ptr<TFPlayerResource> TFPlayerResource::GetPlayerResource()
{
	if (m_PlayerResource && m_PlayerResource->m_PlayerResourceEntity.Get())
		return m_PlayerResource;

	auto entityList = Interfaces::pClientEntityList;

	const auto count = entityList->GetHighestEntityIndex();
	for (int i = 0; i < count; i++)
	{
		IClientEntity* unknownEnt = entityList->GetClientEntity(i);
		if (!unknownEnt)
			continue;

		ClientClass* clClass = unknownEnt->GetClientClass();
		if (!clClass)
			continue;

		const char* name = clClass->GetName();
		if (strcmp(name, "CTFPlayerResource"))
			continue;

		m_PlayerResource = std::shared_ptr<TFPlayerResource>(new TFPlayerResource());
		m_PlayerResource->m_PlayerResourceEntity = unknownEnt->GetBaseEntity();
		return m_PlayerResource;
	}

	return nullptr;
}

bool TFPlayerResource::IsAlive(int playerEntIndex)
{
	if (!CheckEntIndex(playerEntIndex, __FUNCTION__))
		return false;

	char buffer[8];
	sprintf_s(buffer, "%.3i", playerEntIndex);

	auto result = *Entities::GetEntityProp<int*>(dynamic_cast<C_BaseEntity *>(m_PlayerResourceEntity.Get()), { "m_bAlive", buffer });
	return result == 1;
}

int TFPlayerResource::GetMaxHealth(int playerEntIndex)
{
	if (!CheckEntIndex(playerEntIndex, __FUNCTION__))
		return false;

	char buffer[8];
	sprintf_s(buffer, "%.3i", playerEntIndex);

	auto result = *Entities::GetEntityProp<int*>(dynamic_cast<C_BaseEntity*>(m_PlayerResourceEntity.Get()), { "m_iMaxHealth", buffer });
	return result;
}

bool TFPlayerResource::CheckEntIndex(int playerEntIndex, const char* functionName)
{
	if (playerEntIndex < 1 || playerEntIndex >= Interfaces::pEngineTool->GetMaxClients())
	{
		PRINT_TAG();
		Warning("Out of range playerEntIndex %i in %s()\n", playerEntIndex, functionName);
		return false;
	}

	return true;
}