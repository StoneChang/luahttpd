// Created by Nijie on 2014.07.23.
// Copyright (c) 2014 Nijie. All rights reserved.
// Use of this source code is governed by a GPL-2 license that can be found in the LICENSE file. 
//
#include "session.h"
#include "exelogger.h"
#include <string>
#include "sessionmgr.h"
#include "utility.h"
#include "../crypto/keymaker.h"

using namespace std;

#define UPDATE_INTERVAL_SESSION (7 * 24 * 60 * 60 * 1000)	// 1������
#define MAX_SESSION_VALUE_LEN (1024 * 16)

Session::Session()
{
	reset();
}

Session::~Session()
{

}

bool Session::init(uint64 sid)
{
	m_sid = sid;

	m_interval = UPDATE_INTERVAL_SESSION;
	m_bDirty = false;
	return true;
}

void Session::reset()
{
	m_value.clear();
	m_sid = 0;
	m_interval = UPDATE_INTERVAL_SESSION;		// ����ʱ����
	m_bDirty = false;	// �Ƿ���������
}

uint64 Session::getSId()
{
	return m_sid;
}

bool Session::loadData(const char* pData, unsigned int len)
{
	if (len >= sizeof(SSessionTime))
	{
		SSessionTime* pTime = (SSessionTime*)pData;
		m_uDestroyTime = pTime->uDestroyTime;
	}

	if (len > sizeof(SSessionTime))
	{
		const char* pJsonData = pData + sizeof(SSessionTime);
		Json::Reader reader;
		if (!reader.parse(string(pJsonData), m_value))
		{
			SYS_CRITICAL("load json data [%s] failed!", pJsonData);
			return false;
		}
	}

	return true;
}

string Session::getData()
{
	return m_value.toStyledString();
}

unsigned int Session::getDestroyTime()
{
	return m_uDestroyTime;
}

void Session::setDestroyTime(unsigned int uTime)
{
	m_uDestroyTime = uTime;

	m_interval = UPDATE_INTERVAL_SESSION;
	m_bDirty = true;
}

void Session::setKeepTime(unsigned int uTime)
{
	setDestroyTime(uTime + GetCurTime());
}

bool Session::canClear(unsigned int interval)
{
	m_interval -= interval;

	if (m_interval <= 0)
	{
		return true;
	}

	return false;
}

bool Session::set(const char* pKey, const char* pVal)
{
	if (NULL == pKey)
	{
		return false;
	}

	if (NULL == pVal)
	{// ��Ч��ɾ��
		if (m_value.isMember(pKey))
		{
			m_value.removeMember(pKey);
		}
	}
	else
	{
		if (m_value[pKey] == pVal)
		{
			return true;
		}
		
		m_value[pKey] = pVal;
	}

	m_interval = UPDATE_INTERVAL_SESSION;
	m_bDirty = true;
	return true;
}

const char* Session::get(const char* pKey)
{
	if (m_value.isMember(pKey))
	{
		static char szbuff[MAX_SESSION_VALUE_LEN] = {0};
		const string& str = m_value[pKey].asString();
		int nSize = (int)str.size();
		memcpy(szbuff, str.c_str(), nSize);
		szbuff[nSize] = 0;
		return szbuff;
	}

	return "";
}

void Session::save()
{
	if (!m_bDirty)
	{
		return;
	}

	SessionMgr::Instance().save(this);

	m_bDirty = false;
}

unsigned int Session::getId()
{
	SKeyInfo key;
	key.qKey = m_sid;
	return key.nKey[1];
}

const char* Session::id()
{
	SKeyInfo key = EncodeKey(m_sid);
	static char s_szKey[32] = {0};
	sprintf(s_szKey, "%llu", key.qKey);
	return s_szKey;
}
