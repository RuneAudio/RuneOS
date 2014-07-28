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
#ifndef _RENDERING_H_X_INCLUDED_
#define _RENDERING_H_X_INCLUDED_

#include <string>

#include "libupnpp/device.hxx"

class UpMpd;

class UpMpdRenderCtl : public UpnpService {
public:
    UpMpdRenderCtl(UpMpd *dev);

    virtual bool getEventData(bool all, std::vector<std::string>& names, 
                              std::vector<std::string>& values);
    int getvolume_i();
    void setvolume_i(int volume);
    void setmute_i(bool onoff);
private:
    bool rdstateMToU(unordered_map<string, string>& status);
    int setMute(const SoapArgs& sc, SoapData& data);
    int getMute(const SoapArgs& sc, SoapData& data);
    int setVolume(const SoapArgs& sc, SoapData& data, bool isDb);
    int getVolume(const SoapArgs& sc, SoapData& data, bool isDb);
    int listPresets(const SoapArgs& sc, SoapData& data);
    int selectPreset(const SoapArgs& sc, SoapData& data);

    UpMpd *m_dev;
    // Desired volume target. We may delay executing small volume
    // changes to avoid saturating with small requests.
    int m_desiredvolume;
    // State variable storage
    unordered_map<string, string> m_rdstate;
};

#endif /* _RENDERING_H_X_INCLUDED_ */
