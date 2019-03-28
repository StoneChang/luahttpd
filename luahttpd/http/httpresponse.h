// Created by Nijie on 2014.07.23.
// Copyright (c) 2014 Nijie. All rights reserved.
// Use of this source code is governed by a GPL-2 license that can be found in the LICENSE file. 
//
#ifndef __HTTPRESPONSE_H_NIJIE_2013_1121__
#define __HTTPRESPONSE_H_NIJIE_2013_1121__

#include <string>

using namespace std;

class HttpParser;

enum EHttpResponsesType
{
	HTTP_RESPONSE_HTTP = 0,
	HTTP_RESPONSE_CODE,
	HTTP_RESPONSE_ACCEPT_RANGES,// �����������Ƿ�֧��ָ����Χ�����������͵ķֶ����� Accept-Ranges: bytes 
	HTTP_RESPONSE_AGE,			// ��ԭʼ���������������γɵĹ���ʱ�䣨����ƣ��Ǹ��� Age: 12 
	HTTP_RESPONSE_ALLOW,		// ��ĳ������Դ����Ч��������Ϊ���������򷵻�405 Allow: GET, HEAD 
	HTTP_RESPONSE_CACHE_CONTROL,// �������еĻ�������Ƿ���Ի��漰�������� Cache-Control: no-cache 
	HTTP_RESPONSE_CONTENT_ENCODING,	// web������֧�ֵķ�������ѹ���������͡� Content-Encoding: gzip 
	HTTP_RESPONSE_CONTENT_LANGUAGE,	// ��Ӧ������� Content-Language: en,zh 
	HTTP_RESPONSE_CONTENT_LENGTH,	// ��Ӧ��ĳ��� Content-Length: 348 
	HTTP_RESPONSE_CONTENT_LOCATION,	// ������Դ������ı��õ���һ��ַ Content-Location: /index.htm 
	HTTP_RESPONSE_CONTENT_MD5,		// ������Դ��MD5У��ֵ Content-MD5: Q2hlY2sgSW50ZWdyaXR5IQ== 
	HTTP_RESPONSE_CONTENT_RANGE,	// �������������б����ֵ��ֽ�λ�� Content-Range: bytes 21010-47021/47022 
	HTTP_RESPONSE_CONTENT_TYPE,		// �������ݵ�MIME���� Content-Type: text/html; charset=utf-8 
	HTTP_RESPONSE_CONNECTION,
	HTTP_RESPONSE_DATE,			// ԭʼ��������Ϣ������ʱ�� Date: Tue, 15 Nov 2010 08:12:31 GMT 
	HTTP_RESPONSE_ETAG,			// ���������ʵ���ǩ�ĵ�ǰֵ ETag: ��737060cd8c284d8af7ad3082f209582d�� 
	HTTP_RESPONSE_EXPIRES,		// ��Ӧ���ڵ����ں�ʱ�� Expires: Thu, 01 Dec 2010 16:00:00 GMT 
	HTTP_RESPONSE_LAST_MODIFIED,// ������Դ������޸�ʱ�� Last-Modified: Tue, 15 Nov 2010 12:45:26 GMT 
	HTTP_RESPONSE_LOCATION,		// �����ض�����շ���������URL��λ�������������ʶ�µ���Դ Location: http://www.zcmhi.com/archives/94.html 
	HTTP_RESPONSE_PRAGMA,		// ����ʵ���ض���ָ�����Ӧ�õ���Ӧ���ϵ��κν��շ� Pragma: no-cache 
	HTTP_RESPONSE_PROXY_AUTHENTICATE,	// ��ָ����֤�����Ϳ�Ӧ�õ�����ĸ�URL�ϵĲ��� Proxy-Authenticate: Basic 
	HTTP_RESPONSE_REFRESH,		// Ӧ�����ض����һ���µ���Դ�����죬��5��֮���ض�����������������󲿷������֧�֣� Refresh: 5; url=http://www.zcmhi.com/archives/94.html
	HTTP_RESPONSE_RETRY_AFTER,	// ���ʵ����ʱ����ȡ��֪ͨ�ͻ�����ָ��ʱ��֮���ٴγ��� Retry-After: 120 
	HTTP_RESPONSE_SERVER,		// Web������������� Server: Apache/1.3.27 (Unix) (Red-Hat/Linux) 
	HTTP_RESPONSE_SET_COOKIE,	// ����Http Cookie Set-Cookie: UserID=JohnDoe; Max-Age=3600; Version=1 
	HTTP_RESPONSE_TRAILER,		// ָ��ͷ���ڷֿ鴫������β������ Trailer: Max-Forwards 
	HTTP_RESPONSE_TRANSFER_ENCODING,// �ļ�������� Transfer-Encoding:chunked 
	HTTP_RESPONSE_VARY,			// �������δ�����ʹ�û�����Ӧ���Ǵ�ԭʼ���������� Vary: * 
	HTTP_RESPONSE_VIA,			// ��֪����ͻ�����Ӧ��ͨ�����﷢�͵� Via: 1.0 fred, 1.1 nowhere.com (Apache/1.1) 
	HTTP_RESPONSE_WARNING,		// ����ʵ����ܴ��ڵ����� Warning: 199 Miscellaneous warning 
	HTTP_RESPONSE_X_POWERED,
	HTTP_RESPONSE_KEEP_ALIVE,
	HTTP_RESPONSE_WWW_AUTHENTICATE,	// �����ͻ�������ʵ��Ӧ��ʹ�õ���Ȩ���� WWW-Authenticate: Basic 
	HTTP_RESPONSE_DATA,			// ���ص�html��Ϣ
	HTTP_RESPONSE_SIZE,
};

static string s_arKeys[HTTP_RESPONSE_SIZE] = {
	"HTTP ",
	" ",
	"Accept-Ranges: ",
	"Age: ",
	"Allow: ",
	"Cache-Control: ",
	"Content-Encoding: ",
	"Content-Language: ",
	"Content-Length: ",
	"Content-Location: ",
	"Content-MD5: ",
	"Content-Range: ",
	"Content-Type: ",
	"Connection: ",
	"Date: ",
	"ETag: ",
	"Expires: ",
	"Last-Modified: ",
	"Location: ",
	"Pragma: ",
	"Proxy-Authenticate: ",
	"Refresh: ",
	"Retry-After: ",
	"Server: ",
	"Set-Cookie: ",
	"Trailer: ",
	"Transfer-Encoding: ",
	"Vary: ",
	"Via: ",
	"Warning: ",
	"X-Powered-By: ",
	"Keep-Alive: ",
	"WWW-Authenticate: ",
	"\r\n",
};

class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();

	bool		init(HttpParser& http);
	void		reset();
	void		set(int type, const string& str);
	void		appendResponse(const char* pdata);

	void		getText(string& str);

private:
	string		m_arValues[HTTP_RESPONSE_SIZE];
};

#endif	// __HTTPRESPONSE_H_NIJIE_2013_1121__
