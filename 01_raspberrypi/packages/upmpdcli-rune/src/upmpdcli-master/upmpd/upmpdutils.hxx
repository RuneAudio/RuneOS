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
#ifndef _UPMPDUTILS_H_X_INCLUDED_
#define _UPMPDUTILS_H_X_INCLUDED_

#include <string>
#include <unordered_map>

/** Read protocol info file. This contains the connection manager protocol info data
 *
 * We strip white-space from beginning/ends of lines, and allow
 * #-started comments (on a line alone only, comments after data not allowed).
 */
bool read_protocolinfo(const string& fn, string& out);

/**
 * Read file into string.
 * @return true for ok, false else
 */
extern bool file_to_string(const std::string &filename, std::string &data, 
                           std::string *reason = 0);

extern void path_catslash(std::string &s);
extern std::string path_cat(const std::string &s1, const std::string &s2);
extern void trimstring(string &s, const char *ws = " \t");
extern string path_tildexpand(const string &s);
extern void stringToTokens(const string &s, vector<string> &tokens, 
			   const string &delims = " \t", bool skipinit=true);


// Convert between db value to percent values (Get/Set Volume and VolumeDb)
extern int percentodbvalue(int value);
extern int dbvaluetopercent(int dbvalue);

extern bool sleepms(int ms);

// Format milliseconds into H+:MM:SS
extern std::string upnpduration(int ms);
// hms to seconds
extern int upnpdurationtos(const string& dur);

// Return mapvalue or null strings, for maps where absent entry and
// null data are equivalent
extern const std::string& mapget(
    const std::unordered_map<std::string, std::string>& im, 
    const std::string& k);

// Format a didl fragment from MPD status data
class UpSong;
extern std::string didlmake(const UpSong& song);

// Replace the first occurrence of regexp. cxx11 regex does not work
// that well yet...
extern std::string regsub1(const std::string& sexp, const std::string& input, 
                           const std::string& repl);

// Return map with "newer" elements which differ from "old".  Old may
// have fewer elts than "newer" (e.g. may be empty), we use the
// "newer" entries for diffing
extern std::unordered_map<std::string, std::string> 
diffmaps(const std::unordered_map<std::string, std::string>& old,
         const std::unordered_map<std::string, std::string>& newer);

/// Lock/pid file class. From Recoll
class Pidfile {
public:
    Pidfile(const std::string& path)	: m_path(path), m_fd(-1) {}
    ~Pidfile();
    /// Open/create the pid file.
    /// @return 0 if ok, > 0 for pid of existing process, -1 for other error.
    pid_t open();
    /// Write pid into the pid file
    /// @return 0 ok, -1 error
    int write_pid();
    /// Close the pid file (unlocks)
    int close();
    /// Delete the pid file
    int remove();
    const std::string& getreason() {return m_reason;}
private:
    std::string m_path;
    int    m_fd;
    std::string m_reason;
    pid_t read_pid();
    int flopen();
};

#endif /* _UPMPDUTILS_H_X_INCLUDED_ */
