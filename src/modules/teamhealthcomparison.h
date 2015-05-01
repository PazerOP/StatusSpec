/*
 *  teamhealthcomparison.h
 *  StatusSpec project
 *
 *  Copyright (c) 2014 thesupremecommander
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#pragma once

#include "cdll_int.h"

#include "../modules.h"
#include "../tfdefs.h"

class ConCommand;
class ConVar;
class IConVar;

namespace vgui {
	class Panel;
}

class TeamHealthComparison : public Module {
public:
	TeamHealthComparison(std::string name);

	static bool CheckDependencies(std::string name);
private:
	class Panel;
	Panel *panel;

	ConVar *enabled;
	ConCommand *reload_settings;
	void ReloadSettings();
	void ToggleEnabled(IConVar *var, const char *pOldValue, float flOldValue);
};