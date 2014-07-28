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

#ifndef _VDIR_H_X_INCLUDED_
#define _VDIR_H_X_INCLUDED_

/** An easy libupnp virtual directory handler, based on stl 
    maps and strings.

    As libupnp only lets us defines the api calls (open/read/etc.),
    without any data cookie, this has to be a global singleton object.
 */

#include <time.h>

#include <string>
#include <unordered_map>

class VirtualDir {
public:
	static VirtualDir* getVirtualDir();
	bool addFile(const std::string& path, const std::string& name, 
				 const std::string& content, const std::string& mimetype);
	class FileEnt {
	public:
		time_t mtime;
		std::string mimetype;
		std::string content;
	};
	FileEnt *getFile(const std::string& path, const std::string& name);

private:
	VirtualDir() {}

	std::unordered_map<std::string, std::unordered_map<std::string, FileEnt> > 
	m_dirs;
};


#endif /* _VDIR_H_X_INCLUDED_ */

/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
