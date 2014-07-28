/* Copyright (C) 2013 J.F.Dockes
 *	 This program is free software; you can redistribute it and/or modify
 *	 it under the terms of the GNU General Public License as published by
 *	 the Free Software Foundation; either version 2 of the License, or
 *	 (at your option) any later version.
 *
 *	 This program is distributed in the hope that it will be useful,
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	 GNU General Public License for more details.
 *
 *	 You should have received a copy of the GNU General Public License
 *	 along with this program; if not, write to the
 *	 Free Software Foundation, Inc.,
 *	 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include "config.h"

#include <string.h>

#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <set>
using std::string;
using std::cerr;
using std::endl;
using std::map;
using std::vector;
using std::set;

#include <upnp/ixml.h>

#include "upnpp_p.hxx"
#include "upnpplib.hxx"
extern "C" {
#include "getsyshwaddr.h"
}
#include "md5.hxx"
#include "log.hxx"

static LibUPnP *theLib;

LibUPnP *LibUPnP::getLibUPnP(bool serveronly, string* hwaddr,
							 const string ifname, const string ip,
							 unsigned short port)
{
	if (theLib == 0)
		theLib = new LibUPnP(serveronly, hwaddr, ifname, ip, port);
	if (theLib && !theLib->ok()) {
		delete theLib;
		theLib = 0;
		return 0;
	}
	return theLib;
}


LibUPnP::LibUPnP(bool serveronly, string* hwaddr,
				 const string ifname, const string inip, unsigned short port)
	: m_ok(false)
{
	LOGDEB("LibUPnP: serveronly " << serveronly << " &hwaddr " << hwaddr <<
		   " ifname [" << ifname << "] inip [" << inip << "] port " << port 
		   << endl);

	// If our caller wants to retrieve an ethernet address (typically
	// for uuid purposes), or has specified an interface we have to
	// look at the network config.
	const int ipalen(100);
	char ip_address[ipalen];
	ip_address[0] = 0;
	if (hwaddr || !ifname.empty()) {
		char mac[20];
		if (getsyshwaddr(ifname.c_str(), ip_address, ipalen, mac, 13) < 0) {
			LOGERR("LibUPnP::LibUPnP: failed retrieving addr" << endl);
			return;
		}
		if (hwaddr)
			*hwaddr = string(mac);
	}

	// If the interface name was not specified, we possibly use the
	// supplied IP address.
	if (ifname.empty())
		strncpy(ip_address, inip.c_str(), ipalen);

	m_init_error = UpnpInit(ip_address[0] ? ip_address : 0, port);

	if (m_init_error != UPNP_E_SUCCESS) {
		LOGERR(errAsString("UpnpInit", m_init_error) << endl);
		return;
	}
	setMaxContentLength(2000*1024);

	LOGDEB("Using IP " << UpnpGetServerIpAddress() << " port " << 
		   UpnpGetServerPort() << endl);

#if defined(HAVE_UPNPSETLOGLEVEL)
	UpnpCloseLog();
#endif

	// Client initialization is simple, just do it. Defer device
	// initialization because it's more complicated.
	if (serveronly) {
		m_ok = true;
	} else {
		m_init_error = UpnpRegisterClient(o_callback, (void *)this, &m_clh);
		
		if (m_init_error == UPNP_E_SUCCESS) {
			m_ok = true;
		} else {
			LOGERR(errAsString("UpnpRegisterClient", m_init_error) << endl);
		}
	}

	// Servers sometimes make errors (e.g.: minidlna returns bad utf-8).
	ixmlRelaxParser(1);
}

int LibUPnP::setupWebServer(const string& description)
{
	int res = UpnpRegisterRootDevice2(
		UPNPREG_BUF_DESC,
		description.c_str(), 
		description.size(), /* Desc filename len, ignored */
		1, /* URLBase*/
		o_callback, (void *)this, &m_dvh);

	if (res != UPNP_E_SUCCESS) {
		LOGERR(errAsString("UpnpRegisterRootDevice2", m_init_error) << endl);
	}
	return res;
}

void LibUPnP::setMaxContentLength(int bytes)
{
	UpnpSetMaxContentLength(bytes);
}

