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
#ifndef _OHVOLUME_H_X_INCLUDED_
#define _OHVOLUME_H_X_INCLUDED_

#include <string>

#include "libupnpp/device.hxx"

class UpMpd;
class UpMpdRenderCtl;

class OHVolume : public UpnpService {
public:
    OHVolume(UpMpd *dev, UpMpdRenderCtl *ctl);

    virtual bool getEventData(bool all, std::vector<std::string>& names, 
                              std::vector<std::string>& values);
private:
    int characteristics(const SoapArgs& sc, SoapData& data);
    int setVolume(const SoapArgs& sc, SoapData& data);
    int volume(const SoapArgs& sc, SoapData& data);
    int volumeInc(const SoapArgs& sc, SoapData& data);
    int volumeDec(const SoapArgs& sc, SoapData& data);
    int volumeLimit(const SoapArgs& sc, SoapData& data);
    int mute(const SoapArgs& sc, SoapData& data);
    int setMute(const SoapArgs& sc, SoapData& data);

    bool makestate(unordered_map<string, string> &st);
    // State variable storage
    unordered_map<string, string> m_state;
    UpMpd *m_dev;
    UpMpdRenderCtl *m_ctl;
};

#endif /* _OHVOLUME_H_X_INCLUDED_ */
