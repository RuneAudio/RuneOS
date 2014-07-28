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
#include "config.h"

#include <iostream>
using namespace std;

#include "soaphelp.hxx"


/* Example Soap XML doc passed by libupnp is like: 
   <ns0:SetMute>
     <InstanceID>0</InstanceID>
     <Channel>Master</Channel>
     <DesiredMute>False</DesiredMute>
   </ns0:SetMute>
   
   As the top node name is qualified by a namespace, it's easier to just use 
   action name passed in the libupnp action callback.
*/
bool decodeSoapBody(const char *callnm, IXML_Document *actReq, 
                    SoapArgs *res)
{
    bool ret = false;
    IXML_NodeList* nl = 0;
    IXML_Node* topNode = 
        ixmlNode_getFirstChild((IXML_Node *)actReq);
    if (topNode == 0) {
        cerr << "decodeSoap: Empty Action request (no topNode) ??" << endl;
        return false;
    }
    // cerr << "decodeSoap: top node name: " << ixmlNode_getNodeName(topNode) 
    // << endl;

    nl = ixmlNode_getChildNodes(topNode);
    if (nl == 0) {
        // cerr << "decodeSoap: empty Action request (no childs)" << endl;
        // Ok actually, there are no args
        return true;
    }
    // cerr << "decodeSoap: childnodes list length: " << ixmlNodeList_length(nl)
    // << endl;

    for (unsigned long i = 0; i <  ixmlNodeList_length(nl); i++) {
        IXML_Node *cld = ixmlNodeList_item(nl, i);
        if (cld == 0) {
            // cerr << "decodeSoap: got null node  from nodelist at index " <<
            // i << " ??" << endl;
            // Seems to happen with empty arg list?? This looks like a bug, 
            // should we not get an empty node instead?
            if (i == 0) {
                ret = true;
            }
            goto out;
        }
        const char *name = ixmlNode_getNodeName(cld);
        if (cld == 0) {
            DOMString pnode = ixmlPrintNode(cld);
            cerr << "decodeSoap: got null name ??:" << pnode << endl;
            ixmlFreeDOMString(pnode);
            goto out;
        }
        IXML_Node *txtnode = ixmlNode_getFirstChild(cld);
        const char *value = "";
        if (txtnode != 0) {
            value = ixmlNode_getNodeValue(txtnode);
        }
        // Can we get an empty value here ?
        if (value == 0)
            value = "";
        res->args[name] = value;
    }
    res->name = callnm;
    ret = true;
out:
    if (nl)
        ixmlNodeList_free(nl);
    return ret;
}

bool SoapArgs::getBool(const char *nm, bool *value) const
{
    map<string, string>::const_iterator it = args.find(nm);
    if (it == args.end() || it->second.empty()) {
        return false;
    }
    if (it->second[0] == 'F' || it->second[0] == 'f' || 
        it->second[0] == 'N' || it->second[0] == 'n' || 
        it->second[0] == '0') {
        *value = false;
    } else if (it->second[0] == 'T' || it->second[0] == 't' || 
               it->second[0] == 'Y' || it->second[0] == 'y' || 
               it->second[0] == '1') {
        *value = true;
    } else {
        return false;
    }
    return true;
}

bool SoapArgs::getInt(const char *nm, int *value) const
{
    map<string, string>::const_iterator it = args.find(nm);
    if (it == args.end() || it->second.empty()) {
        return false;
    }
    *value = atoi(it->second.c_str());
    return true;
}

bool SoapArgs::getString(const char *nm, string *value) const
{
    map<string, string>::const_iterator it = args.find(nm);
    if (it == args.end() || it->second.empty()) {
        return false;
    }
    *value = it->second;
    return true;
}

string SoapArgs::xmlQuote(const string& in)
{
    string out;
    for (unsigned int i = 0; i < in.size(); i++) {
        switch(in[i]) {
        case '"': out += "&quot;";break;
        case '&': out += "&amp;";break;
        case '<': out += "&lt;";break;
        case '>': out += "&gt;";break;
        case '\'': out += "&apos;";break;
        default: out += in[i];
        }
    }
    return out;
}

// Yes inefficient. whatever...
string SoapArgs::i2s(int val)
{
    char cbuf[30];
    sprintf(cbuf, "%d", val);
    return string(cbuf);
}

IXML_Document *buildSoapBody(SoapData& data)
{
    IXML_Document *doc = ixmlDocument_createDocument();
    if (doc == 0) {
        cerr << "buildSoapResponse: out of memory" << endl;
        return 0;
    }
    string topname = string("u:") + data.name + "Response";
    IXML_Element *top =  
        ixmlDocument_createElementNS(doc, data.serviceType.c_str(), 
                                     topname.c_str());
    ixmlElement_setAttribute(top, "xmlns:u", data.serviceType.c_str());

    for (unsigned i = 0; i < data.data.size(); i++) {
        IXML_Element *elt = 
            ixmlDocument_createElement(doc, data.data[i].first.c_str());
        IXML_Node* textnode = 
            ixmlDocument_createTextNode(doc, data.data[i].second.c_str());
        ixmlNode_appendChild((IXML_Node*)elt,(IXML_Node*)textnode);
        ixmlNode_appendChild((IXML_Node*)top,(IXML_Node*)elt);
    }

    ixmlNode_appendChild((IXML_Node*)doc,(IXML_Node*)top);
    
    return doc;
}

