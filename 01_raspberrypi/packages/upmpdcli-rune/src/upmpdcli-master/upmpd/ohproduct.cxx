/* Copyright (C) 2014 J.F.Dockes
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>
#include <set>
using namespace std;
using namespace std::placeholders;

#include "libupnpp/upnpplib.hxx"
#include "libupnpp/soaphelp.hxx"
#include "libupnpp/device.hxx"
#include "libupnpp/log.hxx"

#include "upmpd.hxx"
#include "ohproduct.hxx"
#include "mpdcli.hxx"
#include "upmpdutils.hxx"

static const string sTpProduct("urn:av-openhome-org:service:Product:1");
static const string sIdProduct("urn:av-openhome-org:serviceId:Product");

OHProduct::OHProduct(UpMpd *dev, const string& friendlyname)
    : UpnpService(sTpProduct, sIdProduct, dev), m_dev(dev),
      m_roomOrName(friendlyname)
{
    dev->addActionMapping(this, "Manufacturer", 
                          bind(&OHProduct::manufacturer, this, _1, _2));
    dev->addActionMapping(this, "Model", bind(&OHProduct::model, this, _1, _2));
    dev->addActionMapping(this, "Product", 
                          bind(&OHProduct::product, this, _1, _2));
    dev->addActionMapping(this, "Standby", 
                          bind(&OHProduct::standby, this, _1, _2));
    dev->addActionMapping(this, "SetStandby", 
                          bind(&OHProduct::setStandby, this, _1, _2));
    dev->addActionMapping(this, "SourceCount", 
                          bind(&OHProduct::sourceCount, this, _1, _2));
    dev->addActionMapping(this, "SourceXml", 
                          bind(&OHProduct::sourceXML, this, _1, _2));
    dev->addActionMapping(this, "SourceIndex", 
                          bind(&OHProduct::sourceIndex, this, _1, _2));
    dev->addActionMapping(this, "SetSourceIndex", 
                          bind(&OHProduct::setSourceIndex, this, _1, _2));
    dev->addActionMapping(this, "SetSourceIndexByName", 
                          bind(&OHProduct::setSourceIndexByName, this, _1, _2));
    dev->addActionMapping(this, "Source", 
                          bind(&OHProduct::source, this, _1, _2));
    dev->addActionMapping(this, "Attributes", 
                          bind(&OHProduct::attributes, this, _1, _2));
    dev->addActionMapping(this, "SourceXmlChangeCount", 
                          bind(&OHProduct::sourceXMLChangeCount, this, _1, _2));
}

static const string csxml(
    "<SourceList>"
    "<Source>"
    "<Name>PlayList</Name>"
    "<Type>Playlist</Type>"
    "<Visible>true</Visible>"
    "</Source>"
    "</SourceList>"
    );

static const string csattrs("Info Time Volume");
static const string csversion("1.0");
static const string csmanname("UpMPDCli heavy industries Co.");
static const string csmaninfo("Such nice guys and gals");
static const string csmanurl("http://www.lesbonscomptes.com/upmpdcli");
static const string csmodname("UpMPDCli UPnP-MPD gateway");
static const string csmodurl("http://www.lesbonscomptes.com/upmpdcli");
static const string csprodname("Upmpdcli");
bool OHProduct::getEventData(bool all, std::vector<std::string>& names, 
                             std::vector<std::string>& values)
{
    //LOGDEB("OHProduct::getEventData" << endl);
    // Our data never changes, so if this is not an unconditional
    // request, we return nothing.
    if (all) {
        names.push_back("ManufacturerName"); values.push_back(csmanname);
        names.push_back("ManufacturerInfo"); values.push_back(csmaninfo);
        names.push_back("ManufacturerUrl");  values.push_back(csmanurl);
        names.push_back("ManufacturerImageUri"); values.push_back("");
        names.push_back("ModelName"); values.push_back(csmodname);
        names.push_back("ModelInfo"); values.push_back(csversion);
        names.push_back("ModelUrl");  values.push_back(csmodurl);
        names.push_back("ModelImageUri"); values.push_back("");
        names.push_back("ProductRoom"); values.push_back(m_roomOrName);
        names.push_back("ProductName"); values.push_back(csprodname);
        names.push_back("ProductInfo"); values.push_back(csversion);
        names.push_back("ProductUrl"); values.push_back("");
        names.push_back("ProductImageUri"); values.push_back("");
        names.push_back("Standby"); values.push_back("0");
        names.push_back("SourceCount"); values.push_back("1");
        names.push_back("SourceXml"); values.push_back(csxml);
        names.push_back("SourceIndex"); values.push_back("0");
        names.push_back("Attributes");values.push_back(csattrs);
    }
    return true;
}

int OHProduct::manufacturer(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::manufacturer" << endl);
    data.addarg("Name", csmanname);
    data.addarg("Info", csmaninfo);
    data.addarg("Url", csmanurl);
    data.addarg("ImageUri", "");
    return UPNP_E_SUCCESS;
}

int OHProduct::model(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::model" << endl);
    data.addarg("Name", csmodname);
    data.addarg("Info", csversion);
    data.addarg("Url", csmodurl);
    data.addarg("ImageUri", "");
    return UPNP_E_SUCCESS;
}

int OHProduct::product(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::product" << endl);
    data.addarg("Room", m_roomOrName);
    data.addarg("Name", csprodname);
    data.addarg("Info", csversion);
    data.addarg("Url", "");
    data.addarg("ImageUri", "");
    return UPNP_E_SUCCESS;
}

int OHProduct::standby(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::standby" << endl);
    data.addarg("Value", "0");
    return UPNP_E_SUCCESS;
}

int OHProduct::setStandby(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::setStandby" << endl);
    bool standby;
    if (!sc.getBool("Value", &standby)) {
        return UPNP_E_INVALID_PARAM;
    }
    return UPNP_E_SUCCESS;
}

int OHProduct::sourceCount(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::sourceCount" << endl);
    data.addarg("Value", "1");
    return UPNP_E_SUCCESS;
}

int OHProduct::sourceXML(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::sourceXML" << endl);
    data.addarg("Value", csxml);
    return UPNP_E_SUCCESS;
}

int OHProduct::sourceIndex(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::sourceIndex" << endl);
    data.addarg("Value", "0");
    return UPNP_E_SUCCESS;
}

int OHProduct::setSourceIndex(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::setSourceIndex" << endl);
    int sindex;
    if (!sc.getInt("Value", &sindex)) {
        return UPNP_E_INVALID_PARAM;
    }
    LOGDEB("OHProduct::setSourceIndex: " << sindex << endl);
    return UPNP_E_SUCCESS;
}

int OHProduct::setSourceIndexByName(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::setSourceIndexByName" << endl);
    map<string, string>::const_iterator it;

    it = sc.args.find("Value");
    if (it == sc.args.end() || it->second.empty()) {
        return UPNP_E_INVALID_PARAM;
    }
    LOGDEB("OHProduct::setSourceIndexByName: " << it->second << endl);
    m_dev->loopWakeup();
    return UPNP_E_SUCCESS;
}

int OHProduct::source(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::source" << endl);
    int sindex;
    if (!sc.getInt("Index", &sindex)) {
        return UPNP_E_INVALID_PARAM;
    }
    LOGDEB("OHProduct::source: " << sindex << endl);
    if (sindex != 0) {
        return UPNP_E_INVALID_PARAM;
    }
    data.addarg("SystemName", m_roomOrName);
    data.addarg("Type", "Playlist");
    data.addarg("Name", "PlayList");
    data.addarg("Visible", "1");
    return UPNP_E_SUCCESS;
}

int OHProduct::attributes(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::attributes" << endl);
    data.addarg("Value", csattrs);
    return UPNP_E_SUCCESS;
}

int OHProduct::sourceXMLChangeCount(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHProduct::sourceXMLChangeCount" << endl);
    data.addarg("Value", "0");
    return UPNP_E_SUCCESS;
}

