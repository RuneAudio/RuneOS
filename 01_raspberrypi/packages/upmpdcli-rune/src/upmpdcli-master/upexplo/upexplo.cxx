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

////////////////////// libupnpp UPnP explorer test program

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <vector>
using namespace std;

#include "libupnpp/upnpplib.hxx"
#include "libupnpp/discovery.hxx"
#include "libupnpp/cdirectory.hxx"

void listServers(LibUPnP *lib, UPnPDeviceDirectory *superdir)
{
	cerr << "Servers:" << endl;
	vector<ContentDirectoryService> dirservices;
	if (!superdir->getDirServices(dirservices)) {
		cerr << "listDirServices failed" << endl;
		return;
	}
	for (vector<ContentDirectoryService>::iterator it = dirservices.begin();
		 it != dirservices.end(); it++) {
		cerr << it->getFriendlyName() << endl;
	}
}

void readdir(LibUPnP *lib, UPnPDeviceDirectory *superdir,
			 const string& friendlyName,
			 const string& cid)
{
	cerr << "readdir: [" << friendlyName << "] [" << cid << "]" << endl;
	ContentDirectoryService server;
	if (!superdir->getServer(friendlyName, server)) {
		cerr << "Server not found" << endl;
		return;
	}
	UPnPDirContent dirbuf;
	int code = server.readDir(cid, dirbuf);
	if (code) {
		cerr << LibUPnP::errAsString("readdir", code) << endl;
		return;
	}
	cerr << "Browse: got " << dirbuf.m_containers.size() <<
		" containers and " << dirbuf.m_items.size() << " items " << endl;
	for (unsigned int i = 0; i < dirbuf.m_containers.size(); i++) {
		cerr << dirbuf.m_containers[i].dump();
	}
	for (unsigned int i = 0; i < dirbuf.m_items.size(); i++) {
		cerr << dirbuf.m_items[i].dump();
	}
}

void getMetadata(LibUPnP *lib, UPnPDeviceDirectory *superdir,
				 const string& friendlyName,
				 const string& cid)
{
	cerr << "getMeta: [" << friendlyName << "] [" << cid << "]" << endl;
	ContentDirectoryService server;
	if (!superdir->getServer(friendlyName, server)) {
		cerr << "Server not found" << endl;
		return;
	}
	UPnPDirContent dirbuf;
	int code = server.getMetadata(cid, dirbuf);
	if (code) {
		cerr << LibUPnP::errAsString("readdir", code) << endl;
		return;
	}
	cerr << "getMeta: got " << dirbuf.m_containers.size() <<
		" containers and " << dirbuf.m_items.size() << " items " << endl;
	for (unsigned int i = 0; i < dirbuf.m_containers.size(); i++) {
		cerr << dirbuf.m_containers[i].dump();
	}
	for (unsigned int i = 0; i < dirbuf.m_items.size(); i++) {
		cerr << dirbuf.m_items[i].dump();
	}
}

void search(LibUPnP *lib, UPnPDeviceDirectory *superdir,
			const string& friendlyName, const string& ss)
{
	cerr << "search: [" << friendlyName << "] [" << ss << "]" << endl;
	ContentDirectoryService server;
	if (!superdir->getServer(friendlyName, server)) {
		cerr << "Server not found" << endl;
		return;
	}
	UPnPDirContent dirbuf;
	string cid("0");
	int code = server.search(cid, ss, dirbuf);
	if (code) {
		cerr << LibUPnP::errAsString("search", code) << endl;
		return;
	}
	cerr << "Search: got " << dirbuf.m_containers.size() <<
		" containers and " << dirbuf.m_items.size() << " items " << endl;
	for (unsigned int i = 0; i < dirbuf.m_containers.size(); i++) {
		cerr << dirbuf.m_containers[i].dump();
	}
	for (unsigned int i = 0; i < dirbuf.m_items.size(); i++) {
		cerr << dirbuf.m_items[i].dump();
	}
}

