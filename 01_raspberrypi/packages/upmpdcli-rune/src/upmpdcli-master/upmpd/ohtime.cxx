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
#include "ohtime.hxx"
#include "mpdcli.hxx"
#include "upmpdutils.hxx"

static const string sTpProduct("urn:av-openhome-org:service:Time:1");
static const string sIdProduct("urn:av-openhome-org:serviceId:Time");

OHTime::OHTime(UpMpd *dev)
    : UpnpService(sTpProduct, sIdProduct, dev), m_dev(dev)
{
    dev->addActionMapping(this, "Time", bind(&OHTime::ohtime, this, _1, _2));
}

void OHTime::getdata(string& trackcount, string &duration, 
                     string& seconds)
{
    // We're relying on AVTransport to have updated the status for us
    const MpdStatus& mpds =  m_dev->getMpdStatusNoUpdate();

    trackcount = SoapArgs::i2s(mpds.trackcounter);

    bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
        (mpds.state == MpdStatus::MPDS_PAUSE);
    if (is_song) {
        duration = SoapArgs::i2s(mpds.songlenms / 1000);
        seconds = SoapArgs::i2s(mpds.songelapsedms / 1000);
    } else {
        duration = "0";
        seconds = "0";
    }
}

bool OHTime::makestate(unordered_map<string, string> &st)
{
    st.clear();
    getdata(st["TrackCount"], st["Duration"], st["Seconds"]);
    return true;
}

bool OHTime::getEventData(bool all, std::vector<std::string>& names, 
                          std::vector<std::string>& values)
{
    //LOGDEB("OHTime::getEventData" << endl);

    unordered_map<string, string> state;
    makestate(state);

    unordered_map<string, string> changed;
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

int OHTime::ohtime(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHTime::ohtime" << endl);
    string trackcount, duration, seconds;
    getdata(trackcount, duration, seconds);
    data.addarg("TrackCount", trackcount);
    data.addarg("Duration", duration);
    data.addarg("Seconds", seconds);
    return UPNP_E_SUCCESS;
}
