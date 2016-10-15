/*
 *  entities.h
 *  StatusSpec project
 *  
 *  Copyright (c) 2014-2015 Forward Command Post
 *  BSD 2-Clause License
 *  http://opensource.org/licenses/BSD-2-Clause
 *
 */

#pragma once

#include <unordered_map>
#include <string>
#include <vector>

class ClientClass;
class IClientEntity;
class RecvProp;
class RecvTable;

class Entities {
public:
	static bool RetrieveClassPropOffset(std::string className, std::vector<std::string> propertyTree);
	template <typename T> __forceinline static T GetEntityProp(IClientEntity *entity, std::vector<std::string> propertyTree) {
		return reinterpret_cast<T>(GetEntityProp(entity, propertyTree));
	};

	static bool CheckEntityBaseclass(IClientEntity *entity, const char* baseclass);

private:
	static bool GetSubProp(RecvTable *table, const char *propName, RecvProp *&prop, int &offset);
	static void *GetEntityProp(IClientEntity *entity, std::vector<std::string> propertyTree);

	static bool CheckClassBaseclass(ClientClass *clientClass, const char* baseclass);
	static bool CheckTableBaseclass(RecvTable *sTable, const char* baseclass);

	static std::unordered_map<std::string, std::unordered_map<std::string, int>> classPropOffsets;
};