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
#include "renderctl.hxx"
#include "mpdcli.hxx"
#include "upmpdutils.hxx"

static const string 
sTpRender("urn:schemas-upnp-org:service:RenderingControl:1");
static const string sIdRender("urn:upnp-org:serviceId:RenderingControl");

static const int minVolumeDelta = 5;

UpMpdRenderCtl::UpMpdRenderCtl(UpMpd *dev)
    : UpnpService(sTpRender, sIdRender, dev), m_dev(dev), m_desiredvolume(-1)
{
    m_dev->addActionMapping(this, "SetMute", 
                            bind(&UpMpdRenderCtl::setMute, this, _1, _2));
    m_dev->addActionMapping(this, "GetMute", 
                            bind(&UpMpdRenderCtl::getMute, this, _1, _2));
    m_dev->addActionMapping(this, "SetVolume", bind(&UpMpdRenderCtl::setVolume, 
                                              this, _1, _2, false));
    m_dev->addActionMapping(this, "GetVolume", bind(&UpMpdRenderCtl::getVolume, 
                                              this, _1, _2, false));
    m_dev->addActionMapping(this, "ListPresets", 
                            bind(&UpMpdRenderCtl::listPresets, this, _1, _2));
    m_dev->addActionMapping(this, "SelectPreset", 
                            bind(&UpMpdRenderCtl::selectPreset, this, _1, _2));
}

////////////////////////////////////////////////////
/// RenderingControl methods

// State variables for the RenderingControl. All evented through LastChange
//  PresetNameList
//  Mute
//  Volume
//  VolumeDB
// LastChange contains all the variables that were changed since the last
// event. For us that's at most Mute, Volume, VolumeDB
// <Event xmlns=”urn:schemas-upnp-org:metadata-1-0/AVT_RCS">
//   <InstanceID val=”0”>
//     <Mute channel=”Master” val=”0”/>
//     <Volume channel=”Master” val=”24”/>
//     <VolumeDB channel=”Master” val=”24”/>
//   </InstanceID>
// </Event>

bool UpMpdRenderCtl::rdstateMToU(unordered_map<string, string>& status)
{
    const MpdStatus &mpds = m_dev->getMpdStatus();

    int volume = m_desiredvolume >= 0 ? m_desiredvolume : mpds.volume;
    if (volume < 0)
        volume = 0;
    status["Volume"] = SoapArgs::i2s(volume);
    //status["VolumeDB"] =  SoapArgs::i2s(percentodbvalue(volume));
    status["Mute"] =  volume == 0 ? "1" : "0";
    return true;
}

bool UpMpdRenderCtl::getEventData(bool all, std::vector<std::string>& names, 
                                  std::vector<std::string>& values)
{
    //LOGDEB("UpMpdRenderCtl::getEventDataRendering. desiredvolume " << 
    //		   m_desiredvolume << (all?" all " : "") << endl);
    if (m_desiredvolume >= 0) {
        m_dev->m_mpdcli->setVolume(m_desiredvolume);
        m_desiredvolume = -1;
    }

    unordered_map<string, string> newstate;
    rdstateMToU(newstate);
    if (all)
        m_rdstate.clear();

    string 
        chgdata("<Event xmlns=\"urn:schemas-upnp-org:metadata-1-0/AVT_RCS\">\n"
                "<InstanceID val=\"0\">\n");

    bool changefound = false;
    for (unordered_map<string, string>::const_iterator it = newstate.begin();
         it != newstate.end(); it++) {

        const string& oldvalue = mapget(m_rdstate, it->first);
        if (!it->second.compare(oldvalue))
            continue;

        changefound = true;

        chgdata += "<";
        chgdata += it->first;
        chgdata += " val=\"";
        chgdata += SoapArgs::xmlQuote(it->second);
        chgdata += "\"/>\n";
    }
    chgdata += "</InstanceID>\n</Event>\n";

    if (!changefound) {
        return true;
    }

    names.push_back("LastChange");
    values.push_back(chgdata);

    m_rdstate = newstate;

    return true;
}

// Actions:
// Note: we need to return all out arguments defined by the SOAP call even if
// they don't make sense (because there is no song playing). Ref upnp arch p.51:
//
//   argumentName: Required if and only if action has out
//   arguments. Value returned from action. Repeat once for each out
//   argument. If action has an argument marked as retval, this
//   argument must be the first element. (Element name not qualified
//   by a namespace; element nesting context is sufficient.) Case
//   sensitive. Single data type as defined by UPnP service
//   description. Every “out” argument in the definition of the action
//   in the service description must be included, in the same order as
//   specified in the service description (SCPD) available from the
//   device.

#if 0
int UpMpdRenderCtl::getVolumeDBRange(const SoapArgs& sc, SoapData& data)
{
    map<string, string>::const_iterator it;

    it = sc.args.find("Channel");
    if (it == sc.args.end() || it->second.compare("Master")) {
        return UPNP_E_INVALID_PARAM;
    }
    data.addarg("MinValue", "-10240");
    data.addarg("MaxValue", "0");

    return UPNP_E_SUCCESS;
}
#endif

