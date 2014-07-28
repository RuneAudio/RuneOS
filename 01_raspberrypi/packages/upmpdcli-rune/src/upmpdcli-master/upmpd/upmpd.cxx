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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>
#include <set>
using namespace std;
using namespace std::placeholders;

#include "libupnpp/upnpplib.hxx"
#include "libupnpp/soaphelp.hxx"
#include "libupnpp/device.hxx"
#include "libupnpp/log.hxx"

#include "upmpd.hxx"
#include "mpdcli.hxx"
#include "upmpdutils.hxx"
#include "renderctl.hxx"
#include "avtransport.hxx"
#include "conman.hxx"
#include "ohproduct.hxx"
#include "ohinfo.hxx"
#include "ohtime.hxx"
#include "ohvolume.hxx"
#include "ohplaylist.hxx"

static const string dfltFriendlyName("UpMpd");
string upmpdProtocolInfo;

// Note: if we ever need this to work without cxx11, there is this:
// http://www.tutok.sk/fastgl/callback.html
UpMpd::UpMpd(const string& deviceid, const string& friendlyname,
			 const unordered_map<string, string>& xmlfiles,
			 MPDCli *mpdcli, unsigned int opts)
	: UpnpDevice(deviceid, xmlfiles), m_mpdcli(mpdcli), m_mpds(0),
	  m_options(opts)
{
	// Note: the order is significant here as it will be used when
	// calling the getStatus() methods, and we want AVTransport to
	// update the mpd status for OHInfo
	UpMpdRenderCtl *rdctl = new UpMpdRenderCtl(this);
	m_services.push_back(rdctl);
	m_services.push_back(new UpMpdAVTransport(this));
	m_services.push_back(new UpMpdConMan(this));
	if (m_options & upmpdDoOH) {
		m_services.push_back(new OHProduct(this, friendlyname));
		m_services.push_back(new OHInfo(this));
		m_services.push_back(new OHTime(this));
		m_services.push_back(new OHVolume(this, rdctl));
		m_services.push_back(new OHPlaylist(this, rdctl));
	}
}

UpMpd::~UpMpd()
{
	for (vector<UpnpService*>::iterator it = m_services.begin();
		 it != m_services.end(); it++) {
		delete(*it);
	}
}

const MpdStatus& UpMpd::getMpdStatus()
{
    m_mpds = &m_mpdcli->getStatus();
    return *m_mpds;
}

/////////////////////////////////////////////////////////////////////
// Main program

#include "conftree.hxx"

static const string ohDesc(
	"<service>"
	"  <serviceType>urn:av-openhome-org:service:Product:1</serviceType>"
	"  <serviceId>urn:av-openhome-org:serviceId:Product</serviceId>"
	"  <SCPDURL>/OHProduct.xml</SCPDURL>"
	"  <controlURL>/ctl/OHProduct</controlURL>"
	"  <eventSubURL>/evt/OHProduct</eventSubURL>"
	"</service>"
	"<service>"
	"  <serviceType>urn:av-openhome-org:service:Info:1</serviceType>"
	"  <serviceId>urn:av-openhome-org:serviceId:Info</serviceId>"
	"  <SCPDURL>/OHInfo.xml</SCPDURL>"
	"  <controlURL>/ctl/OHInfo</controlURL>"
	"  <eventSubURL>/evt/OHInfo</eventSubURL>"
	"</service>"
	"<service>"
	"  <serviceType>urn:av-openhome-org:service:Time:1</serviceType>"
	"  <serviceId>urn:av-openhome-org:serviceId:Time</serviceId>"
	"  <SCPDURL>/OHTime.xml</SCPDURL>"
	"  <controlURL>/ctl/OHTime</controlURL>"
	"  <eventSubURL>/evt/OHTime</eventSubURL>"
	"</service>"
	"<service>"
	"  <serviceType>urn:av-openhome-org:service:Volume:1</serviceType>"
	"  <serviceId>urn:av-openhome-org:serviceId:Volume</serviceId>"
	"  <SCPDURL>/OHVolume.xml</SCPDURL>"
	"  <controlURL>/ctl/OHVolume</controlURL>"
	"  <eventSubURL>/evt/OHVolume</eventSubURL>"
	"</service>"
	"<service>"
	"  <serviceType>urn:av-openhome-org:service:Playlist:1</serviceType>"
	"  <serviceId>urn:av-openhome-org:serviceId:Playlist</serviceId>"
	"  <SCPDURL>/OHPlaylist.xml</SCPDURL>"
	"  <controlURL>/ctl/OHPlaylist</controlURL>"
	"  <eventSubURL>/evt/OHPlaylist</eventSubURL>"
	"</service>"
	);

