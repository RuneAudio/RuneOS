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
#include "ohplaylist.hxx"
#include "mpdcli.hxx"
#include "upmpdutils.hxx"
//#include "renderctl.hxx"
#include "base64.hxx"

static const string sTpProduct("urn:av-openhome-org:service:Playlist:1");
static const string sIdProduct("urn:av-openhome-org:serviceId:Playlist");

OHPlaylist::OHPlaylist(UpMpd *dev, UpMpdRenderCtl *ctl)
    : UpnpService(sTpProduct, sIdProduct, dev), m_dev(dev), m_ctl(ctl)
{
    dev->addActionMapping(this, "Play", 
                          bind(&OHPlaylist::play, this, _1, _2));
    dev->addActionMapping(this, "Pause", 
                          bind(&OHPlaylist::pause, this, _1, _2));
    dev->addActionMapping(this, "Stop", 
                          bind(&OHPlaylist::stop, this, _1, _2));
    dev->addActionMapping(this, "Next", 
                          bind(&OHPlaylist::next, this, _1, _2));
    dev->addActionMapping(this, "Previous", 
                          bind(&OHPlaylist::previous, this, _1, _2));
    dev->addActionMapping(this, "SetRepeat",
                          bind(&OHPlaylist::setRepeat, this, _1, _2));
    dev->addActionMapping(this, "Repeat",
                          bind(&OHPlaylist::repeat, this, _1, _2));
    dev->addActionMapping(this, "SetShuffle",
                          bind(&OHPlaylist::setShuffle, this, _1, _2));
    dev->addActionMapping(this, "Shuffle",
                          bind(&OHPlaylist::shuffle, this, _1, _2));
    dev->addActionMapping(this, "SeekSecondAbsolute",
                          bind(&OHPlaylist::seekSecondAbsolute, this, _1, _2));
    dev->addActionMapping(this, "SeekSecondRelative",
                          bind(&OHPlaylist::seekSecondRelative, this, _1, _2));
    dev->addActionMapping(this, "SeekId",
                          bind(&OHPlaylist::seekId, this, _1, _2));
    dev->addActionMapping(this, "SeekIndex",
                          bind(&OHPlaylist::seekIndex, this, _1, _2));
    dev->addActionMapping(this, "TransportState",
                          bind(&OHPlaylist::transportState, this, _1, _2));
    dev->addActionMapping(this, "Id",
                          bind(&OHPlaylist::id, this, _1, _2));
    dev->addActionMapping(this, "Read",
                          bind(&OHPlaylist::ohread, this, _1, _2));
    dev->addActionMapping(this, "ReadList",
                          bind(&OHPlaylist::readList, this, _1, _2));
    dev->addActionMapping(this, "Insert",
                          bind(&OHPlaylist::insert, this, _1, _2));
    dev->addActionMapping(this, "DeleteId",
                          bind(&OHPlaylist::deleteId, this, _1, _2));
    dev->addActionMapping(this, "DeleteAll",
                          bind(&OHPlaylist::deleteAll, this, _1, _2));
    dev->addActionMapping(this, "TracksMax",
                          bind(&OHPlaylist::tracksMax, this, _1, _2));
    dev->addActionMapping(this, "IdArray",
                          bind(&OHPlaylist::idArray, this, _1, _2));
    dev->addActionMapping(this, "IdArrayChanged",
                          bind(&OHPlaylist::idArrayChanged, this, _1, _2));
    dev->addActionMapping(this, "ProtocolInfo",
                          bind(&OHPlaylist::protocolInfo, this, _1, _2));
    dev->m_mpdcli->consume(false);
}

static const int tracksmax = 16384;

static string mpdstatusToTransportState(MpdStatus::State st)
{
    string tstate;
    switch(st) {
    case MpdStatus::MPDS_PLAY: 
        tstate = "Playing";
        break;
    case MpdStatus::MPDS_PAUSE: 
        tstate = "Paused";
        break;
    default:
        tstate = "Stopped";
    }
    return tstate;
}

