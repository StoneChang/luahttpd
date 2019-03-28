// Created by Nijie on 2014.07.23.
// Copyright (c) 2014 Nijie. All rights reserved.
// Use of this source code is governed by a GPL-2 license that can be found in the LICENSE file. 
//

#include "luascript.h"
#include "exelogger.h"
#include "tolua++.h"
#include <string>
#include "../http/httphandler.h"
#include "exelogger.h"
#include "config.h"
#include "utility.h"
#include "../ask/dbask.h"
#include "../ask/httpask.h"
#include "../ask/serverask.h"

#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <dirent.h>
#endif

using namespace std;

#define FILE_PATH 512
#define PRINT_BUFF_SIZE (1024 * 1024)	// 1m

int tolua_luaexport_open(lua_State*);
int	tolua_apiexport_open(lua_State*);

LuaScript::LuaScript()
{

}

LuaScript::~LuaScript()
{

}

LuaScript& LuaScript::Instance()
{
	static LuaScript o;
	return o;
}

bool LuaScript::init()
{
	m_mapFunc.clear();

	// ��ʼ��Lua��ջ��״̬��
	m_luaState = luaL_newstate();

	if (NULL == m_luaState)
	{
		return false;
	}

	static char s_print_buff[PRINT_BUFF_SIZE];

	luaL_setprintbuff(m_luaState, s_print_buff, PRINT_BUFF_SIZE);

	luaL_openlibs(m_luaState);

	// ����tolua++ ʵ�ֵĵ�������
	tolua_luaexport_open(m_luaState);
	tolua_apiexport_open(m_luaState);

	// ���ؽű�
	string& str = Config::Instance().getDir();

	if (!__travel(str.c_str()))
	{
		SYS_CRITICAL("Load Lua scipt Failed!");
		return false;
	}

	return true;
}

void LuaScript::uninit()
{
	lua_close(m_luaState);
	m_luaState = NULL;
}

void LuaScript::__report(int status)
{
	if (0 != status && !lua_isnil(m_luaState, -1))
	{
		const char* szMsg = lua_tostring(m_luaState, -1);
		if (szMsg == NULL)
		{
			szMsg = "(error object is not a string)";
		}

		SYS_CRITICAL("%s", szMsg);
		lua_pop(m_luaState, 1);
	}
}

#ifdef WIN32
bool LuaScript::__travel(const char* pszPath)
{
	if (NULL == pszPath)
	{
		return false;
	}

	_finddata_t file;
	intptr_t handle;

	chdir(pszPath);

	handle = _findfirst("*", &file);
	if (-1 == handle)
	{
		chdir("..");
		return false;
	}

	do
	{
		string strName = file.name;
		int fileNameLen = (int)strName.size();

		if("." == strName || ".." == strName)
		{
			continue;
		}

		if(file.attrib & _A_SUBDIR)
		{
			if (!__travel(file.name))
			{
				return false;
			}
		}
		else
		{
			if (4 >= fileNameLen)
			{
				continue;
			}

			string strFile = strName.substr(fileNameLen - 3, fileNameLen);
			string strModuleName = strName.substr(0, fileNameLen - 4);
			if ("lua" == strFile)
			{
				if (false == __loadModule(strModuleName.c_str()))
				{
					SYS_CRITICAL("__LoadModule \"%s\", File \"%s\" fail.", strModuleName.c_str(), strName.c_str());
					return false;
				}
			}
		}
	}
	while(!(_findnext(handle, &file)));

	_findclose(handle);
	chdir("..");
	return true;
}
#else
bool LuaScript::__travel(const char* pszPath)
{
	if (NULL == pszPath)
	{
		return false;
	}

	struct stat s;
	DIR* dir;
	struct dirent* dt;
	string strPath = pszPath;

	if (lstat(strPath.c_str(), &s) < 0)
	{
		printf("lstat error\n");
		return false;
	}

	if (S_ISDIR(s.st_mode))
	{
		strPath += "/";
		dir = opendir(strPath.c_str());
		if (NULL == dir)
		{
			return false;
		}

		if (chdir(strPath.c_str()) < 0)
		{
			return false;
		}

		while ((dt = readdir(dir)) != NULL)
		{
			string strName = dt->d_name;
			if ("." == strName || ".." == strName)
			{
				continue;
			}

			string strNewPath = strPath + strName;
			if (stat(strNewPath.c_str(), &s) < 0)
			{
				if (4 >= strName.size())
				{
					continue;
				}

				string strExt = strName.substr(strName.size() - 3, strName.size());
				string strModule = strName.substr(0, strName.size() - 4);
				if ("lua" == strExt)
				{
					if (false == __loadModule(strModule.c_str()))
					{
						SYS_CRITICAL("__LoadModule \"%s\", File \"%s\" fail.", strModule.c_str(), strPath.c_str());
						return false;
					}
				}
				continue;
			}

			if (S_ISDIR(s.st_mode))
			{
				if (!__travel(strNewPath.c_str()))
				{
					return false;
				}
			}
			else
			{
				if (4 >= strName.size())
				{
					continue;
				}

				string strExt = strName.substr(strName.size() - 3, strName.size());
				string strModule = strName.substr(0, strName.size() - 4);
				if ("lua" == strExt)
				{
					if (false == __loadModule(strModule.c_str()))
					{
						SYS_CRITICAL("__LoadModule \"%s\", File \"%s\" fail.", strModule.c_str(), strPath.c_str());
						return false;
					}
				}
			}
		}
	}

	chdir("..");
	return true;
}
#endif