static char *thisprog;

static int op_flags;
#define OPT_MOINS 0x1
#define OPT_h	  0x2
#define OPT_p	  0x4
#define OPT_d	  0x8
#define OPT_D     0x10
#define OPT_c     0x20
#define OPT_l     0x40
#define OPT_f     0x80
#define OPT_q     0x100
#define OPT_i     0x200
#define OPT_P     0x400
#define OPT_O     0x800

static const char usage[] = 
"-c configfile \t configuration file to use\n"
"""""""""""""""""""""""""""""-h host    \t specify host MPD is running on\n"""""""""""""""""""""""""""""
"-p port     \t specify MPD port\n"
"-d logfilename\t debug messages to\n"
"-l loglevel\t  log level (0-6)\n"
"-D    \t run as a daemon\n"
"-f friendlyname\t define device displayed name\n"
"-q 0|1\t if set, we own the mpd queue, else avoid clearing it whenever we feel like it\n"
"-i iface    \t specify network interface name to be used for UPnP\n"
"-P upport    \t specify port number to be used for UPnP\n"
"-O 0|1\t decide if we run and export the OpenHome services\n"
"\n"
			;
static void
Usage(void)
{
	fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
	exit(1);
}

static string myDeviceUUID;

static string datadir(DATADIR "/");
static string configdir(CONFIGDIR "/");

// Our XML description data. !Keep description.xml first!
static vector<const char *> xmlfilenames = 
{
	/* keep first */ "description.xml", /* keep first */
	"RenderingControl.xml", "AVTransport.xml", "ConnectionManager.xml",
};
static vector<const char *> ohxmlfilenames = 
{
	"OHProduct.xml", "OHInfo.xml", "OHTime.xml", "OHVolume.xml", 
	"OHPlaylist.xml",
};


