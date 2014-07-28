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

#ifndef _UPMPD_H_X_INCLUDED_
#define _UPMPD_H_X_INCLUDED_

class MPDCli;
class MpdStatus;

// The UPnP MPD frontend device with its 3 services
class UpMpd : public UpnpDevice {
public:
    friend class UpMpdRenderCtl;
    friend class UpMpdAVTransport;
    friend class OHInfo;
    friend class OHPlaylist;

    enum Options {
        upmpdNone,
        // If set, the MPD queue belongs to us, we shall clear
        // it as we like.
        upmpdOwnQueue, 
        // Export OpenHome services
        upmpdDoOH,
    };
    UpMpd(const string& deviceid, const string& friendlyname,
          const unordered_map<string, string>& xmlfiles,
          MPDCli *mpdcli, unsigned int opts = upmpdNone);
    ~UpMpd();

    const MpdStatus &getMpdStatus();
    const MpdStatus &getMpdStatusNoUpdate()
        {
            return *m_mpds;
        }

private:
    MPDCli *m_mpdcli;
    const MpdStatus *m_mpds;

    unsigned int m_options;
    vector<UpnpService*> m_services;
};


extern string upmpdProtocolInfo;

#endif /* _UPMPD_H_X_INCLUDED_ */
