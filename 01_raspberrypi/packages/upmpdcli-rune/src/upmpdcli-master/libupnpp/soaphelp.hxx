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
#ifndef _SOAPHELP_H_X_INCLUDED_
#define _SOAPHELP_H_X_INCLUDED_

#include <vector>
#include <string>
#include <map>

#include <upnp/ixml.h>

/** Store returned values after decoding the arguments in a SOAP Call */
class SoapArgs {
public:
    std::string name;
    std::map<std::string, std::string> args;

    // Utility methods
    bool getBool(const char *nm, bool *value) const;
    bool getInt(const char *nm, int *value) const;
    bool getString(const char *nm, std::string *value) const;
    static std::string xmlQuote(const std::string& in);
    static std::string i2s(int val);
};

/** Decode the XML in a Soap call and return the arguments in a SoapArgs 
 * structure.
 *
 * @param name the action name is stored for convenience in the return
 * structure. The caller normally gets it from libupnp, passing it is simpler
 * than retrieving from the input top node where it has a namespace qualifier.
 * @param actReq the XML document containing the SOAP data.
 * @param[output] res the decoded data.
 * @return true for success, false if a decoding error occurred.
 */
extern bool decodeSoapBody(const char *name, IXML_Document *actReq, 
                           SoapArgs *res);

/** Store the values to be encoded in a SOAP response. 
 *
 * The elements in the response must be in a defined order, so we
 * can't use a map as container, we use a vector of pairs instead.
 * The generic UpnpDevice callback fills up name and service type, the
 * device call only needs to fill the data vector.
 */
class SoapData {
public:
    std::string name;
    std::string serviceType;
    std::vector<std::pair<std::string, std::string> > data;
    void addarg(const std::string& k, const std::string& v) {
        data.push_back(pair<string,string>(k, v));
    }
};

/** Build a SOAP response data XML document from a list of values */
extern IXML_Document *buildSoapBody(SoapData& data);

#endif /* _SOAPHELP_H_X_INCLUDED_ */
