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
#ifndef _DEVICE_H_X_INCLUDED_
#define _DEVICE_H_X_INCLUDED_

#include <unordered_map>
#include <functional>

#include "soaphelp.hxx"

class UpnpService;

typedef function<int (const SoapArgs&, SoapData&)> soapfun;

/** Define a virtual interface to link libupnp operations to a device 
 * implementation 
 */
class UpnpDevice {
public:
    UpnpDevice(const std::string& deviceId, 
               const std::unordered_map<std::string, std::string>& xmlfiles);

    void addService(UpnpService *, const std::string& serviceId);

    /**
     * Add mapping from service+action-name to handler function.
     */
    void addActionMapping(const UpnpService*, 
                          const std::string& actName, soapfun);

    /** 
     * Generate event.
     *
     * To be called by the device layer when data changes and an
     * event should happen. Use is not mandatory because the polling by 
     * getEventData() may be sufficient.
     */
    void notifyEvent(const std::string& serviceId,
                     const std::vector<std::string>& names, 
                     const std::vector<std::string>& values);

    /** 
     * Main routine. To be called by main() when done with initialization. 
     *
     * This loop mostly polls getEventData and generates an UPnP event if
     * there is anything to broadcast. The UPnP action calls happen in
     * other threads with which we synchronize, currently using a global lock.
     */
    void eventloop();

    /** 
     * To be called from a service action callback to wake up the
     * event loop early if something needs to be broadcast without
     * waiting for the normal delay.
     *
     * Will only do something if the previous event is not too recent.
     */
    void loopWakeup(); // To trigger an early event

    /** Check status */
    bool ok() {return m_lib != 0;}

private:
    const std::string& serviceType(const std::string& serviceId);
            
    LibUPnP *m_lib;
    std::string m_deviceId;
    // We keep the services in a map for easy access from id and in a
    // vector for ordered walking while fetching status. Order is
    // determine by addService() call sequence.
    std::unordered_map<std::string, UpnpService*> m_servicemap;
    std::vector<std::string> m_serviceids;
    std::unordered_map<std::string, soapfun> m_calls;

    static unordered_map<std::string, UpnpDevice *> o_devices;

    /* Static callback for libupnp. This looks up the appropriate
     * device using the device ID (UDN), the calls its callback
     * method */
    static int sCallBack(Upnp_EventType et, void* evp, void*);

    /* Gets called when something needs doing */
    int callBack(Upnp_EventType et, void* evp);
};

/**
 * Upnp service implementation class. This does not do much useful beyond
 * encapsulating the service actions and event callback. In most cases, the
 * services will need full access to the device state anyway.
 */
class UpnpService {
public:
    UpnpService(const std::string& stp,const std::string& sid, UpnpDevice *dev) 
        : m_serviceType(stp), m_serviceId(sid)
        {
            dev->addService(this, sid);
        }
    virtual ~UpnpService() {}

    /** 
     * Poll to retrieve evented data changed since last call.
     *
     * To be implemented by the derived class.
     * Called by the library when a control point subscribes, to
     * retrieve eventable data. 
     * Return name/value pairs for changed variables in the data arrays.
     */
    virtual bool getEventData(bool all, std::vector<std::string>& names, 
                              std::vector<std::string>& values) 
        {
            return true;
        }

    virtual const std::string& getServiceType() const
        {
            return m_serviceType;
        }
    virtual const std::string& getServiceId() const
        {
            return m_serviceId;
        }

protected:
    const std::string m_serviceType;
    const std::string m_serviceId;
};

#endif /* _DEVICE_H_X_INCLUDED_ */
