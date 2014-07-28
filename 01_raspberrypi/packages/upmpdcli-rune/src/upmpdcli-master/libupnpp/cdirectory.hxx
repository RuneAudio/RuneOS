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
#ifndef _UPNPDIR_HXX_INCLUDED_
#define _UPNPDIR_HXX_INCLUDED_

#include <string>
#include <set>

#include "description.hxx"
#include "cdircontent.hxx"

/**
 * Content Directory Service class.
 *
 * This stores identity data from a directory service
 * and the device it belongs to, and has methods to query
 * the directory, using libupnp for handling the UPnP protocols.
 *
 * Note: m_rdreqcnt: number of entries requested per directory read.
 * 0 means all entries. The device can still return less entries than
 * requested, depending on its own limits. In general it's not optimal
 * becauses it triggers issues, and is sometimes actually slower, e.g. on
 * a D-Link NAS 327
 *
 * The value chosen may affect by the UpnpSetMaxContentLength
 * (2000*1024) done during initialization, but this should be ample
 */
class ContentDirectoryService {
public:
	/** Construct by copying data from device and service objects.
	 *
	 * To be used by the discovery service. 
	 * External users must call getDirServices() instead
	 */
	ContentDirectoryService(const UPnPDevice& device,
				const UPnPService& service)
		: m_actionURL(caturl(device.URLBase, service.controlURL)),
		  m_serviceType(service.serviceType),
		  m_deviceId(device.UDN),
		  m_friendlyName(device.friendlyName),
		  m_manufacturer(device.manufacturer),
		  m_modelName(device.modelName),
		  m_rdreqcnt(200)
	{
		if (!m_modelName.compare("MediaTomb")) {
			// Readdir by 200 entries is good for most, but MediaTomb likes
			// them really big. Actually 1000 is better but I don't dare
			m_rdreqcnt = 500;
		}
	}

	/** An empty one */
	ContentDirectoryService() {}

	/** Read a container's children list into dirbuf.
	 *
	 * @param objectId the UPnP object Id for the container. Root has Id "0"
	 * @param[out] dirbuf stores the entries we read.
	 * @return UPNP_E_SUCCESS for success, else libupnp error code.
	 */
	int readDir(const std::string& objectId, UPnPDirContent& dirbuf);

	int readDirSlice(const string& objectId, int offset,
					 int count, UPnPDirContent& dirbuf,
					 int *didread, int *total);

	/** Search the content directory service.
	 *
	 * @param objectId the UPnP object Id under which the search
	 * should be done. Not all servers actually support this below
	 * root. Root has Id "0"
	 * @param searchstring an UPnP searchcriteria string. Check the
	 * UPnP document: UPnP-av-ContentDirectory-v1-Service-20020625.pdf
	 * section 2.5.5. Maybe we'll provide an easier way some day...
	 * @param[out] dirbuf stores the entries we read.
	 * @return UPNP_E_SUCCESS for success, else libupnp error code.
	 */
	int search(const std::string& objectId, const std::string& searchstring,
		   UPnPDirContent& dirbuf);

	/** Read metadata for a given node.
	 *
	 * @param objectId the UPnP object Id. Root has Id "0"
	 * @param[out] dirbuf stores the entries we read. At most one entry will be
	 *	 returned.
	 * @return UPNP_E_SUCCESS for success, else libupnp error code.
	 */
	int getMetadata(const std::string& objectId, UPnPDirContent& dirbuf);

	/** Retrieve search capabilities
	 *
	 * @param[out] result an empty vector: no search, or a single '*' element:
	 *	   any tag can be used in a search, or a list of usable tag names.
	 * @return UPNP_E_SUCCESS for success, else libupnp error code.
	 */
	int getSearchCapabilities(std::set<std::string>& result);

	/** Retrieve the "friendly name" for this server, useful for display. */
	std::string getFriendlyName() const {return m_friendlyName;}

private:
	std::string m_actionURL;
	std::string m_serviceType;
	std::string m_deviceId;
	std::string m_friendlyName;
	std::string m_manufacturer;
	std::string m_modelName;

	int m_rdreqcnt; // Slice size to use when reading
};

#endif /* _UPNPDIR_HXX_INCLUDED_ */
/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
