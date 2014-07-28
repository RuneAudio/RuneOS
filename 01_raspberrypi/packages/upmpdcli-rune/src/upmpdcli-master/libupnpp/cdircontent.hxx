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
#ifndef _UPNPDIRCONTENT_H_X_INCLUDED_
#define _UPNPDIRCONTENT_H_X_INCLUDED_

#include <string>
#include <vector>
#include <map>
#include <sstream>

/**
 * UpnP Media Server directory entry, converted from XML data.
 *
 * This is a dumb data holder class, a struct with helpers.
 */
class UPnPDirObject {
public:
	enum ObjType {item, container};
	// There are actually several kinds of containers:
	// object.container.storageFolder, object.container.person,
	// object.container.playlistContainer etc., but they all seem to
	// behave the same as far as we're concerned. Otoh, musicTrack
	// items are special to us, and so should playlists, but I've not
	// seen one of the latter yet (servers seem to use containers for
	// playlists).
	enum ItemClass {audioItem_musicTrack, audioItem_playlist};

	std::string m_id; // ObjectId
	std::string m_pid; // Parent ObjectId
	std::string m_title; // dc:title. Directory name for a container.
	ObjType m_type; // item or container
	ItemClass m_iclass;
	// Properties as gathered from the XML document (url, artist, etc.)
	// The map keys are the XML tag or attribute names.
	std::map<std::string, std::string> m_props;

	/** Get named property
	 * @param property name (e.g. upnp:artist, upnp:album,
	 *	   upnp:originalTrackNumber, upnp:genre). Use m_title instead
	 *	   for dc:title.
	 * @param[out] value
	 * @return true if found.
	 */
	bool getprop(const string& name, string& value) const
	{
		std::map<std::string, std::string>::const_iterator it =
			m_props.find(name);
		if (it == m_props.end())
			return false;
		value = it->second;
		return true;
	}

	void clear()
	{
		m_id.clear();
		m_pid.clear();
		m_title.clear();
		m_type = (ObjType)-1;
		m_iclass = (ItemClass)-1;
		m_props.clear();
	}
	std::string dump()
	{
		std::ostringstream os;
		os << "UPnPDirObject: id [" << m_id << "] pid [" << m_pid <<
			"] title [" << m_title << "] type [" <<
			(m_type == item ? "item" : "container") <<
			"] properties: " << std::endl;
		for (std::map<std::string,std::string>::const_iterator it =
				 m_props.begin(); it != m_props.end(); it++) {
			os << "[" << it->first << "]->[" << it->second << "] "
			   << std::endl;
		}
		os << std::endl;
		return os.str();
	}
};

/**
 * Image of a MediaServer Directory Service container (directory),
 * possibly containing items and subordinate containers.
 */
class UPnPDirContent {
public:
	std::vector<UPnPDirObject> m_containers;
	std::vector<UPnPDirObject> m_items;

	/**
	 * Parse from DIDL-Lite XML data.
	 *
	 * Normally only used by ContentDirectoryService::readDir()
	 * This is cumulative: in general, the XML data is obtained in
	 * several documents corresponding to (offset,count) slices of the
	 * directory (container). parse() can be called repeatedly with
	 * the successive XML documents and will accumulate entries in the item
	 * and container vectors. This makes more sense if the different
	 * chunks are from the same container, but given that UPnP Ids are
	 * actually global, nothing really bad will happen if you mix
	 * up...
	 */
	bool parse(const std::string& didltext);
};

#endif /* _UPNPDIRCONTENT_H_X_INCLUDED_ */
/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
