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
// An XML parser which constructs an UPnP device object from the
// device descriptor
#include "config.h"

#include <stdlib.h>
#include <errno.h>

#include <iostream>
#include <map>
using namespace std;

#include "upnpp_p.hxx"
#include "expatmm.hxx"
#include "description.hxx"

class UPnPDeviceParser : public expatmm::inputRefXMLParser {
public:
	UPnPDeviceParser(const string& input, UPnPDevice& device)
		: expatmm::inputRefXMLParser(input), m_device(device)
		{}

protected:
	virtual void StartElement(const XML_Char *name, const XML_Char **)
	{
		m_tabs.push_back('\t');
		m_path.push_back(name);
	}
	virtual void EndElement(const XML_Char *name)
	{
		if (!strcmp(name, "service")) {
			m_device.services.push_back(m_tservice);
			m_tservice.clear();
		}
		if (m_tabs.size())
			m_tabs.erase(m_tabs.size()-1);
		m_path.pop_back();
	}
	virtual void CharacterData(const XML_Char *s, int len)
	{
		if (s == 0 || *s == 0)
			return;
		string str(s, len);
		trimstring(str);
		switch (m_path.back()[0]) {
		case 'c':
			if (!m_path.back().compare("controlURL"))
				m_tservice.controlURL += str;
			break;
		case 'd':
			if (!m_path.back().compare("deviceType"))
				m_device.deviceType += str;
			break;
		case 'e':
			if (!m_path.back().compare("eventSubURL"))
				m_tservice.eventSubURL += str;
			break;
		case 'f':
			if (!m_path.back().compare("friendlyName"))
				m_device.friendlyName += str;
			break;
		case 'm':
			if (!m_path.back().compare("manufacturer"))
				m_device.manufacturer += str;
			else if (!m_path.back().compare("modelName"))
				m_device.modelName += str;
			break;
		case 's':
			if (!m_path.back().compare("serviceType"))
				m_tservice.serviceType = str;
			else if (!m_path.back().compare("serviceId"))
				m_tservice.serviceId += str;
		case 'S':
			if (!m_path.back().compare("SCPDURL"))
				m_tservice.SCPDURL = str;
			break;
		case 'U':
			if (!m_path.back().compare("UDN"))
				m_device.UDN = str;
			else if (!m_path.back().compare("URLBase"))
				m_device.URLBase += str;
			break;
		}
	}

private:
	UPnPDevice& m_device;
	string m_tabs;
	std::vector<std::string> m_path;
	UPnPService m_tservice;
};

UPnPDevice::UPnPDevice(const string& url, const string& description)
	: ok(false)
{
	//cerr << "UPnPDevice::UPnPDevice: url: " << url << endl;
	//cerr << " description " << endl << description << endl;

	UPnPDeviceParser mparser(description, *this);
	if (!mparser.Parse())
		return;
	if (URLBase.empty()) {
		// The standard says that if the URLBase value is empty, we
		// should use the url the description was retrieved
		// from. However this is sometimes something like
		// http://host/desc.xml, sometimes something like http://host/
		// (rare, but e.g. sent by the server on a dlink nas).
		URLBase = baseurl(url);
	}
	ok = true;
	//cerr << "URLBase: [" << URLBase << "]" << endl;
	//cerr << dump() << endl;
}

/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