int UpMpdRenderCtl::getvolume_i()
{
    return m_desiredvolume >= 0 ? m_desiredvolume : 
        m_dev->m_mpdcli->getVolume();
}

void UpMpdRenderCtl::setvolume_i(int volume)
{
    int previous_volume = m_dev->m_mpdcli->getVolume();
    int delta = previous_volume - volume;
    if (delta < 0)
        delta = -delta;
    LOGDEB("UpMpdRenderCtl::setVolume: volume " << volume << " delta " << 
           delta << endl);
    if (delta >= minVolumeDelta) {
        m_dev->m_mpdcli->setVolume(volume);
        m_desiredvolume = -1;
    } else {
        m_desiredvolume = volume;
    }
}

void UpMpdRenderCtl::setmute_i(bool onoff)
{
    if (onoff) {
        if (m_desiredvolume >= 0) {
            m_dev->m_mpdcli->setVolume(m_desiredvolume);
            m_desiredvolume = -1;
        }
        m_dev->m_mpdcli->setVolume(0, true);
    } else {
        // Restore pre-mute
        m_dev->m_mpdcli->setVolume(1, true);
    }
}

int UpMpdRenderCtl::setMute(const SoapArgs& sc, SoapData& data)
{
    map<string, string>::const_iterator it;

    it = sc.args.find("Channel");
    if (it == sc.args.end() || it->second.compare("Master")) {
        return UPNP_E_INVALID_PARAM;
    }
		
    it = sc.args.find("DesiredMute");
    if (it == sc.args.end() || it->second.empty()) {
        return UPNP_E_INVALID_PARAM;
    }
    if (it->second[0] == 'F' || it->second[0] == '0') {
        setmute_i(false);
    } else if (it->second[0] == 'T' || it->second[0] == '1') {
        setmute_i(true);
    } else {
        return UPNP_E_INVALID_PARAM;
    }
    m_dev->loopWakeup();
    return UPNP_E_SUCCESS;
}

int UpMpdRenderCtl::getMute(const SoapArgs& sc, SoapData& data)
{
    map<string, string>::const_iterator it;

    it = sc.args.find("Channel");
    if (it == sc.args.end() || it->second.compare("Master")) {
        return UPNP_E_INVALID_PARAM;
    }
    int volume = m_dev->m_mpdcli->getVolume();
    data.addarg("CurrentMute", volume == 0 ? "1" : "0");
    return UPNP_E_SUCCESS;
}

int UpMpdRenderCtl::setVolume(const SoapArgs& sc, SoapData& data, bool isDb)
{
    map<string, string>::const_iterator it;

    it = sc.args.find("Channel");
    if (it == sc.args.end() || it->second.compare("Master")) {
        return UPNP_E_INVALID_PARAM;
    }
		
    it = sc.args.find("DesiredVolume");
    if (it == sc.args.end() || it->second.empty()) {
        return UPNP_E_INVALID_PARAM;
    }
    int volume = atoi(it->second.c_str());
    if (isDb) {
        volume = dbvaluetopercent(volume);
    } 
    if (volume < 0 || volume > 100) {
        return UPNP_E_INVALID_PARAM;
    }
	
    setvolume_i(volume);

    m_dev->loopWakeup();
    return UPNP_E_SUCCESS;
}

int UpMpdRenderCtl::getVolume(const SoapArgs& sc, SoapData& data, bool isDb)
{
    // LOGDEB("UpMpdRenderCtl::getVolume" << endl);
    map<string, string>::const_iterator it;

    it = sc.args.find("Channel");
    if (it == sc.args.end() || it->second.compare("Master")) {
        return UPNP_E_INVALID_PARAM;
    }
		
    int volume = getvolume_i();
    if (isDb) {
        volume = percentodbvalue(volume);
    }
    data.addarg("CurrentVolume", SoapArgs::i2s(volume));
    return UPNP_E_SUCCESS;
}

int UpMpdRenderCtl::listPresets(const SoapArgs& sc, SoapData& data)
{
    // The 2nd arg is a comma-separated list of preset names
    data.addarg("CurrentPresetNameList", "FactoryDefaults");
    return UPNP_E_SUCCESS;
}

int UpMpdRenderCtl::selectPreset(const SoapArgs& sc, SoapData& data)
{
    map<string, string>::const_iterator it;
		
    it = sc.args.find("PresetName");
    if (it == sc.args.end() || it->second.empty()) {
        return UPNP_E_INVALID_PARAM;
    }
    if (it->second.compare("FactoryDefaults")) {
        return UPNP_E_INVALID_PARAM;
    }

    // Well there is only the volume actually...
    int volume = 50;
    m_dev->m_mpdcli->setVolume(volume);

    return UPNP_E_SUCCESS;
}