bool LibUPnP::setLogFileName(const std::string& fn, LogLevel level)
{
	PTMutexLocker lock(m_mutex);
	if (fn.empty() || level == LogLevelNone) {
#if defined(HAVE_UPNPSETLOGLEVEL)
		UpnpCloseLog();
#endif
	} else {
#if defined(HAVE_UPNPSETLOGLEVEL)
		setLogLevel(level);
		UpnpSetLogFileNames(fn.c_str(), fn.c_str());
		int code = UpnpInitLog();
		if (code != UPNP_E_SUCCESS) {
			LOGERR(errAsString("UpnpInitLog", code) << endl);
			return false;
		}
#endif
	}
	return true;
}

bool LibUPnP::setLogLevel(LogLevel level)
{
#if defined(HAVE_UPNPSETLOGLEVEL)
	switch (level) {
	case LogLevelNone: 
		setLogFileName("", LogLevelNone);
		break;
	case LogLevelError: 
		UpnpSetLogLevel(UPNP_CRITICAL);
		break;
	case LogLevelDebug: 
		UpnpSetLogLevel(UPNP_ALL);
		break;
	}
#endif
	return true;
}

void LibUPnP::registerHandler(Upnp_EventType et, Upnp_FunPtr handler,
							  void *cookie)
{
	PTMutexLocker lock(m_mutex);
	if (handler == 0) {
		m_handlers.erase(et);
	} else {
		Handler h(handler, cookie);
		m_handlers[et] = h;
	}
}

std::string LibUPnP::errAsString(const std::string& who, int code)
{
	std::ostringstream os;
	os << who << " :" << code << ": " << UpnpGetErrorMessage(code);
	return os.str();
}

int LibUPnP::o_callback(Upnp_EventType et, void* evp, void* cookie)
{
	LibUPnP *ulib = (LibUPnP *)cookie;
	if (ulib == 0) {
		// Because the asyncsearch calls uses a null cookie.
		//cerr << "o_callback: NULL ulib!" << endl;
		ulib = theLib;
	}
	PLOGDEB("LibUPnP::o_callback: event type: %s\n",evTypeAsString(et).c_str());

	map<Upnp_EventType, Handler>::iterator it = ulib->m_handlers.find(et);
	if (it != ulib->m_handlers.end()) {
		(it->second.handler)(et, evp, it->second.cookie);
	}
	return UPNP_E_SUCCESS;
}

LibUPnP::~LibUPnP()
{
	int error = UpnpFinish();
	if (error != UPNP_E_SUCCESS) {
		PLOGINF("%s\n", errAsString("UpnpFinish", error).c_str());
	}
	PLOGDEB("LibUPnP: done\n");
}

string LibUPnP::makeDevUUID(const std::string& name, const std::string& hw)
{
	string digest;
	MD5String(name, digest);
	// digest has 16 bytes of binary data. UUID is like:
	// f81d4fae-7dec-11d0-a765-00a0c91e6bf6
	// Where the last 12 chars are provided by the hw addr

	char uuid[100];
	sprintf(uuid, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%s",
			digest[0]&0xff, digest[1]&0xff, digest[2]&0xff, digest[3]&0xff,
			digest[4]&0xff, digest[5]&0xff,  digest[6]&0xff, digest[7]&0xff,
			digest[8]&0xff, digest[9]&0xff, hw.c_str());
	return uuid;
}


