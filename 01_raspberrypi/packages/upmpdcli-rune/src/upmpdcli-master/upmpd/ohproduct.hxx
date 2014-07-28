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
#ifndef _OHPRODUCT_H_X_INCLUDED_
#define _OHPRODUCT_H_X_INCLUDED_

#include <string>

#include "libupnpp/device.hxx"

class UpMpd;

class OHProduct : public UpnpService {
public:
    OHProduct(UpMpd *dev, const string& friendlyname);

    virtual bool getEventData(bool all, std::vector<std::string>& names, 
                              std::vector<std::string>& values);
private:
    int manufacturer(const SoapArgs& sc, SoapData& data);
    int model(const SoapArgs& sc, SoapData& data);
    int product(const SoapArgs& sc, SoapData& data);
    int standby(const SoapArgs& sc, SoapData& data);
    int setStandby(const SoapArgs& sc, SoapData& data);
    int sourceCount(const SoapArgs& sc, SoapData& data);
    int sourceXML(const SoapArgs& sc, SoapData& data);
    int sourceIndex(const SoapArgs& sc, SoapData& data);
    int setSourceIndex(const SoapArgs& sc, SoapData& data);
    int setSourceIndexByName(const SoapArgs& sc, SoapData& data);
    int source(const SoapArgs& sc, SoapData& data);
    int attributes(const SoapArgs& sc, SoapData& data);
    int sourceXMLChangeCount(const SoapArgs& sc, SoapData& data);

    UpMpd *m_dev;
    string m_roomOrName;
};

#endif /* _OHPRODUCT_H_X_INCLUDED_ */
