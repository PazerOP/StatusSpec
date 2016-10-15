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

	bool IsAlive(int playerEntIndex);

private:
	CHandle<C_BaseEntity> m_PlayerResourceEntity;
	static std::shared_ptr<TFPlayerResource> m_PlayerResource;
};