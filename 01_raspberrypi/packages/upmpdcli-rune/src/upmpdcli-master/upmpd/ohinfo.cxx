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
#include "ohinfo.hxx"
#include "mpdcli.hxx"
#include "upmpdutils.hxx"

static const string sTpProduct("urn:av-openhome-org:service:Info:1");
static const string sIdProduct("urn:av-openhome-org:serviceId:Info");

OHInfo::OHInfo(UpMpd *dev)
    : UpnpService(sTpProduct, sIdProduct, dev), m_dev(dev)
{
    dev->addActionMapping(this, "Counters", 
                          bind(&OHInfo::counters, this, _1, _2));
    dev->addActionMapping(this, "Track", 
                          bind(&OHInfo::track, this, _1, _2));
    dev->addActionMapping(this, "Details", 
                          bind(&OHInfo::details, this, _1, _2));
    dev->addActionMapping(this, "Metatext", 
                          bind(&OHInfo::metatext, this, _1, _2));
}

void OHInfo::urimetadata(string& uri, string& metadata)
{
    const MpdStatus &mpds =  m_dev->getMpdStatusNoUpdate();
    bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
        (mpds.state == MpdStatus::MPDS_PAUSE);

    if (is_song) {
        uri = mpds.currentsong.uri;
        metadata = didlmake(mpds.currentsong);
    } else {
        uri.clear();
        metadata.clear();
    }
}

void OHInfo::makedetails(string &duration, string& bitrate, 
                         string& bitdepth, string& samplerate)
{
    const MpdStatus &mpds =  m_dev->getMpdStatusNoUpdate();

    bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
        (mpds.state == MpdStatus::MPDS_PAUSE);

    if (is_song) {
        duration = SoapArgs::i2s(mpds.songlenms / 1000);
        bitrate = SoapArgs::i2s(mpds.kbrate * 1000);
        bitdepth = SoapArgs::i2s(mpds.bitdepth);
        samplerate = SoapArgs::i2s(mpds.sample_rate);
    } else {
        duration = bitrate = bitdepth = samplerate = "0";
    }
}

bool OHInfo::makestate(unordered_map<string, string> &st)
{
    st.clear();

    st["TrackCount"] = SoapArgs::i2s(m_dev->m_mpds ? 
                                     m_dev->m_mpds->trackcounter : 0);
    st["DetailsCount"] = SoapArgs::i2s(m_dev->m_mpds ? 
                                       m_dev->m_mpds->detailscounter : 0);
    st["MetatextCount"] = "0";
    string uri, metadata;
    urimetadata(uri, metadata);
    st["Uri"] = uri;
    st["Metadata"] = metadata;
    makedetails(st["Duration"], st["BitRate"], st["BitDepth"],st["SampleRate"]);
    st["Lossless"] = "0";
    st["CodecName"] = "";
    st["Metatext"] = "";
    return true;
}

bool OHInfo::getEventData(bool all, std::vector<std::string>& names, 
                             std::vector<std::string>& values)
{
    //LOGDEB("OHInfo::getEventData" << endl);

    unordered_map<string, string> state, changed;
    makestate(state);
    if (all) {
        changed = state;
    } else {
        changed = diffmaps(m_state, state);
    }
    m_state = state;

    for (auto& member : changed) {
        names.push_back(member.first);
        values.push_back(member.second);
    }

    return true;
}

int OHInfo::counters(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHInfo::counters" << endl);
    
    data.addarg("TrackCount", SoapArgs::i2s(m_dev->m_mpds ?
                                            m_dev->m_mpds->trackcounter : 0));
    data.addarg("DetailsCount", SoapArgs::i2s(m_dev->m_mpds ?
                                              m_dev->m_mpds->detailscounter:0));
    data.addarg("MetatextCount", "0");
    return UPNP_E_SUCCESS;
}

int OHInfo::track(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHInfo::track" << endl);

    string uri, metadata;
    urimetadata(uri, metadata);
    data.addarg("Uri", uri);
    data.addarg("Metadata", metadata);
    return UPNP_E_SUCCESS;
}

int OHInfo::details(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHInfo::details" << endl);

    string duration, bitrate, bitdepth, samplerate;
    makedetails(duration, bitrate, bitdepth, samplerate);
    data.addarg("Duration", duration);
    data.addarg("BitRate", bitrate);
    data.addarg("BitDepth", bitdepth);
    data.addarg("SampleRate", samplerate);
    data.addarg("Lossless", "0");
    data.addarg("CodecName", "");
    return UPNP_E_SUCCESS;
}

int OHInfo::metatext(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHInfo::metatext" << endl);
    data.addarg("Value", "");
    return UPNP_E_SUCCESS;
}
