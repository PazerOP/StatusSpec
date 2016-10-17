#include "common.h"

#include <regex>

CSteamID ParseSteamID(const char* input)
{
	if (!input)
		return k_steamIDNil;

	while (isspace(input[0]))
		input++;

	if (!input[0])
		return k_steamIDNil;

	std::cmatch match;
	if (std::regex_match(input, match, std::regex("\\s*STEAM_[01]:([01]):(\\d+)")))
	{
		// Classic SteamID
		return CSteamID(
			strtoul(match[2].first, nullptr, 10) * 2 + strtoul(match[1].first, nullptr, 10),
			k_EUniversePublic,
			k_EAccountTypeIndividual);
	}
	else if (std::regex_match(input, match, std::regex("\\s*\\[?\\s*[uU]\\s*:\\s*([01])\\s*:\\s*(\\d+)\\s*\\]?")))
	{
		// SteamID 3
		return CSteamID(
			strtoul(match[2].first, nullptr, 10),
			(EUniverse)strtoul(match[1].first, nullptr, 10),
			k_EAccountTypeIndividual);
	}
	else if (isdigit(input[0]))
	{
		// SteamID64
		return CSteamID(strtoull(input, nullptr, 10));
	}

	return k_steamIDNil;
}

std::string RenderSteamID(const CSteamID& id)
{
	Assert(id.BIndividualAccount());
	if (!id.BIndividualAccount())
		return std::string();

	return std::string("[U:") + std::to_string(id.GetEUniverse()) + ':' + std::to_string(id.GetAccountID()) + ']';
}