bool LuaScript::__loadModule(const char* pszName)
{
	char szBuf[FILE_PATH] = {0};
	sprintf(szBuf, "require \"%s\"", pszName);

	int nStatus = luaL_dostring(m_luaState, szBuf);
	if (nStatus)
	{
		__report(nStatus);
		return false;
	}
	return true;
}

// ����lua�ű�
// ����ֵ��ʾ�Ƿ������ȷ
bool LuaScript::callFunc(HttpHandler& handler)
{
	string strCmd = handler.getCmd();

	int nFuncId = 0;

	map<string, int>::iterator it = m_mapFunc.find(strCmd.c_str());
	if (m_mapFunc.end() == it)
	{
		vector<string> vecClass;
		int nCount = SpiltString(strCmd, '.', vecClass);
		if (0 == nCount)
		{
			SYS_CRITICAL("Can find Effect Function %s", strCmd.c_str());

			// 404 ����
			handler.setResponse(HTTP_RESPONSE_CODE, "404 Not Found");
			handler.appendResponse("404 Lua Script Not Find");
			handler.response();
			return false;
		}

		lua_getglobal(m_luaState, vecClass[0].c_str());        // effect����
		for (int i = 1; i < nCount; ++i)
		{
			if (!lua_istable(m_luaState, -1))
			{
				lua_pop(m_luaState, i);
				SYS_CRITICAL("Can find Effect Function %s", strCmd.c_str());

				// 404 ����
				handler.setResponse(HTTP_RESPONSE_CODE, "404 Not Found");
				handler.appendResponse("404 Lua Script Not Find");
				handler.response();
				return false;
			}
			lua_getfield(m_luaState, -1, vecClass[i].c_str());
		}

		if (!lua_isfunction(m_luaState, -1))
		{
			lua_pop(m_luaState, nCount);
			SYS_CRITICAL("Can find Effect Function %s", strCmd.c_str());

			// 404 ����
			handler.setResponse(HTTP_RESPONSE_CODE, "404 Not Found");
			handler.appendResponse("404 Lua Script Not Find");
			handler.response();
			return false;
		}

		// lua_pushvalue(m_luaState, -1);  
		nFuncId = luaL_ref(m_luaState, LUA_REGISTRYINDEX);
		m_mapFunc[strCmd] = nFuncId;
		if (nCount > 1)
		{
			lua_pop(m_luaState, nCount - 1);
		}
	}
	else
	{
		nFuncId = it->second;
	}

	// ��������ֵ���ҵ���Ӧ��Lua���󣨺�����
	lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, nFuncId);	// fun

	// �������
	tolua_pushusertype(m_luaState, &handler, "HttpHandler");

	luaL_resetprintbuff(m_luaState); // ��ӡ��������

	// ����Lua����
	int nStatus = lua_pcall(m_luaState, 1, 0, 0);
	if (nStatus)
	{
		// ���ô��󣬱���
		__report(nStatus);
		handler.appendResponse("Error!");
		handler.response();
		return false;
	}

	// ȡ�÷���ֵ
	// �޷���ֵ

	// ȡ�ṹ
	handler.appendResponse(luaL_getprintbuff(m_luaState));
	handler.response();

	return true;
}

bool LuaScript::callBackFunc(HttpHandler& handler, LuaAsk& ask)
{
	int nFuncId = ask.getCallBack();
	if (0 == nFuncId)
	{
		return false;
	}

	// ��������ֵ���ҵ���Ӧ��Lua���󣨺�����
	lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, nFuncId);	// fun

	// �������
	tolua_pushusertype(m_luaState, &handler, "HttpHandler");
	switch (ask.getType())
	{
	case LUA_ASK_MYSQL:
		tolua_pushusertype(m_luaState, (DBAsk*)&ask, "DBAsk");
		break;

	case LUA_ASK_HTTP:
		tolua_pushusertype(m_luaState, (HttpAsk*)&ask, "HttpAsk");
		break;

	case LUA_ASK_SERVER:
		tolua_pushusertype(m_luaState, (ServerAsk*)&ask, "ServerAsk");
		break;

	default:
		lua_pushnil(m_luaState);
		break;
	}

	luaL_resetprintbuff(m_luaState); // ��ӡ��������

	if (nFuncId != 0)
	{
		handler.decCallbackCount();	// Ҫ���������Ϊ�����ڻص����滹���������µĻص��¼�
	}
	// ����Lua����
	int nStatus = lua_pcall(m_luaState, 2, 0, 0);
	if (nStatus)
	{
		// ���ô��󣬱���
		__report(nStatus);
		handler.appendResponse("Error!");
		handler.response();
		return false;
	}

	// ��������
	luaL_unref(m_luaState, LUA_REGISTRYINDEX, nFuncId);

	// ȡ�ṹ
	handler.appendResponse(luaL_getprintbuff(m_luaState));
	handler.response();
	return true;
}
