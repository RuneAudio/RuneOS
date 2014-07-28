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
#ifndef _UPNPPDISC_H_X_INCLUDED_
#define _UPNPPDISC_H_X_INCLUDED_

#include <vector>

#include "cdirectory.hxx"

/**
 * Manage UPnP discovery and maintain a directory of active devices. Singleton.
 *
 * We are only interested in MediaServers with a ContentDirectory service
 * for now, but this could be made more general, by removing the filtering.
 */
class UPnPDeviceDirectory {
public:
	/** Retrieve the singleton object for the discovery service,
	 * and possibly start it up if this is the first call.
	 *
	 * This initializes the discovery service on first call, starting
	 * the message-handling thread, registering our message handlers, 
	 * and initiating an asynchronous UPnP device search. 
	 *
	 * The search implies a timeout period (the specified interval
	 * over which the servers will send replies at random points). Any
	 * subsequent getDirServices() call will block until the timeout
	 * is expired, so that the client can choose to do something else
	 * to use the time before getDirServices() can be hoped to return
	 * immediate results. Use getRemainingDelay() to know the current
	 * state of things.
	 *
	 * We need a separate thread to process the messages coming up
	 * from libupnp, because some of them will in turn trigger other
	 * calls to libupnp, and this must not be done from the libupnp
	 * thread context which reported the initial message.
	 */
	static UPnPDeviceDirectory *getTheDir(time_t search_window = 1);

	/** Clean up before exit. Do call this.*/
	static void terminate();

	/** Retrieve the directory services currently seen on the network */
	bool getDirServices(std::vector<ContentDirectoryService>&);
	/** Retrieve specific service designated by its friendlyName */
	bool getServer(const string& friendlyName, ContentDirectoryService& server);

	/** My health */
	bool ok() {return m_ok;}
	/** My diagnostic if health is bad */
	const std::string getReason() {return m_reason;}

	/** Remaining time until current search complete */
	time_t getRemainingDelay();

private:
	UPnPDeviceDirectory(time_t search_window);
	UPnPDeviceDirectory(const UPnPDeviceDirectory &);
	UPnPDeviceDirectory& operator=(const UPnPDeviceDirectory &);
	bool search();
	void expireDevices();

	bool m_ok;
	std::string m_reason;
	int m_searchTimeout;
	time_t m_lastSearch;
};


#endif /* _UPNPPDISC_H_X_INCLUDED_ */
/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