string LibUPnP::evTypeAsString(Upnp_EventType et)
{
	switch (et) {
	case UPNP_CONTROL_ACTION_REQUEST: return "UPNP_CONTROL_ACTION_REQUEST";
	case UPNP_CONTROL_ACTION_COMPLETE: return "UPNP_CONTROL_ACTION_COMPLETE";
	case UPNP_CONTROL_GET_VAR_REQUEST: return "UPNP_CONTROL_GET_VAR_REQUEST";
	case UPNP_CONTROL_GET_VAR_COMPLETE: return "UPNP_CONTROL_GET_VAR_COMPLETE";
	case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
		return "UPNP_DISCOVERY_ADVERTISEMENT_ALIVE";
	case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
		return "UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE";
	case UPNP_DISCOVERY_SEARCH_RESULT: return "UPNP_DISCOVERY_SEARCH_RESULT";
	case UPNP_DISCOVERY_SEARCH_TIMEOUT: return "UPNP_DISCOVERY_SEARCH_TIMEOUT";
	case UPNP_EVENT_SUBSCRIPTION_REQUEST:
		return "UPNP_EVENT_SUBSCRIPTION_REQUEST";
	case UPNP_EVENT_RECEIVED: return "UPNP_EVENT_RECEIVED";
	case UPNP_EVENT_RENEWAL_COMPLETE: return "UPNP_EVENT_RENEWAL_COMPLETE";
	case UPNP_EVENT_SUBSCRIBE_COMPLETE: return "UPNP_EVENT_SUBSCRIBE_COMPLETE";
	case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
		return "UPNP_EVENT_UNSUBSCRIBE_COMPLETE";
	case UPNP_EVENT_AUTORENEWAL_FAILED: return "UPNP_EVENT_AUTORENEWAL_FAILED";
	case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
		return "UPNP_EVENT_SUBSCRIPTION_EXPIRED";
	default: return "UPNP UNKNOWN EVENT";
	}
}

/////////////////////// Small global helpers
/** Get rid of white space at both ends */
void trimstring(string &s, const char *ws)
{
	string::size_type pos = s.find_first_not_of(ws);
	if (pos == string::npos) {
		s.clear();
		return;
	}
	s.replace(0, pos, string());

	pos = s.find_last_not_of(ws);
	if (pos != string::npos && pos != s.length()-1)
		s.replace(pos+1, string::npos, string());
}
string caturl(const string& s1, const string& s2)
{
	string out(s1);
	if (out[out.size()-1] == '/') {
		if (s2[0] == '/')
			out.erase(out.size()-1);
	} else {
		if (s2[0] != '/')
			out.push_back('/');
	}
	out += s2;
	return out;
}

string baseurl(const string& url)
{
    string::size_type pos = url.find("://");
    if (pos == string::npos)
        return url;

    pos = url.find_first_of("/", pos + 3);
    if (pos == string::npos) {
        return url;
    } else {
        return url.substr(0, pos + 1);
    }
}

static void path_catslash(string &s) {
	if (s.empty() || s[s.length() - 1] != '/')
		s += '/';
}
string path_getfather(const string &s)
{
	string father = s;

	// ??
	if (father.empty())
		return "./";

	if (father[father.length() - 1] == '/') {
		// Input ends with /. Strip it, handle special case for root
		if (father.length() == 1)
			return father;
		father.erase(father.length()-1);
	}

	string::size_type slp = father.rfind('/');
	if (slp == string::npos)
		return "./";

	father.erase(slp);
	path_catslash(father);
	return father;
}
string path_getsimple(const string &s) {
    string simple = s;

    if (simple.empty())
        return simple;

    string::size_type slp = simple.rfind('/');
    if (slp == string::npos)
        return simple;

    simple.erase(0, slp+1);
    return simple;
}

template <class T> bool csvToStrings(const string &s, T &tokens)
{
	string current;
	tokens.clear();
	enum states {TOKEN, ESCAPE};
	states state = TOKEN;
	for (unsigned int i = 0; i < s.length(); i++) {
		switch (s[i]) {
		case ',':
			switch(state) {
			case TOKEN:
				tokens.insert(tokens.end(), current);
				current.clear();
				continue;
			case ESCAPE:
				current += ',';
				state = TOKEN;
				continue;
			}
			break;
		case '\\':
			switch(state) {
			case TOKEN:
				state=ESCAPE;
				continue;
			case ESCAPE:
				current += '\\';
				state = TOKEN;
				continue;
			}
			break;

		default:
			switch(state) {
			case ESCAPE:
				state = TOKEN;
				break;
			case TOKEN:
				break;
			}
			current += s[i];
		}
	}
	switch(state) {
	case TOKEN:
		tokens.insert(tokens.end(), current);
		break;
	case ESCAPE:
		return false;
	}
	return true;
}

//template bool csvToStrings<list<string> >(const string &, list<string> &);
template bool csvToStrings<vector<string> >(const string &, vector<string> &);
template bool csvToStrings<set<string> >(const string &, set<string> &);
/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
