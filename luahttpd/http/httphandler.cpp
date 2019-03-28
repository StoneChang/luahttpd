// Created by Nijie on 2014.07.23.
// Copyright (c) 2014 Nijie. All rights reserved.
// Use of this source code is governed by a GPL-2 license that can be found in the LICENSE file. 
//
#include "httphandler.h"
#include "utility.h"
#include "../session/sessionmgr.h"
#include "../crypto/keymaker.h"
#include "../script/luascript.h"
#include "../ask/dbask.h"
#include "serverwork.h"
#include "../net/client.h"
#include "../ask/httpask.h"

#include <map>

using namespace std;

#define HTTP_SEND_PACKET_LEN (4 * 1024)

HttpHandler::HttpHandler()
{
	m_pSession = NULL;
	m_nCallBackCount = 0;
}

HttpHandler::~HttpHandler()
{
}

void HttpHandler::reset()
{
	m_parse.reset();
	m_response.reset();
	m_strCmd.clear();		// ���������
	m_mapRequest.clear();
	m_mapCookie.clear();
	m_mapValue.clear();
	m_nCallBackCount = 0;
	m_pSession = NULL;
}

bool HttpHandler::init(Client* pClient, const char* pBuf)
{
	if (!m_parse.parse(pBuf))
	{
		return false;
	}

	if (!m_response.init(m_parse))
	{
		return false;
	}

	string& strUrl = m_parse.get(HTTP_HEAD_GET);

	string strValues;

	int nPos = strUrl.find('?', 0);
	if (nPos == string::npos)
	{// û��get������
		m_strCmd = strUrl;
	}
	else
	{
		m_strCmd = strUrl.substr(0, nPos);
		strValues = strUrl.substr(nPos + 1);	// GET����ֵ
	}

	if (m_strCmd.empty())
	{
		m_strCmd = "index";
	}
	else if (m_strCmd[0] == '/')
	{
		m_strCmd = m_strCmd.substr(1);
		if (m_strCmd.empty())
		{
			m_strCmd = "index";
		}
	}

	// GET����
	if (!strValues.empty())
	{
		vector<string> vec;
		int count = SpiltString(strValues, '&', vec);
		for (int i = 0; i < count; ++i)
		{
			string& strGet = vec[i];
			if (strGet.empty())
			{
				continue;
			}

			nPos = strGet.find('=', 0);

			if (nPos == string::npos)
			{// û������
				continue;
			}
			else
			{
				string strK = strGet.substr(0, nPos);
				string strV = strGet.substr(nPos + 1);
				m_mapRequest[strK] = strV;
			}
		}
	}

	// POST����
	strValues = m_parse.get(HTTP_HEAD_POST);
	if (!strValues.empty())
	{
		vector<string> vec;
		int count = SpiltString(strValues, '&', vec);
		for (int i = 0; i < count; ++i)
		{
			string& strPost = vec[i];
			if (strPost.empty())
			{
				continue;
			}

			nPos = strPost.find('=', 0);

			if (nPos == string::npos)
			{// û������
				continue;
			}
			else
			{
				string strK = strPost.substr(0, nPos);
				string strV = strPost.substr(nPos + 1);
				m_mapRequest[strK] = strV;
			}
		}
	}

	// ����cookie
	string& strCookie = m_parse.get(HTTP_HEAD_COOKIE);
	if (!strCookie.empty())
	{
		const char* pChCookie = strCookie.c_str();
		char szCookie[4096] = {0};

		int index = 0;
		while (index < 4095 && *pChCookie != 0)
		{
			if (*pChCookie != ' ')
			{
				szCookie[index++] = *pChCookie;
			}
			++pChCookie;
		}

		szCookie[index] = 0;

		vector<string> vec;
		int nCookie = SpiltString(szCookie, ';', vec);

		for (int i = 0; i < nCookie; ++i)
		{
			string& str = vec[i];
			if (str.empty())
			{
				continue;
			}

			size_t nPos = str.find('=', 0);

			if (nPos == string::npos)
			{// û������
				continue;
			}
			else
			{
				string strK = str.substr(0, nPos);
				string strV = str.substr(nPos + 1);
				m_mapCookie[strK] = strV;
			}
		}
	}

	m_pClient = pClient;
	return true;
}

const char* HttpHandler::get(int type)
{
	string& strValue = m_parse.get(type);
	return strValue.c_str();
}

const char* HttpHandler::get(const char* key)
{
	map<string, string>::iterator it = m_mapRequest.find(string(key));

	if (it == m_mapRequest.end())
	{
		return "";
	}

	return it->second.c_str();
}

string& HttpHandler::getCmd()
{
	return m_strCmd;
}

void HttpHandler::appendResponse(const char* pbuf)
{
	m_response.appendResponse(pbuf);
}

void HttpHandler::setResponse(int type, const char* pBuf)
{
	m_response.set(type,pBuf);
}

void HttpHandler::getResponse(string& str)
{
	if (!m_mapCookie.empty() || m_pSession != NULL)
	{
		string strCookie;
		getCookie(strCookie);
		if (!strCookie.empty())
		{
			m_response.set(HTTP_RESPONSE_SET_COOKIE, strCookie);
		}
	}
	m_response.getText(str);
}

