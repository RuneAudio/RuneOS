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
#ifndef _LIBUPNP_H_X_INCLUDED_
#define _LIBUPNP_H_X_INCLUDED_

#include <string>
#include <map>

#include <upnp/upnp.h>
#include <upnp/upnptools.h>

#include "ptmutex.hxx"

/** Our link to libupnp. Initialize and keep the handle around */
class LibUPnP {
public:
	~LibUPnP();

	/** Retrieve the singleton LibUPnP object 
	 *
	 * This initializes libupnp, possibly setting an address and port, possibly
	 * registering a client if serveronly is false.
	 *
	 * @param serveronly no client init
	 * @param hwaddr returns the hardware address for the specified network 
	 *   interface, or the first one is ifname is empty. If the IP address is
	 *   specified instead of the interface name, the hardware address
	 *   returned is not necessarily the one matching the IP.
	 * @param ifname if not empty, network interface to use. Translated to
	 *   IP address for the UpnpInit() call.
	 * @param ip if not empty, IP address to use. Only used if ifname is empty.
	 * @param port   port parameter to UpnpInit() (0 for default).
	 * @return 0 for failure.
	 */
	static LibUPnP* getLibUPnP(bool serveronly = false, std::string* hwaddr = 0,
							   const std::string ifname = string(),
							   const std::string ip = string(),
							   unsigned short port = 0);

	/** Set libupnp log file name and activate logging.
	 *
	 * @param fn file name to use. Use empty string to turn logging off
	 */
	enum LogLevel {LogLevelNone, LogLevelError, LogLevelDebug};
	bool setLogFileName(const std::string& fn, LogLevel level = LogLevelError);
	bool setLogLevel(LogLevel level);

	/** Set max library buffer size for reading content from servers. */
	void setMaxContentLength(int bytes);

	/** Check state after initialization */
	bool ok() const
	{
		return m_ok;
	}

	/** Retrieve init error if state not ok */
	int getInitError() const
	{
		return m_init_error;
	}

	/** Build a unique persistent UUID for a root device. This uses a hash
		of the input name (e.g.: friendlyName), and the host Ethernet address */
	static std::string makeDevUUID(const std::string& name, const std::string& hw);

	/** Translate libupnp integer error code (UPNP_E_XXX) to string */
	static std::string errAsString(const std::string& who, int code);

/////////////////////////////////////////////////////////////////////////////
	/* The methods which follow are normally for use by the 
	 * intermediate layers in libupnpp, such as the base device class
	 * or the server directory, end-user code should not need them in
	 * general.
	 */

	/** Specify function to be called on given UPnP event. This will happen
	 * in the libupnp thread context. 
	 */
	void registerHandler(Upnp_EventType et, Upnp_FunPtr handler, void *cookie);

	/** Translate libupnp event type as string */
	static std::string evTypeAsString(Upnp_EventType);

	int setupWebServer(const std::string& description);

	UpnpClient_Handle getclh()
	{
		return m_clh;
	}
	UpnpDevice_Handle getdvh()
	{
		return m_dvh;
	}
private:

	// A Handler object records the data from registerHandler.
	class Handler {
	public:
		Handler()
			: handler(0), cookie(0) {}
		Handler(Upnp_FunPtr h, void *c)
			: handler(h), cookie(c) {}
		Upnp_FunPtr handler;
		void *cookie;
	};

	
	LibUPnP(bool serveronly, std::string *hwaddr, 
			const std::string ifname, const std::string ip,
			unsigned short port);

	LibUPnP(const LibUPnP &);
	LibUPnP& operator=(const LibUPnP &);

	static int o_callback(Upnp_EventType, void *, void *);

	bool m_ok;
	int	 m_init_error;
	UpnpClient_Handle m_clh;
	UpnpDevice_Handle m_dvh;
	PTMutexInit m_mutex;
	std::map<Upnp_EventType, Handler> m_handlers;
};

#endif /* _LIBUPNP.H_X_INCLUDED_ */
/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
