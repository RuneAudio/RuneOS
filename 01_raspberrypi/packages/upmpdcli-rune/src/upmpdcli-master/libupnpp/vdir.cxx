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

#include <sys/types.h>
#include <memory.h>
#include <iostream>

#include <upnp/upnp.h>

#include "upnpp_p.hxx"
#include "vdir.hxx"
#include "log.hxx"

using namespace std;

static VirtualDir *theDir;

struct Handle {
	Handle(VirtualDir::FileEnt *e)
		: entry(e), offset(0)
		{}
	VirtualDir::FileEnt *entry;	
	size_t offset;
};

static int vdclose(UpnpWebFileHandle fileHnd)
{
	Handle *h = (Handle*)fileHnd;
	delete h;
	return 0;
}

static VirtualDir::FileEnt *vdgetentry(const char *pathname)
{
	//LOGDEB("vdgetentry: [" << pathname << "]" << endl);
	VirtualDir *thedir = VirtualDir::getVirtualDir();
	if (thedir == 0)
		return 0;

	string dir = path_getfather(pathname);
	string fn = path_getsimple(pathname);

	return theDir->getFile(dir, fn);
}

static int vdgetinfo(const char *fn, struct File_Info* info )
{
	//LOGDEB("vdgetinfo: [" << fn << "] off_t " << sizeof(off_t) <<
	// " time_t " << sizeof(time_t) << endl);
	VirtualDir::FileEnt *entry = vdgetentry(fn);
	if (entry == 0) {
		LOGERR("vdgetinfo: no entry for " << fn << endl);
		return -1;
	}

	info->file_length = entry->content.size();
	info->last_modified = entry->mtime;
	info->is_directory = 0;
	info->is_readable = 1;
	info->content_type = ixmlCloneDOMString(entry->mimetype.c_str());

	return 0;
}

static UpnpWebFileHandle vdopen(const char* fn, enum UpnpOpenFileMode Mode)
{
	//LOGDEB("vdopen: " << fn << endl);
	VirtualDir::FileEnt *entry = vdgetentry(fn);
	if (entry == 0) {
		LOGERR("vdopen: no entry for " << fn << endl);
		return NULL;
	}
	return new Handle(entry);
}

static int vdread(UpnpWebFileHandle fileHnd, char* buf, size_t buflen)
{
	// LOGDEB("vdread: " << endl);
	if (buflen == 0)
		return 0;
	Handle *h = (Handle *)fileHnd;
	if (h->offset >= h->entry->content.size())
		return 0;
	size_t toread = buflen > h->entry->content.size() - h->offset ?
		h->entry->content.size() - h->offset : buflen;
	memcpy(buf, h->entry->content.c_str() + h->offset, toread);
	h->offset += toread;
	return toread;
}

static int vdseek(UpnpWebFileHandle fileHnd, off_t offset, int origin)
{
	// LOGDEB("vdseek: " << endl);
	Handle *h = (Handle *)fileHnd;
	if (origin == 0)
		h->offset = offset;
	else if (origin == 1)
		h->offset += offset;
	else if (origin == 2)
		h->offset = h->entry->content.size() + offset;
	else 
		return -1;
	return offset;
}

static int vdwrite(UpnpWebFileHandle fileHnd, char* buf, size_t buflen)
{
	LOGERR("vdwrite" << endl);
	return -1;
}

static struct UpnpVirtualDirCallbacks myvdcalls = {
	vdgetinfo, vdopen, vdread, vdwrite, vdseek, vdclose
};

VirtualDir *VirtualDir::getVirtualDir()
{
	if (theDir == 0) {
		theDir = new VirtualDir();
		if (UpnpSetVirtualDirCallbacks(&myvdcalls) != UPNP_E_SUCCESS) {
			LOGERR("SetVirtualDirCallbacks failed" << endl);
			delete theDir;
			theDir = 0;
			return 0;
		}
	} 
	return theDir;
}

bool VirtualDir::addFile(const string& _path, const string& name, 
						 const string& content, const string& mimetype)
{
	string path(_path);
	if (path.empty() || path[path.size()-1] != '/') {
		path += '/';
	}
	if (m_dirs.find(path) == m_dirs.end()) {
		m_dirs[path] = unordered_map<string, VirtualDir::FileEnt>();
		UpnpAddVirtualDir(path.c_str());
	}
	VirtualDir::FileEnt entry;
	entry.mtime = time(0);
	entry.mimetype = mimetype;
	entry.content = content;
	m_dirs[path][name] = entry;
	// LOGDEB("VirtualDir::addFile: added entry for dir " << 
	// path << " name " << name << endl);
	return true;
}

VirtualDir::FileEnt *VirtualDir::getFile(const string& _path, 
										 const string& name)
{
	string path(_path);
	if (path.empty() || path[path.size()-1] != '/') {
		path += '/';
	}

	// LOGDEB("VirtualDir::getFile: path " << path << " name " << name << endl);

	unordered_map<string, unordered_map<string,VirtualDir::FileEnt> >::iterator dir = 
		m_dirs.find(path);
	if (dir == m_dirs.end()) {
		LOGERR("VirtualDir::getFile: no dir: " << path << endl);
		return 0;
	}
	unordered_map<string, FileEnt>::iterator f = dir->second.find(name);
	if (f == dir->second.end()) {
		LOGERR("VirtualDir::getFile: no file: " << path << endl);
		return 0;
	}

	return &(f->second);
}

/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