void HttpHandler::doing()
{
	if (m_strCmd == "favicon.ico")
	{
		return;
	}
	
	LuaScript::Instance().callFunc(*this);
}

Session* HttpHandler::getSession()
{
	if (NULL != m_pSession)
	{
		return m_pSession;
	}

	const char* pSId = getCookie("sid");
#ifdef WIN32
	uint64 sid = _strtoui64(pSId, NULL, 10);
#else
	uint64 sid = strtoull(pSId, NULL, 10);
#endif
	SKeyInfo key = DecodeKey(sid);

	m_pSession = SessionMgr::Instance().findSession(key.qKey);
	if (NULL == m_pSession)
	{
		m_pSession = SessionMgr::Instance().createSession();
	}

	return m_pSession;
}

void HttpHandler::setCookie(const char* key, const char* val)
{
	if (NULL == key)
	{
		return;
	}

	if (NULL == val)
	{// delete
		m_mapCookie.erase(key);
	}
	else
	{
		m_mapCookie[key] = val;
	}
}

const char* HttpHandler::getCookie(const char* key)
{
	if (NULL == key)
	{
		return "";
	}

	map<string, string>::iterator it = m_mapCookie.find(key);
	if (m_mapCookie.end() == it)
	{
		return "";
	}

	return it->second.c_str();
}

void HttpHandler::getCookie(string& str)
{
	if (NULL != m_pSession)
	{
		m_mapCookie["sid"] = m_pSession->id();
	}
	
	char szCookie[4096] = {0};

	bool bFirst = true;

	for (map<string, string>::iterator it = m_mapCookie.begin(); it != m_mapCookie.end(); ++it)
	{
		if (bFirst)
		{
#ifdef WIN32
			_snprintf_s(szCookie, 4095, "%s=%s", it->first.c_str(), it->second.c_str());
#else
			snprintf(szCookie, 4095, "%s=%s", it->first.c_str(), it->second.c_str());
#endif
		}
		else
		{
#ifdef WIN32
			_snprintf_s(szCookie, 4095, "\r\nSet-Cookie:%s=%s", it->first.c_str(), it->second.c_str());
#else
			snprintf(szCookie, 4095, "\r\nSet-Cookie:%s=%s", it->first.c_str(), it->second.c_str());
#endif
		}
		str += szCookie;
		bFirst = false;
	}
}

void HttpHandler::setValue(const char* pKey, const char* pVal)
{
	if (pKey == NULL)
	{
		return;
	}
	m_mapValue[pKey] = pVal;
}

const char* HttpHandler::getValue(const char* pKey)
{
	if (pKey == NULL)
	{
		return "";
	}

	map<string, string>::iterator it = m_mapValue.find(pKey);
	if (it == m_mapValue.end())
	{
		return "";
	}

	return it->second.c_str();
}

bool HttpHandler::addAsk(LuaAsk* ask, int nCallbackId /*= 0*/)
{
	if (NULL == ask)
	{
		return false;
	}

	if (0 != nCallbackId)
	{
		ask->setCallBack(nCallbackId);
	}

	ask->setHandler(this);
	unsigned char type = ask->getType();
	switch (type)
	{
	case LUA_ASK_MYSQL:
		{
			DBAsk* pDBAsk = (DBAsk*)ask;
			if (!ServerWork::Instance().addDBQuery(pDBAsk))
			{
				return false;
			}
		}
		break;

	case LUA_ASK_HTTP:
		{
			HttpAsk* pHttpAsk = (HttpAsk*)ask;
			if (!ServerWork::Instance().addHttpRequest(pHttpAsk))
			{
				return false;
			}
		}
		break;

	case LUA_ASK_SERVER:
		break;

	default:
		return false;
	}

	if (nCallbackId != 0)
	{
		incCallbackCount();
	}
	
	return true;
}

void HttpHandler::response()
{
	if (NULL != m_pSession)
	{
		m_pSession->save();
	}

	if (m_nCallBackCount > 0)
	{// ˵�����������Ļص���Ҫ����
		return;
	}

	if (NULL == m_pClient)
	{
		return;
	}

	// ˵������ľ��Ǹûص������߸�����û�лص���Ϊ��
	string str;
	getResponse(str);

	const char* ptr = str.c_str();
	int count = (int)str.size();

	for (int i = 0; i < count; i += HTTP_SEND_PACKET_LEN)
	{
		int len = count - i;
		if (len > HTTP_SEND_PACKET_LEN)
		{
			len = HTTP_SEND_PACKET_LEN;
		}
		
		while (true)
		{
			bool bRet = m_pClient->Send(ptr + i, len);
			if (bRet)
			{
				break;
			}
			else
			{
#ifdef WIN32
				Sleep(1);
#else
				usleep(1);
#endif
			}
		}
	}
	m_pClient->setTick(1); // �ӳ�1 - 2 ��tick��رգ����⻹û����ȥ�͹ر���
}

int HttpHandler::getCallbackCount()
{
	return m_nCallBackCount;
}

void HttpHandler::incCallbackCount()
{
	++m_nCallBackCount;
}

void HttpHandler::decCallbackCount()
{
	--m_nCallBackCount;
}