// The data format for id lists is an array of msb 32 bits ints
// encoded in base64...
static string translateIdArray(const vector<UpSong>& in)
{
    string out1;
    string sdeb;
    for (auto us : in) {
        unsigned int val = us.mpdid;
        if (val) {
            out1 += (unsigned char) ((val & 0xff000000) >> 24);
            out1 += (unsigned char) ((val & 0x00ff0000) >> 16);
            out1 += (unsigned char) ((val & 0x0000ff00) >> 8);
            out1 += (unsigned char) ((val & 0x000000ff));
        }
        //sdeb += SoapArgs::i2s(val) + " ";
    }
    //LOGDEB("OHPlaylist: current ids: " << sdeb << endl);
    return base64_encode(out1);
}
string OHPlaylist::makeIdArray()
{
    string out;

    // Retrieve the data for current queue songs from mpd, and make an
    // ohPlaylist id array.
    vector<UpSong> vdata;
    bool ok = m_dev->m_mpdcli->getQueueData(vdata);
    if (ok) {
        out = translateIdArray(vdata);
    }

    // Update metadata cache: entries not in the curren id list are
    // not valid any more. Also there may be entries which were added
    // through an MPD client and which don't know about, record the
    // metadata for these. We don't update the current array, but
    // just build a new cache for data about current entries
    unordered_map<int, string> nmeta;

    // Walk the playlist data from MPD
    for (auto& usong : vdata) {
        auto inold = m_metacache.find(usong.mpdid);
        if (inold != m_metacache.end()) {
            // Entries already in the metadata array just get
            // transferred to the new array
            nmeta[usong.mpdid].swap(inold->second);
        } else {
            // Entries not in the old array are translated from the
            // MPD data to our format. They were probably added by
            // another MPD client. 
            nmeta[usong.mpdid] = didlmake(usong);
        }
    }
    m_metacache = nmeta;

    return out;
}

bool OHPlaylist::makestate(unordered_map<string, string> &st)
{
    st.clear();

    const MpdStatus &mpds = m_dev->getMpdStatusNoUpdate();

    st["TransportState"] =  mpdstatusToTransportState(mpds.state);
    st["Repeat"] = SoapArgs::i2s(mpds.rept);
    st["Shuffle"] = SoapArgs::i2s(mpds.random);
    st["Id"] = SoapArgs::i2s(mpds.songid);
    st["TracksMax"] = SoapArgs::i2s(tracksmax);
    st["ProtocolInfo"] = upmpdProtocolInfo;
    st["IdArray"] = makeIdArray();

    return true;
}

bool OHPlaylist::getEventData(bool all, std::vector<std::string>& names, 
                              std::vector<std::string>& values)
{
    //LOGDEB("OHPlaylist::getEventData" << endl);

    unordered_map<string, string> state;

    makestate(state);

    unordered_map<string, string> changed;
    if (all) {
        changed = state;
    } else {
        changed = diffmaps(m_state, state);
    }
    m_state = state;

    for (auto& entry : changed) {
        names.push_back(entry.first);
        values.push_back(entry.second);
    }

    return true;
}

void OHPlaylist::maybeWakeUp(bool ok)
{
    if (ok && m_dev)
        m_dev->loopWakeup();
}