void getSearchCaps(LibUPnP *lib, UPnPDeviceDirectory *superdir,
				   const string& friendlyName)
{
	cerr << "getSearchCaps: [" << friendlyName << "]" << endl;
	ContentDirectoryService server;
	if (!superdir->getServer(friendlyName, server)) {
		cerr << "Server not found" << endl;
		return;
	}
	set<string> capa;
	int code = server.getSearchCapabilities(capa);
	if (code) {
		cerr << LibUPnP::errAsString("readdir", code) << endl;
		return;
	}
	if (capa.empty()) {
		cerr << "No search capabilities";
	} else {
		for (set<string>::const_iterator it = capa.begin();
			 it != capa.end(); it++) {
			cerr << "[" << *it << "]";
		}
	}
	cerr << endl;
}

static char *thisprog;
static char usage [] =
			" -l : list servers\n"
			" -r <server> <objid> list object id (root is '0')\n"
			" -s <server> <searchstring> search for string\n"
			" -m <server> <objid> : list object metadata\n"
			" -c <server> get search capabilities\n"
			"  \n\n"
			;
static void
Usage(void)
{
	fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
	exit(1);
}
static int	   op_flags;
#define OPT_MOINS 0x1
#define OPT_l	  0x2
#define OPT_r	  0x4
#define OPT_c	  0x8
#define OPT_s	  0x10
#define OPT_m	  0x20

int main(int argc, char *argv[])
{
	string fname;
	string arg;

	thisprog = argv[0];
	argc--; argv++;

	while (argc > 0 && **argv == '-') {
		(*argv)++;
		if (!(**argv))
			/* Cas du "adb - core" */
			Usage();
		while (**argv)
			switch (*(*argv)++) {
			case 'c':	op_flags |= OPT_c; if (argc < 2)  Usage();
				fname = *(++argv);argc--;
				goto b1;
			case 'l':	op_flags |= OPT_l; break;
			case 'm':	op_flags |= OPT_m; if (argc < 3)  Usage();
				fname = *(++argv);argc--;
				arg = *(++argv);argc--;
				goto b1;
			case 'r':	op_flags |= OPT_r; if (argc < 3)  Usage();
				fname = *(++argv);argc--;
				arg = *(++argv);argc--;
				goto b1;
			case 's':	op_flags |= OPT_s; if (argc < 3)  Usage();
				fname = *(++argv);argc--;
				arg = *(++argv);argc--;
				goto b1;
			default: Usage();	break;
			}
	b1: argc--; argv++;
	}

	if (argc != 0)
		Usage();

	LibUPnP *mylib = LibUPnP::getLibUPnP();
	if (!mylib) {
		cerr << "Can't get LibUPnP" << endl;
		return 1;
	}
	if (!mylib->ok()) {
		cerr << "Lib init failed: " <<
			mylib->errAsString("main", mylib->getInitError()) << endl;
		return 1;
	}
	mylib->setLogFileName("/tmp/libupnp.log");
	UPnPDeviceDirectory *superdir = UPnPDeviceDirectory::getTheDir();
	if (!superdir || !superdir->ok()) {
		cerr << "Discovery services startup failed" << endl;
		return 1;
	}

	if ((op_flags & OPT_l)) {
		while (true) {
			listServers(mylib, superdir);
			sleep(5);
		}
	} else if ((op_flags & OPT_m)) {
		getMetadata(mylib, superdir, fname, arg);
	} else if ((op_flags & OPT_r)) {
		readdir(mylib, superdir, fname, arg);
	} else if ((op_flags & OPT_s)) {
		search(mylib, superdir, fname, arg);
	} else if ((op_flags & OPT_c)) {
		getSearchCaps(mylib, superdir, fname);
	} else {
		Usage();
	}
	superdir->terminate();
	return 0;
}

/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
