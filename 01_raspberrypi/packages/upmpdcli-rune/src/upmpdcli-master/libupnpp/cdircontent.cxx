/* Copyright (C) 2013 J.F.Dockes
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

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <set>
using std::string;
using std::cerr;
using std::endl;
using std::vector;
using std::map;
using std::set;

#include "expatmm.hxx"
#include "upnpp_p.hxx"
#include "cdircontent.hxx"

#if 0
static void listmap(const map<string,string>& mp)
{
	for (map<string,string>::const_iterator it = mp.begin();
		 it != mp.end(); it++) {
		cerr << "[" << it->first << "]->[" << it->second << "] ";
	}
}
#endif

static const char* upnptags[] = {
	"upnp:artist",
	"upnp:album",
	"upnp:genre",
	"upnp:originalTrackNumber",
	"upnp:class",
};
static const int nupnptags = sizeof(upnptags) / sizeof(char*);

// An XML parser which builds directory contents from DIDL lite input.
class UPnPDirParser : public expatmm::inputRefXMLParser {
public:
	UPnPDirParser(UPnPDirContent& dir, const string& input)
		: inputRefXMLParser(input), m_dir(dir)
		{
			m_okitems["object.item.audioItem.musicTrack"] =
				UPnPDirObject::audioItem_musicTrack;
			m_okitems["object.item.playlistItem"] =
				UPnPDirObject::audioItem_playlist;
		}
	UPnPDirContent& m_dir;

protected:
	class StackEl {
	public:
		StackEl(const string& nm) : name(nm) {}
		string name;
		map<string,string> attributes;
	};

	virtual void StartElement(const XML_Char *name, const XML_Char **attrs)
		{
			//cerr << "startElement: name [" << name << "]" << endl;

			m_path.push_back(StackEl(name));
			for (int i = 0; attrs[i] != 0; i += 2) {
				m_path.back().attributes[attrs[i]] = attrs[i+1];
			}

			switch (name[0]) {
			case 'c':
				if (!strcmp(name, "container")) {
					m_tobj.clear();
					m_tobj.m_type = UPnPDirObject::container;
					m_tobj.m_id = m_path.back().attributes["id"];
					m_tobj.m_pid = m_path.back().attributes["parentID"];
				}
				break;
			case 'i':
				if (!strcmp(name, "item")) {
					m_tobj.clear();
					m_tobj.m_type = UPnPDirObject::item;
					m_tobj.m_id = m_path.back().attributes["id"];
					m_tobj.m_pid = m_path.back().attributes["parentID"];
				}
				break;
			default:
				break;
			}
		}

	virtual bool checkobjok()
		{
			bool ok =  !m_tobj.m_id.empty() && !m_tobj.m_pid.empty() &&
				!m_tobj.m_title.empty();

			if (ok && m_tobj.m_type == UPnPDirObject::item) {
				map<string, UPnPDirObject::ItemClass>::const_iterator it;
				it = m_okitems.find(m_tobj.m_props["upnp:class"]);
				if (it == m_okitems.end()) {
					PLOGINF("checkobjok: found object of unknown class: [%s]\n",
							m_tobj.m_props["upnp:class"].c_str());
					ok = false;
				} else {
					m_tobj.m_iclass = it->second;
				}
			}

			if (!ok) {
				PLOGINF("checkobjok: skip: id [%s] pid [%s] clss [%s] tt [%s]\n",
						m_tobj.m_id.c_str(), m_tobj.m_pid.c_str(),
						m_tobj.m_props["upnp:class"].c_str(),
						m_tobj.m_title.c_str());
			}
			return ok;
		}
	virtual void EndElement(const XML_Char *name)
		{
			if (!strcmp(name, "container")) {
				if (checkobjok()) {
					//cerr << "Pushing container: " << m_tobj.m_title << endl;
					m_dir.m_containers.push_back(m_tobj);
				}
			} else if (!strcmp(name, "item")) {
				if (checkobjok()) {
					//cerr << "Pushing item: " << m_tobj.m_title << endl;
					m_dir.m_items.push_back(m_tobj);
				}
			} else if (!strcmp(name, "res")) {
				// <res protocolInfo="http-get:*:audio/mpeg:*" size="5171496"
				// bitrate="24576" duration="00:03:35" sampleFrequency="44100"
				// nrAudioChannels="2">
				string s;
				s="protocolInfo";m_tobj.m_props[s] = m_path.back().attributes[s];
				s="size";m_tobj.m_props[s] = m_path.back().attributes[s];
				s="bitrate";m_tobj.m_props[s] = m_path.back().attributes[s];
				s="duration";m_tobj.m_props[s] = m_path.back().attributes[s];
				s="sampleFrequency";m_tobj.m_props[s] = m_path.back().attributes[s];
				s="nrAudioChannels";m_tobj.m_props[s] = m_path.back().attributes[s];
			}

			m_path.pop_back();
		}

	virtual void CharacterData(const XML_Char *s, int len)
		{
			if (s == 0 || *s == 0)
				return;
			string str(s, len);
			trimstring(str);
			switch (m_path.back().name[0]) {
			case 'd':
				if (!m_path.back().name.compare("dc:title"))
					m_tobj.m_title += str;
				break;
			case 'r':
				if (!m_path.back().name.compare("res")) {
					m_tobj.m_props["url"] += str;
				}
				break;
			case 'u':
				for (int i = 0; i < nupnptags; i++) {
					if (!m_path.back().name.compare(upnptags[i])) {
						m_tobj.m_props[upnptags[i]] += str;
					}
				}
				break;
			}
		}

private:
	vector<StackEl> m_path;
	UPnPDirObject m_tobj;
	map<string, UPnPDirObject::ItemClass> m_okitems;
};

bool UPnPDirContent::parse(const std::string& input)
{
	UPnPDirParser parser(*this, input);
	return parser.Parse();
}
/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
