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
#ifndef _OHPLAYLIST_H_X_INCLUDED_
#define _OHPLAYLIST_H_X_INCLUDED_

#include <string>

#include "libupnpp/device.hxx"

class UpMpd;
class UpMpdRenderCtl;

class OHPlaylist : public UpnpService {
public:
    OHPlaylist(UpMpd *dev, UpMpdRenderCtl *ctl);

    virtual bool getEventData(bool all, std::vector<std::string>& names, 
                              std::vector<std::string>& values);
private:
    int play(const SoapArgs& sc, SoapData& data);
    int pause(const SoapArgs& sc, SoapData& data);
    int stop(const SoapArgs& sc, SoapData& data);
    int next(const SoapArgs& sc, SoapData& data);
    int previous(const SoapArgs& sc, SoapData& data);
    int setRepeat(const SoapArgs& sc, SoapData& data);
    int repeat(const SoapArgs& sc, SoapData& data);
    int setShuffle(const SoapArgs& sc, SoapData& data);
    int shuffle(const SoapArgs& sc, SoapData& data);
    int seekSecondAbsolute(const SoapArgs& sc, SoapData& data);
    int seekSecondRelative(const SoapArgs& sc, SoapData& data);
    int seekId(const SoapArgs& sc, SoapData& data);
    int seekIndex(const SoapArgs& sc, SoapData& data);
    int transportState(const SoapArgs& sc, SoapData& data);
    int id(const SoapArgs& sc, SoapData& data);
    int ohread(const SoapArgs& sc, SoapData& data);
    int readList(const SoapArgs& sc, SoapData& data);
    int insert(const SoapArgs& sc, SoapData& data);
    int deleteId(const SoapArgs& sc, SoapData& data);
    int deleteAll(const SoapArgs& sc, SoapData& data);
    int tracksMax(const SoapArgs& sc, SoapData& data);
    int idArray(const SoapArgs& sc, SoapData& data);
    int idArrayChanged(const SoapArgs& sc, SoapData& data);
    int protocolInfo(const SoapArgs& sc, SoapData& data);

    std::string makeIdArray();
    bool makestate(unordered_map<string, string> &st);
    void maybeWakeUp(bool ok);
    // State variable storage
    unordered_map<string, string> m_state;
    UpMpd *m_dev;
    UpMpdRenderCtl *m_ctl;

    // Storage for song metadata, indexed by MPD song queue id. The
    // data is the DIDL XML string.
    std::unordered_map<int, std::string> m_metacache;
};

#endif /* _OHPLAYLIST_H_X_INCLUDED_ */
