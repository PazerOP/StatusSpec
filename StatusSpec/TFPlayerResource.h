#pragma once
#include <memory>
#include <ehandle.h>

class C_BaseEntity;

class TFPlayerResource final
{
	TFPlayerResource() { }

public:
	~TFPlayerResource() { }
	static std::shared_ptr<TFPlayerResource> GetPlayerResource();

	int GetMaxHealth(int playerEntIndex);
	bool IsAlive(int playerEntIndex);

private:
	bool CheckEntIndex(int playerEntIndex, const char* functionName);

	CHandle<C_BaseEntity> m_PlayerResourceEntity;
	static std::shared_ptr<TFPlayerResource> m_PlayerResource;
};