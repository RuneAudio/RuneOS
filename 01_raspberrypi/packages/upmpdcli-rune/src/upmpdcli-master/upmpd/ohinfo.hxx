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
#ifndef _OHINFO_H_X_INCLUDED_
#define _OHINFO_H_X_INCLUDED_

#include <string>

#include "libupnpp/device.hxx"

class UpMpd;

class OHInfo : public UpnpService {
public:
    OHInfo(UpMpd *dev);

    virtual bool getEventData(bool all, std::vector<std::string>& names, 
                              std::vector<std::string>& values);
private:
    int counters(const SoapArgs& sc, SoapData& data);
    int track(const SoapArgs& sc, SoapData& data);
    int details(const SoapArgs& sc, SoapData& data);
    int metatext(const SoapArgs& sc, SoapData& data);

    bool makestate(unordered_map<string, string>& state);
    void urimetadata(string& uri, string& metadata);
    void makedetails(string &duration, string& bitrate, 
                     string& bitdepth, string& samplerate);

    // State variable storage
    unordered_map<string, string> m_state;
    UpMpd *m_dev;
};

#endif /* _OHINFO_H_X_INCLUDED_ */