int main(int argc, char *argv[])
{
	string mpdhost("localhost");
	int mpdport = 6600;
	string mpdpassword;
	string logfilename;
	int loglevel(upnppdebug::Logger::LLINF);
	string configfile;
	string friendlyname(dfltFriendlyName);
	bool ownqueue = true;
	bool openhome = false;
	string upmpdcliuser("upmpdcli");
	string pidfilename("/var/run/upmpdcli.pid");
	string iface;
	unsigned short upport = 0;
	string upnpip;

	const char *cp;
	if ((cp = getenv("UPMPD_HOST")))
		mpdhost = cp;
	if ((cp = getenv("UPMPD_PORT")))
		mpdport = atoi(cp);
	if ((cp = getenv("UPMPD_FRIENDLYNAME")))
		friendlyname = atoi(cp);
	if ((cp = getenv("UPMPD_CONFIG")))
		configfile = cp;
	if ((cp = getenv("UPMPD_UPNPIFACE")))
		iface = cp;
	if ((cp = getenv("UPMPD_UPNPPORT")))
		upport = atoi(cp);

	thisprog = argv[0];
	argc--; argv++;
	while (argc > 0 && **argv == '-') {
		(*argv)++;
		if (!(**argv))
			Usage();
		while (**argv)
			switch (*(*argv)++) {
			case 'c':	op_flags |= OPT_c; if (argc < 2)  Usage();
				configfile = *(++argv); argc--; goto b1;
			case 'D':	op_flags |= OPT_D; break;
			case 'd':	op_flags |= OPT_d; if (argc < 2)  Usage();
				logfilename = *(++argv); argc--; goto b1;
			case 'f':	op_flags |= OPT_f; if (argc < 2)  Usage();
				friendlyname = *(++argv); argc--; goto b1;
			case 'h':	op_flags |= OPT_h; if (argc < 2)  Usage();
				mpdhost = *(++argv); argc--; goto b1;
			case 'i':	op_flags |= OPT_i; if (argc < 2)  Usage();
				iface = *(++argv); argc--; goto b1;
			case 'l':	op_flags |= OPT_l; if (argc < 2)  Usage();
				loglevel = atoi(*(++argv)); argc--; goto b1;
			case 'O': {
				op_flags |= OPT_O; 
				if (argc < 2)  Usage();
				const char *cp =  *(++argv);
				if (*cp == '1' || *cp == 't' || *cp == 'T' || *cp == 'y' || 
					*cp == 'Y')
					openhome = true;
				argc--; goto b1;
			}
			case 'P':	op_flags |= OPT_P; if (argc < 2)  Usage();
				upport = atoi(*(++argv)); argc--; goto b1;
			case 'p':	op_flags |= OPT_p; if (argc < 2)  Usage();
				mpdport = atoi(*(++argv)); argc--; goto b1;
			case 'q':	op_flags |= OPT_q; if (argc < 2)  Usage();
				ownqueue = atoi(*(++argv)) != 0; argc--; goto b1;
			default: Usage();	break;
			}
	b1: argc--; argv++;
	}

	if (argc != 0)
		Usage();

	if (!configfile.empty()) {
		ConfSimple config(configfile.c_str(), 1, true);
		if (!config.ok()) {
			cerr << "Could not open config: " << configfile << endl;
			return 1;
		}
		string value;
		if (!(op_flags & OPT_d))
			config.get("logfilename", logfilename);
		if (!(op_flags & OPT_f))
			config.get("friendlyname", friendlyname);
		if (!(op_flags & OPT_l) && config.get("loglevel", value))
			loglevel = atoi(value.c_str());
		if (!(op_flags & OPT_h))
			config.get("mpdhost", mpdhost);
		if (!(op_flags & OPT_p) && config.get("mpdport", value)) {
			mpdport = atoi(value.c_str());
		}
		config.get("mpdpassword", mpdpassword);
		if (!(op_flags & OPT_q) && config.get("ownqueue", value)) {
			ownqueue = atoi(value.c_str()) != 0;
		}
		if (config.get("openhome", value)) {
			openhome = atoi(value.c_str()) != 0;
		}
		if (!(op_flags & OPT_i)) {
			config.get("upnpiface", iface);
			if (iface.empty()) {
				config.get("upnpip", upnpip);
			}
		}
		if (!(op_flags & OPT_P) && config.get("upnpport", value)) {
			upport = atoi(value.c_str());
		}
	}

	if (upnppdebug::Logger::getTheLog(logfilename) == 0) {
		cerr << "Can't initialize log" << endl;
		return 1;
	}
	upnppdebug::Logger::getTheLog("")->setLogLevel(upnppdebug::Logger::LogLevel(loglevel));

    Pidfile pidfile(pidfilename);

	// If started by root, do the pidfile + change uid thing
	uid_t runas(0);
	if (geteuid() == 0) {
		struct passwd *pass = getpwnam(upmpdcliuser.c_str());
		if (pass == 0) {
			LOGFAT("upmpdcli won't run as root and user " << upmpdcliuser << 
				   " does not exist " << endl);
			return 1;
		}
		runas = pass->pw_uid;

		pid_t pid;
		if ((pid = pidfile.open()) != 0) {
			LOGFAT("Can't open pidfile: " << pidfile.getreason() << 
				   ". Return (other pid?): " << pid << endl);
			return 1;
		}
		if (pidfile.write_pid() != 0) {
			LOGFAT("Can't write pidfile: " << pidfile.getreason() << endl);
			return 1;
		}
	}

	if ((op_flags & OPT_D)) {
		if (daemon(1, 0)) {
			LOGFAT("Daemon failed: errno " << errno << endl);
			return 1;
		}
	}

	if (geteuid() == 0) {
		// Need to rewrite pid, it may have changed with the daemon call
		pidfile.write_pid();
		if (!logfilename.empty() && logfilename.compare("stderr")) {
			if (chown(logfilename.c_str(), runas, -1) < 0) {
				LOGERR("chown("<<logfilename<<") : errno : " << errno << endl);
			}
		}
		if (setuid(runas) < 0) {
			LOGFAT("Can't set my uid to " << runas << " current: " << geteuid()
				   << endl);
			return 1;
		}
	}

	// Initialize MPD client object. Retry until it works or power fail.
	MPDCli *mpdclip = 0;
	int mpdretrysecs = 2;
	for (;;) {
		mpdclip = new MPDCli(mpdhost, mpdport, mpdpassword);
		if (mpdclip == 0) {
			LOGFAT("Can't allocate MPD client object" << endl);
			return 1;
		}
		if (!mpdclip->ok()) {
			LOGERR("MPD connection failed" << endl);
			delete mpdclip;
			mpdclip = 0;
			sleep(mpdretrysecs);
			mpdretrysecs = MIN(2*mpdretrysecs, 120);
		} else {
			break;
		}
	}

	// Initialize libupnpp, and check health
	LibUPnP *mylib = 0;
	string hwaddr;
	int libretrysecs = 10;
    for (;;) {
		// Libupnp init fails if we're started at boot and the network
		// is not ready yet. So retry this forever
		mylib = LibUPnP::getLibUPnP(true, &hwaddr, iface, upnpip, upport);
		if (mylib) {
			break;
		}
		sleep(libretrysecs);
		libretrysecs = MIN(2*libretrysecs, 120);
	}

	if (!mylib->ok()) {
		LOGFAT("Lib init failed: " <<
			   mylib->errAsString("main", mylib->getInitError()) << endl);
		return 1;
	}

	//string upnplogfilename("/tmp/upmpdcli_libupnp.log");
	//mylib->setLogFileName(upnplogfilename, LibUPnP::LogLevelDebug);

	// Create unique ID
	string UUID = LibUPnP::makeDevUUID(friendlyname, hwaddr);

	// Read our XML data to make it available from the virtual directory
	if (openhome) {
		xmlfilenames.insert(xmlfilenames.end(), ohxmlfilenames.begin(),
							ohxmlfilenames.end());
	}

	{
		string protofile = path_cat(datadir, "protocolinfo.txt");
		if (!read_protocolinfo(protofile, upmpdProtocolInfo)) {
			LOGFAT("Failed reading protocol info from " << protofile << endl);
			return 1;
		}
	}
			
	string reason;
	unordered_map<string, string> xmlfiles;
	for (unsigned int i = 0; i < xmlfilenames.size(); i++) {
		string filename = path_cat(datadir, xmlfilenames[i]);
		string data;
		if (!file_to_string(filename, data, &reason)) {
			LOGFAT("Failed reading " << filename << " : " << reason << endl);
			return 1;
		}
		if (i == 0) {
			// Special for description: set UUID and friendlyname
			data = regsub1("@UUID@", data, UUID);
			data = regsub1("@FRIENDLYNAME@", data, friendlyname);
			if (openhome) 
				data = regsub1("@OPENHOME@", data, ohDesc);
		}
		xmlfiles[xmlfilenames[i]] = data;
	}
	unsigned int options = UpMpd::upmpdNone;
	if (ownqueue)
		options |= UpMpd::upmpdOwnQueue;
	if (openhome)
		options |= UpMpd::upmpdDoOH;
	// Initialize the UPnP device object.
	UpMpd device(string("uuid:") + UUID, friendlyname, 
				 xmlfiles, mpdclip, options);

	// And forever generate state change events.
	LOGDEB("Entering event loop" << endl);
	device.eventloop();

	return 0;
}

/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
