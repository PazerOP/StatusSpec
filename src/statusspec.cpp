/*
 *  statusspec.cpp
 *  StatusSpec project
 *  
 *  Copyright (c) 2013 thesupremecommander
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#include "statusspec.h"

inline bool DataCompare( const BYTE* pData, const BYTE* bSig, const char* szMask )
{
    for( ; *szMask; ++szMask, ++pData, ++bSig)
    {
        if( *szMask == 'x' && *pData != *bSig)
            return false;
    }
	
    return ( *szMask ) == NULL;
}

// Finds a pattern at the specified address
inline DWORD FindPattern ( DWORD dwAddress, DWORD dwSize, BYTE* pbSig, const char* szMask )
{
    for( DWORD i = NULL; i < dwSize; i++ )
    {
        if( DataCompare( (BYTE*) ( dwAddress + i ), pbSig, szMask ) )
            return (DWORD)( dwAddress + i );
    }
    return 0;
}

IGameResources* GetGameResources() {
	//IGameResources* res;
    static DWORD funcadd = NULL;
    if( !funcadd )
        funcadd = FindPattern( (DWORD) GetHandleOfModule( _T("client") ), 0x2680C6, (PBYTE) "\xA1\x00\x00\x00\x00\x85\xC0\x74\x06\x05", "x????xxxxx" );
        
    typedef IGameResources* (*GGR_t) (void);
    GGR_t GGR = (GGR_t) funcadd;
    return GGR();
}

static void icons_enabled_change(IConVar *var, const char *pOldValue, float flOldValue);
ConVar force_refresh_specgui("statusspec_force_refresh_specgui", "0", 0, "whether to force the specgui to refresh");
ConVar icons_enabled("statusspec_icons_enabled", "0", 0, "enable status icons", icons_enabled_change);
ConVar icons_dynamic("statusspec_icons_bg_dynamic", "1", 0, "dynamically move the background color with the icons");
ConVar icons_size("statusspec_icons_size", "15", 0, "square size of status icons");
ConVar icons_max("statusspec_icons_max", "5", 0, "max number of icons to be rendered");
ConVar icons_blu_x_base("statusspec_icons_blu_x_base", "160", 0, "x-coordinate of the first BLU player");
ConVar icons_blu_x_delta("statusspec_icons_blu_x_delta", "0", 0, "amount to move in x-direction for next BLU player");
ConVar icons_blu_y_base("statusspec_icons_blu_y_base", "221", 0, "y-coordinate of the first BLU player");
ConVar icons_blu_y_delta("statusspec_icons_blu_y_delta", "-15", 0, "amount to move in y-direction for next BLU player");
ConVar icons_blu_bg_red("statusspec_icons_blu_bg_red", "104", 0, "red value of the icon background for BLU players");
ConVar icons_blu_bg_green("statusspec_icons_blu_bg_green", "124", 0, "green value of the icon background for BLU players");
ConVar icons_blu_bg_blue("statusspec_icons_blu_bg_blue", "155", 0, "blue value of the icon background for BLU players");
ConVar icons_blu_bg_alpha("statusspec_icons_blu_bg_alpha", "127", 0, "alpha value of the icon background for BLU players");
ConVar icons_red_x_base("statusspec_icons_red_x_base", "160", 0, "x-coordinate of the first RED player");
ConVar icons_red_x_delta("statusspec_icons_red_x_delta", "0", 0, "amount to move in x-direction for next RED player");
ConVar icons_red_y_base("statusspec_icons_red_y_base", "241", 0, "y-coordinate of the first RED player");
ConVar icons_red_y_delta("statusspec_icons_red_y_delta", "15", 0, "amount to move in y-direction for next RED player");
ConVar icons_red_bg_red("statusspec_icons_red_bg_red", "180", 0, "red value of the icon background for RED players");
ConVar icons_red_bg_green("statusspec_icons_red_bg_green", "92", 0, "green value of the icon background for RED players");
ConVar icons_red_bg_blue("statusspec_icons_red_bg_blue", "77", 0, "blue value of the icon background for RED players");
ConVar icons_red_bg_alpha("statusspec_icons_red_bg_alpha", "127", 0, "alpha value of the icon background for RED players");

void (__fastcall *origPaintTraverse)(void* thisptr, int edx, VPANEL, bool, bool);

bool CheckCondition(uint32_t conditions[3], int condition) {
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
	
	return false;
}

void UpdateEntities() {
	int iEntCount = pEntityList->GetHighestEntityIndex();
	IClientEntity *cEntity;
	
	playerInfo.clear();

	for (int i = 0; i < iEntCount; i++) {
		cEntity = pEntityList->GetClientEntity(i);

		// Ensure valid player entity
		if (cEntity == NULL || !(GetGameResources()->IsConnected(i))) {
			continue;
		}
		
		// get our stuff directly from entity data
		int team = *MakePtr(int*, cEntity, WSOffsets::pCTFPlayer__m_iTeamNum);
		uint32_t playerCond = *MakePtr(uint32_t*, cEntity, WSOffsets::pCTFPlayer__m_nPlayerCond);
		uint32_t condBits = *MakePtr(uint32_t*, cEntity, WSOffsets::pCTFPlayer___condition_bits);
		uint32_t playerCondEx = *MakePtr(uint32_t*, cEntity, WSOffsets::pCTFPlayer__m_nPlayerCondEx);
		uint32_t playerCondEx2 = *MakePtr(uint32_t*, cEntity, WSOffsets::pCTFPlayer__m_nPlayerCondEx2);
		
		if (team != TEAM_RED && team != TEAM_BLU) {
			continue;
		}
		
		int j = playerInfo.size();
		playerInfo.push_back(Player());
		
		playerInfo[j].team = team;
		playerInfo[j].conditions[0] = playerCond|condBits;
		playerInfo[j].conditions[1] = playerCondEx;
		playerInfo[j].conditions[2] = playerCondEx2;
	}
}

void __fastcall hookedPaintTraverse( vgui::IPanel *thisPtr, int edx,  VPANEL vguiPanel, bool forceRepaint, bool allowForce = true ) {
	if (icons_enabled.GetBool()) {
		UpdateEntities();
	}
	
	origPaintTraverse(thisPtr, edx, vguiPanel, forceRepaint, allowForce);

	if (pEngineClient->IsDrawingLoadingImage() || !pEngineClient->IsInGame() || !pEngineClient->IsConnected() || pEngineClient->Con_IsVisible( ))
		return;
	
	const char* panelName = g_pVGuiPanel->GetName(vguiPanel);
	if (strcmp(panelName, "specgui") == 0) {
		specguiPanel = g_pVGui->PanelToHandle(vguiPanel);
	}
	else if (panelName[0] == 'M' && panelName[3] == 'S' && panelName[9] == 'T' && panelName[12] == 'P') {
		if (force_refresh_specgui.GetBool() && specguiPanel) {
			g_pVGuiPanel->SendMessage(g_pVGui->HandleToPanel(specguiPanel), performlayoutCommand, g_pVGui->HandleToPanel(specguiPanel));
		}
		
		if (icons_enabled.GetBool()) {
			bool bDynamic = icons_dynamic.GetBool();
			int iSize = g_pVGuiSchemeManager->GetProportionalScaledValue(icons_size.GetInt());
			int iMaxIcons = icons_max.GetInt();
			int iBluXBase = g_pVGuiSchemeManager->GetProportionalScaledValue(icons_blu_x_base.GetInt());
			int iBluXDelta = g_pVGuiSchemeManager->GetProportionalScaledValue(icons_blu_x_delta.GetInt());
			int iBluYBase = g_pVGuiSchemeManager->GetProportionalScaledValue(icons_blu_y_base.GetInt());
			int iBluYDelta = g_pVGuiSchemeManager->GetProportionalScaledValue(icons_blu_y_delta.GetInt());
			int iBluBGRed = icons_blu_bg_red.GetInt();
			int iBluBGGreen = icons_blu_bg_green.GetInt();
			int iBluBGBlue = icons_blu_bg_blue.GetInt();
			int iBluBGAlpha = icons_blu_bg_alpha.GetInt();
			int iRedXBase = g_pVGuiSchemeManager->GetProportionalScaledValue(icons_red_x_base.GetInt());
			int iRedXDelta = g_pVGuiSchemeManager->GetProportionalScaledValue(icons_red_x_delta.GetInt());
			int iRedYBase = g_pVGuiSchemeManager->GetProportionalScaledValue(icons_red_y_base.GetInt());
			int iRedYDelta = g_pVGuiSchemeManager->GetProportionalScaledValue(icons_red_y_delta.GetInt());
			int iRedBGRed = icons_red_bg_red.GetInt();
			int iRedBGGreen = icons_red_bg_green.GetInt();
			int iRedBGBlue = icons_red_bg_blue.GetInt();
			int iRedBGAlpha = icons_red_bg_alpha.GetInt();
			
			int iSpacing = g_pVGuiSchemeManager->GetProportionalScaledValue(1);
			int iIconSize = iSize - (2 * iSpacing);
			int iBarSize = iSize * iMaxIcons;
			
			int iPlayerCount = playerInfo.size();
			int iRedPlayers = -1;
			int iBluPlayers = -1;
			
			for (int i = 0; i < iPlayerCount; i++) {
				int iX = 0;
				int iY = 0;
				
				if (playerInfo[i].team == TEAM_RED) {
					iRedPlayers++;
					
					if (!bDynamic) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
						g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iBarSize, iY + iSize);
					}
				}
				else if (playerInfo[i].team == TEAM_BLU) {
					iBluPlayers++;
					
					if (!bDynamic) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
						g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iBarSize, iY + iSize);
					}
				}
				else {
					continue;
				}
				
				int iIcons = 0;
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_Ubercharged)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					
					g_pVGuiSurface->DrawSetTexture(m_iTextureUbercharged);
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_Kritzkrieged)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					
					g_pVGuiSurface->DrawSetTexture(m_iTextureCritBoosted);
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_UberBulletResist)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureResistShieldRed);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
						g_pVGuiSurface->DrawSetTexture(m_iTextureBulletResistRed);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureResistShieldBlu);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
						g_pVGuiSurface->DrawSetTexture(m_iTextureBulletResistBlu);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					
					iIcons++;
				}
				else if (CheckCondition(playerInfo[i].conditions, TFCond_SmallBulletResist)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureBulletResistRed);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureBulletResistBlu);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_UberBlastResist)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureResistShieldRed);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
						g_pVGuiSurface->DrawSetTexture(m_iTextureBlastResistRed);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureResistShieldBlu);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
						g_pVGuiSurface->DrawSetTexture(m_iTextureBlastResistBlu);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					
					iIcons++;
				}
				else if (CheckCondition(playerInfo[i].conditions, TFCond_SmallBlastResist)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureBlastResistRed);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureBlastResistBlu);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_UberFireResist)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureResistShieldRed);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
						g_pVGuiSurface->DrawSetTexture(m_iTextureFireResistRed);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureResistShieldBlu);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
						g_pVGuiSurface->DrawSetTexture(m_iTextureFireResistBlu);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					
					iIcons++;
				}
				else if (CheckCondition(playerInfo[i].conditions, TFCond_SmallFireResist)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureFireResistRed);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureFireResistBlu);
						g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
						g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					}
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_Buffed)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureBuffBannerRed);
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureBuffBannerBlu);
					}
					
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_DefenseBuffed)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureBattalionsBackupRed);
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureBattalionsBackupBlu);
					}
					
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_RegenBuffed)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureConcherorRed);
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
						g_pVGuiSurface->DrawSetTexture(m_iTextureConcherorBlu);
					}
					
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_Jarated)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					
					g_pVGuiSurface->DrawSetTexture(m_iTextureJarated);
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_Milked)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					
					g_pVGuiSurface->DrawSetTexture(m_iTextureMilked);
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_MarkedForDeath) || CheckCondition(playerInfo[i].conditions, TFCond_MarkedForDeathSilent)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					
					g_pVGuiSurface->DrawSetTexture(m_iTextureMarkedForDeath);
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_Bleeding)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					
					g_pVGuiSurface->DrawSetTexture(m_iTextureBleeding);
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
				
				if (CheckCondition(playerInfo[i].conditions, TFCond_OnFire)) {
					if (playerInfo[i].team == TEAM_RED) {
						iX = iRedXBase + (iRedPlayers * iRedXDelta) + (iIcons * iSize);
						iY = iRedYBase + (iRedPlayers * iRedYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iRedBGRed, iRedBGGreen, iRedBGBlue, iRedBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					else if (playerInfo[i].team == TEAM_BLU) {
						iX = iBluXBase + (iBluPlayers * iBluXDelta) + (iIcons * iSize);
						iY = iBluYBase + (iBluPlayers * iBluYDelta);
						if (bDynamic) {
							g_pVGuiSurface->DrawSetColor(iBluBGRed, iBluBGGreen, iBluBGBlue, iBluBGAlpha);
							g_pVGuiSurface->DrawFilledRect(iX, iY, iX + iSize, iY + iSize);
						}
					}
					
					g_pVGuiSurface->DrawSetTexture(m_iTextureOnFire);
					g_pVGuiSurface->DrawSetColor(255, 255, 255, 255);
					g_pVGuiSurface->DrawTexturedRect(iX + iSpacing, iY + iSpacing, iX + iIconSize, iY + iIconSize);
					
					iIcons++;
				}
				
				if (iIcons >= iMaxIcons) {
					continue;
				}
			}
		}
	}
}

// The plugin is a static singleton that is exported as an interface
StatusSpecPlugin g_StatusSpecPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(StatusSpecPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_StatusSpecPlugin );

StatusSpecPlugin::StatusSpecPlugin()
{
}

StatusSpecPlugin::~StatusSpecPlugin()
{
}

bool StatusSpecPlugin::Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	ConnectTier1Libraries( &interfaceFactory, 1 );
	ConnectTier2Libraries( &interfaceFactory, 1 );
	ConnectTier3Libraries( &interfaceFactory, 1 );
	
	void* hmClient = GetHandleOfModule("client");
	CreateInterfaceFn clientFactory = (CreateInterfaceFn) GetFuncAddress(hmClient, "CreateInterface");
	pClient     = (IBaseClientDLL*) clientFactory(CLIENT_DLL_INTERFACE_VERSION, NULL);
	pEntityList = (IClientEntityList*) clientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, NULL);
	
	pEngineClient = (IVEngineClient*) interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
	
	if (!(pClient && pEntityList && pEngineClient && g_pVGuiPanel && g_pVGuiSchemeManager && g_pVGuiSurface && g_pVGui))
	{
		Warning("Unable to load required libraries for %s!", PLUGIN_DESC);
		return false;
	}
	
	m_iTextureUbercharged = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureCritBoosted = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureResistShieldRed = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureResistShieldBlu = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureBulletResistRed = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureBlastResistRed = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureFireResistRed = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureBulletResistBlu = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureBlastResistBlu = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureFireResistBlu = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureBuffBannerRed = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureBuffBannerBlu = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureBattalionsBackupRed = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureBattalionsBackupBlu = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureConcherorRed = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureConcherorBlu = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureJarated = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureMilked = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureMarkedForDeath = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureBleeding = g_pVGuiSurface->CreateNewTextureID();
	m_iTextureOnFire = g_pVGuiSurface->CreateNewTextureID();
	
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureUbercharged, TEXTURE_UBERCHARGEICON, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureCritBoosted, TEXTURE_CRITBOOSTICON, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureResistShieldRed, TEXTURE_RESISTSHIELDRED, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureResistShieldBlu, TEXTURE_RESISTSHIELDBLU, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureBulletResistRed, TEXTURE_BULLETRESISTRED, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureBlastResistRed, TEXTURE_BLASTRESISTRED, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureFireResistRed, TEXTURE_FIRERESISTRED, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureBulletResistBlu, TEXTURE_BULLETRESISTBLU, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureBlastResistBlu, TEXTURE_BLASTRESISTBLU, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureFireResistBlu, TEXTURE_FIRERESISTBLU, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureBuffBannerRed, TEXTURE_BUFFBANNERRED, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureBuffBannerBlu, TEXTURE_BUFFBANNERBLU, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureBattalionsBackupRed, TEXTURE_BATTALIONSBACKUPRED, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureBattalionsBackupBlu, TEXTURE_BATTALIONSBACKUPBLU, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureConcherorRed, TEXTURE_CONCHERORRED, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureConcherorBlu, TEXTURE_CONCHERORBLU, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureJarated, TEXTURE_JARATE, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureMilked, TEXTURE_MADMILK, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureMarkedForDeath, TEXTURE_MARKEDFORDEATH, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureBleeding, TEXTURE_BLEEDING, 0, false);
	g_pVGuiSurface->DrawSetTextureFile(m_iTextureOnFire, TEXTURE_ONFIRE, 0, false);
	
	performlayoutCommand = new KeyValues("Command", "Command", "performlayout");
	
	// hook PaintTraverse
	origPaintTraverse = (void (__fastcall *)(void *, int, VPANEL, bool, bool))
	HookVFunc(*(DWORD**)g_pVGuiPanel, Index_PaintTraverse, (DWORD*) &hookedPaintTraverse);
	
	// get offsets
	WSOffsets::PrepareOffsets();
	
	// register CVars
	ConVar_Register( 0 );
	
	// ready to go
	Msg("%s loaded!\n", PLUGIN_DESC);
	return true;
}

void StatusSpecPlugin::Unload( void )
{
	ConVar_Unregister( );
	DisconnectTier3Libraries();
	DisconnectTier2Libraries();
	DisconnectTier1Libraries();
}

void StatusSpecPlugin::FireGameEvent( KeyValues * event ) {}
void StatusSpecPlugin::Pause( void ) {}
void StatusSpecPlugin::UnPause( void ) {}
const char *StatusSpecPlugin::GetPluginDescription( void ) { return PLUGIN_DESC; }
void StatusSpecPlugin::LevelInit( char const *pMapName ) {}
void StatusSpecPlugin::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) {}
void StatusSpecPlugin::GameFrame( bool simulating ) {}
void StatusSpecPlugin::LevelShutdown( void ) {}
void StatusSpecPlugin::ClientActive( edict_t *pEntity ) {}
void StatusSpecPlugin::ClientDisconnect( edict_t *pEntity ) {}
void StatusSpecPlugin::ClientPutInServer( edict_t *pEntity, char const *playername ) {}
void StatusSpecPlugin::SetCommandClient( int index ) {}
void StatusSpecPlugin::ClientSettingsChanged( edict_t *pEdict ) {}
PLUGIN_RESULT StatusSpecPlugin::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen ) { return PLUGIN_CONTINUE; }
PLUGIN_RESULT StatusSpecPlugin::ClientCommand( edict_t *pEntity, const CCommand &args ) { return PLUGIN_CONTINUE; }
PLUGIN_RESULT StatusSpecPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID ) { return PLUGIN_CONTINUE; }
void StatusSpecPlugin::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ) {}
void StatusSpecPlugin::OnEdictAllocated( edict_t *edict ) {}
void StatusSpecPlugin::OnEdictFreed( const edict_t *edict ) {}

static void icons_enabled_change(IConVar *var, const char *pOldValue, float flOldValue) {
	UpdateEntities();
}