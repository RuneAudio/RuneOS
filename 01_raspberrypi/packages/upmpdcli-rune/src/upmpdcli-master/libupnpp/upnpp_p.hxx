/* Copyright (C) 2013 J.F.Dockes
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _UPNPP_H_X_INCLUDED_
#define _UPNPP_H_X_INCLUDED_
#include "config.h"

/* Private shared defs for the library. Clients need not and should
   not include this */

#include <string>

// Concatenate paths. Caller should make sure it makes sense.
extern std::string caturl(const std::string& s1, const std::string& s2);
// Return the scheme://host:port[/] part of input, or input if it is weird
extern std::string baseurl(const std::string& url);
extern void trimstring(std::string &s, const char *ws = " \t\n");
extern std::string path_getfather(const std::string &s);
extern std::string path_getsimple(const std::string &s);
template <class T> bool csvToStrings(const std::string& s, T &tokens);

#if defined(HAVE_UPNPSETLOGLEVEL)
#include <upnp/upnpdebug.h>
#define PLOGINF(...) UpnpPrintf(UPNP_INFO, API, __FILE__, __LINE__, __VA_ARGS__)
#define PLOGDEB(...) UpnpPrintf(UPNP_INFO,API, __FILE__, __LINE__, __VA_ARGS__)
#else
#define PLOGINF(...)
#define PLOGDEB(...)
#endif

#endif /* _UPNPP_H_X_INCLUDED_ */