int OHPlaylist::play(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::play" << endl);
    bool ok = m_dev->m_mpdcli->play();
    maybeWakeUp(ok);
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::pause(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::pause" << endl);
    bool ok = m_dev->m_mpdcli->pause(true);
    maybeWakeUp(ok);
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::stop(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::stop" << endl);
    bool ok = m_dev->m_mpdcli->stop();
    maybeWakeUp(ok);
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::next(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::next" << endl);
    bool ok = m_dev->m_mpdcli->next();
    maybeWakeUp(ok);
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::previous(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::previous" << endl);
    bool ok = m_dev->m_mpdcli->previous();
    maybeWakeUp(ok);
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::setRepeat(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::setRepeat" << endl);
    bool onoff;
    bool ok = sc.getBool("Value", &onoff);
    if (ok) {
        ok = m_dev->m_mpdcli->repeat(onoff);
        maybeWakeUp(ok);
    }
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::repeat(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::repeat" << endl);
    const MpdStatus &mpds =  m_dev->getMpdStatus();
    data.addarg("Value", mpds.rept? "1" : "0");
    return UPNP_E_SUCCESS;
}

int OHPlaylist::setShuffle(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::setShuffle" << endl);
    bool onoff;
    bool ok = sc.getBool("Value", &onoff);
    if (ok) {
        // Note that mpd shuffle shuffles the playlist, which is different
        // from playing at random
        ok = m_dev->m_mpdcli->random(onoff);
        maybeWakeUp(ok);
    }
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::shuffle(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::shuffle" << endl);
    const MpdStatus &mpds =  m_dev->getMpdStatus();
    data.addarg("Value", mpds.random ? "1" : "0");
    return UPNP_E_SUCCESS;
}

int OHPlaylist::seekSecondAbsolute(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::seekSecondAbsolute" << endl);
    int seconds;
    bool ok = sc.getInt("Value", &seconds);
    if (ok) {
        ok = m_dev->m_mpdcli->seek(seconds);
        maybeWakeUp(ok);
    }
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::seekSecondRelative(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::seekSecondRelative" << endl);
    int seconds;
    bool ok = sc.getInt("Value", &seconds);
    if (ok) {
        const MpdStatus &mpds =  m_dev->getMpdStatusNoUpdate();
        bool is_song = (mpds.state == MpdStatus::MPDS_PLAY) || 
            (mpds.state == MpdStatus::MPDS_PAUSE);
        if (is_song) {
            seconds += mpds.songelapsedms / 1000;
            ok = m_dev->m_mpdcli->seek(seconds);
        } else {
            ok = false;
        }
        maybeWakeUp(ok);
    }
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::transportState(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::transportState" << endl);
    const MpdStatus &mpds = m_dev->getMpdStatusNoUpdate();
    string tstate;
    switch(mpds.state) {
    case MpdStatus::MPDS_PLAY: 
        tstate = "Playing";
        break;
    case MpdStatus::MPDS_PAUSE: 
        tstate = "Paused";
        break;
    default:
        tstate = "Stopped";
    }
    data.addarg("TransportState", tstate);
    return UPNP_E_SUCCESS;
}

// Skip to track specified by Id
int OHPlaylist::seekId(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::seekId" << endl);
    int id;
    bool ok = sc.getInt("Value", &id);
    if (ok) {
        ok = m_dev->m_mpdcli->playId(id);
        maybeWakeUp(ok);
    }
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

// Skip to track with specified index 
int OHPlaylist::seekIndex(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::seekIndex" << endl);
    int pos;
    bool ok = sc.getInt("Value", &pos);
    if (ok) {
        ok = m_dev->m_mpdcli->play(pos);
        maybeWakeUp(ok);
    }
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

// Return current Id
int OHPlaylist::id(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::id" << endl);
    const MpdStatus &mpds = m_dev->getMpdStatusNoUpdate();
    data.addarg("Value", SoapArgs::i2s(mpds.songid));
    return UPNP_E_SUCCESS;
}

// Report the uri and metadata for a given track id. 
// Returns a 800 fault code if the given id is not in the playlist. 
int OHPlaylist::ohread(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::ohread" << endl);
    int id;
    bool ok = sc.getInt("Id", &id);
    UpSong song;
    if (ok) {
        ok = m_dev->m_mpdcli->statSong(song, id, true);
    }
    if (ok) {
        auto cached = m_metacache.find(id);
        string metadata;
        if (cached != m_metacache.end()) {
            metadata = SoapArgs::xmlQuote(cached->second);
        } else {
            metadata = SoapArgs::xmlQuote(didlmake(song));
        }
        data.addarg("Uri", song.uri);
        data.addarg("Metadata", metadata);
    }
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

// Given a space separated list of track Id's, report their associated
// uri and metadata in the following xml form:
//
//  <TrackList>
//    <Entry>
//      <Id></Id>
//      <Uri></Uri>
//      <Metadata></Metadata>
//    </Entry>
//  </TrackList>
//
// Any ids not in the playlist are ignored. 
int OHPlaylist::readList(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::readList" << endl);
    string sids;
    bool ok = sc.getString("IdList", &sids);
    vector<string> ids;
    string out("<TrackList>");
    if (ok) {
        stringToTokens(sids, ids);
        for (auto& sid : ids) {
            int id = atoi(sid.c_str());
            UpSong song;
            if (!m_dev->m_mpdcli->statSong(song, id, true))
                continue;
            auto mit = m_metacache.find(id);
            string metadata;
            if (mit != m_metacache.end()) {
                metadata = SoapArgs::xmlQuote(mit->second);
            } else {
                metadata = SoapArgs::xmlQuote(didlmake(song));
            }
            out += "<Entry><Id>";
            out += sid.c_str();
            out += "</Id><Uri>";
            out += song.uri;
            out += "</Uri><Metadata>";
            out += metadata;
            out += "</Metadata></Entry>";
        }
        out += "</TrackList>";
        data.addarg("TrackList", out);
    }
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

// Adds the given uri and metadata as a new track to the playlist. 
// Set the AfterId argument to 0 to insert a track at the start of the
// playlist.
// Reports a 800 fault code if AfterId is not 0 and doesn’t appear in
// the playlist.
// Reports a 801 fault code if the playlist is full (i.e. already
// contains TracksMax tracks).
int OHPlaylist::insert(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::insert" << endl);
    int afterid;
    string uri, metadata;
    bool ok = sc.getInt("AfterId", &afterid);
    ok = ok && sc.getString("Uri", &uri);
    if (ok)
        ok = ok && sc.getString("Metadata", &metadata);

    LOGDEB("OHPlaylist::insert: afterid " << afterid << " Uri " <<
           uri << " Metadata " << metadata << endl);
    if (ok) {
        int id = m_dev->m_mpdcli->insertAfterId(uri, afterid);
        if ((ok = (id != -1))) {
            m_metacache[id] = metadata;
            data.addarg("NewId", SoapArgs::i2s(id));
        }
    }
    maybeWakeUp(ok);
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::deleteId(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::deleteId" << endl);
    int id;
    bool ok = sc.getInt("Value", &id);
    if (ok) {
        ok = m_dev->m_mpdcli->deleteId(id);
        maybeWakeUp(ok);
    }
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::deleteAll(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::deleteAll" << endl);
    bool ok = m_dev->m_mpdcli->clearQueue();
    maybeWakeUp(ok);
    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::tracksMax(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::tracksMax" << endl);
    data.addarg("Value", SoapArgs::i2s(tracksmax));
    return UPNP_E_SUCCESS;
}

// Returns current list of id as array of big endian 32bits integers,
// base-64-encoded. 
int OHPlaylist::idArray(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::idArray" << endl);
    data.addarg("Token", "0");
    data.addarg("Array", makeIdArray());
    return UPNP_E_SUCCESS;
}

// Check if id array changed since last call (which returned a gen token)
int OHPlaylist::idArrayChanged(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::idArrayChanged" << endl);
    int token;
    bool ok = sc.getInt("Token", &token);
    // Bool indicating if array changed
    data.addarg("Value", "0");

    return ok ? UPNP_E_SUCCESS : UPNP_E_INTERNAL_ERROR;
}

int OHPlaylist::protocolInfo(const SoapArgs& sc, SoapData& data)
{
    LOGDEB("OHPlaylist::protocolInfo" << endl);
    data.addarg("Value", upmpdProtocolInfo);
    return UPNP_E_SUCCESS;
}
