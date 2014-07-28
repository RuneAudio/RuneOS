/* Copyright (C) 2003 J.F.Dockes 
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
#ifndef TEST_CONFTREE

#include <unistd.h> // for access(2)
#include <ctype.h>
#include <fnmatch.h>
#include <sys/stat.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cstring>

using namespace std;

#include "conftree.hxx"
#include "upmpdutils.hxx"

#undef DEBUG
#ifdef DEBUG
#define LOGDEB(X) fprintf X
#else
#define LOGDEB(X)
#endif

#ifndef MIN 
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#endif

#define LL 1024

void ConfSimple::parseinput(istream &input)
{
    string submapkey;
    char cline[LL];
    bool appending = false;
    string line;
    bool eof = false;

    for (;;) {
        cline[0] = 0;
	input.getline(cline, LL-1);
	LOGDEB((stderr, "Parse:line: [%s] status %d\n", cline, int(status)));
	if (!input.good()) {
	    if (input.bad()) {
                LOGDEB((stderr, "Parse: input.bad()\n"));
		status = STATUS_ERROR;
		return;
	    }
            LOGDEB((stderr, "Parse: eof\n"));
	    // Must be eof ? But maybe we have a partial line which
	    // must be processed. This happens if the last line before
	    // eof ends with a backslash, or there is no final \n
            eof = true;
	}

        {
            int ll = strlen(cline);
            while (ll > 0 && (cline[ll-1] == '\n' || cline[ll-1] == '\r')) {
                cline[ll-1] = 0;
                ll--;
            }
        }

	if (appending)
	    line += cline;
	else
	    line = cline;

	// Note that we trim whitespace before checking for backslash-eol
	// This avoids invisible whitespace problems.
	trimstring(line);
	if (line.empty() || line.at(0) == '#') {
            if (eof)
                break;
	    m_order.push_back(ConfLine(ConfLine::CFL_COMMENT, line));
	    continue;
	}
	if (line[line.length() - 1] == '\\') {
	    line.erase(line.length() - 1);
	    appending = true;
	    continue;
	}
	appending = false;

	if (line[0] == '[') {
	    trimstring(line, "[]");
	    if (dotildexpand)
		submapkey = path_tildexpand(line);
	    else 
		submapkey = line;
	    // No need for adding sk to order, will be done with first
	    // variable insert. Also means that empty section are
	    // expandable (won't be output when rewriting)
	    // Another option would be to add the subsec to m_order here
	    // and not do it inside i_set() if init is true
	    continue;
	}

	// Look for first equal sign
	string::size_type eqpos = line.find("=");
	if (eqpos == string::npos) {
	    m_order.push_back(ConfLine(ConfLine::CFL_COMMENT, line));
	    continue;
	}

	// Compute name and value, trim white space
	string nm, val;
	nm = line.substr(0, eqpos);
	trimstring(nm);
	val = line.substr(eqpos+1, string::npos);
	trimstring(val);
	
	if (nm.length() == 0) {
	    m_order.push_back(ConfLine(ConfLine::CFL_COMMENT, line));
	    continue;
	}
	i_set(nm, val, submapkey, true);
        if (eof)
            break;
    }
}


ConfSimple::ConfSimple(int readonly, bool tildexp)
    : dotildexpand(tildexp), m_fmtime(0), m_holdWrites(false)
{
    status = readonly ? STATUS_RO : STATUS_RW;
}

void ConfSimple::reparse(const string& d)
{
    clear();
    stringstream input(d, ios::in);
    parseinput(input);
}

ConfSimple::ConfSimple(const string& d, int readonly, bool tildexp)
    : dotildexpand(tildexp), m_fmtime(0), m_holdWrites(false)
{
    status = readonly ? STATUS_RO : STATUS_RW;

    stringstream input(d, ios::in);
    parseinput(input);
}

ConfSimple::ConfSimple(const char *fname, int readonly, bool tildexp)
    : dotildexpand(tildexp), m_filename(fname), m_fmtime(0), m_holdWrites(false)
{
    status = readonly ? STATUS_RO : STATUS_RW;

    ifstream input;
    if (readonly) {
	input.open(fname, ios::in);
    } else {
	ios::openmode mode = ios::in|ios::out;
	// It seems that there is no separate 'create if not exists' 
	// open flag. Have to truncate to create, but dont want to do 
	// this to an existing file !
	if (access(fname, 0) < 0) {
	    mode |= ios::trunc;
	}
	input.open(fname, mode);
	if (input.is_open()) {
	    status = STATUS_RW;
	} else {
	    input.clear();
	    input.open(fname, ios::in);
	    if (input.is_open()) {
		status = STATUS_RO;
	    }
	}
    }

    if (!input.is_open()) {
	status = STATUS_ERROR;
	return;
    }	    

    parseinput(input);
    i_changed(true);
}

ConfSimple::StatusCode ConfSimple::getStatus() const
{
    switch (status) {
    case STATUS_RO: return STATUS_RO;
    case STATUS_RW: return STATUS_RW;
    default: return STATUS_ERROR;
    }
}

bool ConfSimple::sourceChanged() const
{
    if (!m_filename.empty()) {
	struct stat st;
	if (stat(m_filename.c_str(), &st) == 0) {
	    if (m_fmtime != st.st_mtime) {
		return true;
	    }
	}
    }
    return false;
}

bool ConfSimple::i_changed(bool upd)
{
    if (!m_filename.empty()) {
	struct stat st;
	if (stat(m_filename.c_str(), &st) == 0) {
	    if (m_fmtime != st.st_mtime) {
		if (upd)
		    m_fmtime = st.st_mtime;
		return true;
	    }
	}
    }
    return false;
}

int ConfSimple::get(const string &nm, string &value, const string &sk) const
{
    if (!ok())
	return 0;

    // Find submap
    map<string, map<string, string> >::const_iterator ss;
    if ((ss = m_submaps.find(sk)) == m_submaps.end()) 
	return 0;

    // Find named value
    map<string, string>::const_iterator s;
    if ((s = ss->second.find(nm)) == ss->second.end()) 
	return 0;
    value = s->second;
    return 1;
}

// Appropriately output a subkey (nm=="") or variable line.
// Splits long lines
static ConfSimple::WalkerCode varprinter(void *f, const string &nm, 
					 const string &value)
{
    ostream *output = (ostream *)f;
    if (nm.empty()) {
	*output << "\n[" << value << "]\n";
    } else {
	string value1;
	if (value.length() < 60) {
	    value1 = value;
	} else {
	    string::size_type pos = 0;
	    while (pos < value.length()) {
		string::size_type len = MIN(60, value.length() - pos);
		value1 += value.substr(pos, len);
		pos += len;
		if (pos < value.length())
		    value1 += "\\\n";
	    }
	}
	*output << nm << " = " << value1 << "\n";
    }
    return ConfSimple::WALK_CONTINUE;
}

// Set variable and rewrite data
int ConfSimple::set(const std::string &nm, const std::string &value, 
		    const string &sk)
{
    if (status  != STATUS_RW)
	return 0;
    LOGDEB((stderr, "ConfSimple::set [%s]:[%s] -> [%s]\n", sk.c_str(),
	     nm.c_str(), value.c_str()));
    if (!i_set(nm, value, sk))
	return 0;
    return write();
}

// Internal set variable: no rw checking or file rewriting. If init is
// set, we're doing initial parsing, else we are changing a parsed
// tree (changes the way we update the order data)
int ConfSimple::i_set(const std::string &nm, const std::string &value, 
		      const string &sk, bool init)
{
    LOGDEB((stderr, "ConfSimple::i_set: nm[%s] val[%s] key[%s], init %d\n",
	    nm.c_str(), value.c_str(), sk.c_str(), init));
    // Values must not have embedded newlines
    if (value.find_first_of("\n\r") != string::npos) {
	LOGDEB((stderr, "ConfSimple::i_set: LF in value\n"));
	return 0;
    }
    bool existing = false;
    map<string, map<string, string> >::iterator ss;
    // Test if submap already exists, else create it, and insert variable:
    if ((ss = m_submaps.find(sk)) == m_submaps.end()) {
	LOGDEB((stderr, "ConfSimple::i_set: new submap\n"));
	map<string, string> submap;
	submap[nm] = value;
	m_submaps[sk] = submap;

	// Maybe add sk entry to m_order data:
	if (!sk.empty()) {
	    ConfLine nl(ConfLine::CFL_SK, sk);
	    // Append SK entry only if it's not already there (erase
	    // does not remove entries from the order data, adn it may
	    // be being recreated after deletion)
	    if (find(m_order.begin(), m_order.end(), nl) == m_order.end()) {
		m_order.push_back(nl);
	    }
	}
    } else {
	// Insert or update variable in existing map.
	map<string, string>::iterator it;
	it = ss->second.find(nm);
	if (it == ss->second.end()) {
	    ss->second.insert(pair<string,string>(nm, value));
	} else {
	    it->second = value;
	    existing = true;
	}
    }

    // If the variable already existed, no need to change the m_order data
    if (existing) {
	LOGDEB((stderr, "ConfSimple::i_set: existing var: no order update\n"));
	return 1;
    }

    // Add the new variable at the end of its submap in the order data.

    if (init) {
	// During the initial construction, just append:
	LOGDEB((stderr, "ConfSimple::i_set: init true: append\n"));
	m_order.push_back(ConfLine(ConfLine::CFL_VAR, nm));
	return 1;
    } 

    // Look for the start and end of the subkey zone. Start is either
    // at begin() for a null subkey, or just behind the subkey
    // entry. End is either the next subkey entry, or the end of
    // list. We insert the new entry just before end.
    vector<ConfLine>::iterator start, fin;
    if (sk.empty()) {
	start = m_order.begin();
	LOGDEB((stderr,"ConfSimple::i_set: null sk, start at top of order\n"));
    } else {
	start = find(m_order.begin(), m_order.end(), 
		     ConfLine(ConfLine::CFL_SK, sk));
	if (start == m_order.end()) {
	    // This is not logically possible. The subkey must
	    // exist. We're doomed
	    std::cerr << "Logical failure during configuration variable " 
		"insertion" << endl;
	    abort();
	}
    }

    fin = m_order.end();
    if (start != m_order.end()) {
	// The null subkey has no entry (maybe it should)
	if (!sk.empty())
	    start++;
	for (vector<ConfLine>::iterator it = start; it != m_order.end(); it++) {
	    if (it->m_kind == ConfLine::CFL_SK) {
		fin = it;
		break;
	    }
	}
    }

    // It may happen that the order entry already exists because erase doesnt
    // update m_order
    if (find(start, fin, ConfLine(ConfLine::CFL_VAR, nm)) == fin) {
	m_order.insert(fin, ConfLine(ConfLine::CFL_VAR, nm));
    }
    return 1;
}

int ConfSimple::erase(const string &nm, const string &sk)
{
    if (status  != STATUS_RW)
	return 0;

    map<string, map<string, string> >::iterator ss;
    if ((ss = m_submaps.find(sk)) == m_submaps.end()) {
	return 0;
    }
    
    ss->second.erase(nm);
    if (ss->second.empty()) {
	m_submaps.erase(ss);
    }
    return write();
}

int ConfSimple::eraseKey(const string &sk)
{
    vector<string> nms = getNames(sk);
    for (vector<string>::iterator it = nms.begin(); it != nms.end(); it++) {
	erase(*it, sk);
    }
    return write();
}

// Walk the tree, calling user function at each node
ConfSimple::WalkerCode 
ConfSimple::sortwalk(WalkerCode (*walker)(void *,const string&,const string&),
		     void *clidata) const
{
    if (!ok())
	return WALK_STOP;
    // For all submaps:
    for (map<string, map<string, string> >::const_iterator sit = 
	     m_submaps.begin();
	 sit != m_submaps.end(); sit++) {

	// Possibly emit submap name:
	if (!sit->first.empty() && walker(clidata, string(), sit->first.c_str())
	    == WALK_STOP)
	    return WALK_STOP;

	// Walk submap
	const map<string, string> &sm = sit->second;
	for (map<string, string>::const_iterator it = sm.begin();it != sm.end();
	     it++) {
	    if (walker(clidata, it->first, it->second) == WALK_STOP)
		return WALK_STOP;
	}
    }
    return WALK_CONTINUE;
}

// Write to default output. This currently only does something if output is
// a file
bool ConfSimple::write()
{
    if (!ok())
	return false;
    if (m_holdWrites)
	return true;
    if (m_filename.length()) {
	ofstream output(m_filename.c_str(), ios::out|ios::trunc);
	if (!output.is_open())
	    return 0;
	return write(output);
    } else {
	// No backing store, no writing. Maybe one day we'll need it with
        // some kind of output string. This can't be the original string which
        // is currently readonly.
	//ostringstream output(m_ostring, ios::out | ios::trunc);
	return 1;
    }
}

// Write out the tree in configuration file format:
// This does not check holdWrites, this is done by write(void), which
// lets ie: showall work even when holdWrites is set
bool ConfSimple::write(ostream& out) const
{
    if (!ok())
	return false;
    string sk;
    for (vector<ConfLine>::const_iterator it = m_order.begin(); 
	 it != m_order.end(); it++) {
	switch(it->m_kind) {
	case ConfLine::CFL_COMMENT: 
	    out << it->m_data << endl; 
	    if (!out.good()) 
		return false;
	    break;
	case ConfLine::CFL_SK:      
	    sk = it->m_data;
	    LOGDEB((stderr, "ConfSimple::write: SK [%s]\n", sk.c_str()));
	    // Check that the submap still exists, and only output it if it
	    // does
	    if (m_submaps.find(sk) != m_submaps.end()) {
		out << "[" << it->m_data << "]" << endl;
		if (!out.good()) 
		    return false;
	    }
	    break;
	case ConfLine::CFL_VAR:
	    string nm = it->m_data;
	    LOGDEB((stderr, "ConfSimple::write: VAR [%s], sk [%s]\n",
		    nm.c_str(), sk.c_str()));
	    // As erase() doesnt update m_order we can find unexisting
	    // variables, and must not output anything for them. Have
	    // to use a ConfSimple::get() to check here, because
	    // ConfTree's could retrieve from an ancestor even if the
	    // local var is gone.
	    string value;
	    if (ConfSimple::get(nm, value, sk)) {
		    varprinter(&out, nm, value);
		    if (!out.good()) 
			return false;
		    break;
	    }
	    LOGDEB((stderr, "ConfSimple::write: no value: nm[%s] sk[%s]\n",
			nm.c_str(), sk.c_str()));
	    break;
	}
    }
    return true;
}

void ConfSimple::showall() const
{
    if (!ok())
	return;
    write(std::cout);
}

vector<string> ConfSimple::getNames(const string &sk, const char *pattern) const
{
    vector<string> mylist;
    if (!ok())
	return mylist;
    map<string, map<string, string> >::const_iterator ss;
    if ((ss = m_submaps.find(sk)) == m_submaps.end()) {
	return mylist;
    }
    mylist.reserve(ss->second.size());
    map<string, string>::const_iterator it;
    for (it = ss->second.begin(); it != ss->second.end(); it++) {
        if (pattern && 0 != fnmatch(pattern, it->first.c_str(), 0))
            continue;
	mylist.push_back(it->first);
    }
    return mylist;
}

vector<string> ConfSimple::getSubKeys() const
{
    vector<string> mylist;
    if (!ok())
	return mylist;
    mylist.reserve(m_submaps.size());
    map<string, map<string, string> >::const_iterator ss;
    for (ss = m_submaps.begin(); ss != m_submaps.end(); ss++) {
	mylist.push_back(ss->first);
    }
    return mylist;
}

bool ConfSimple::hasNameAnywhere(const string& nm) const
{
    vector<string>keys = getSubKeys();
    for (vector<string>::const_iterator it = keys.begin(); 
         it != keys.end(); it++) {
        string val;
        if (get(nm, val, *it))
            return true;
    }
    return false;
}

// //////////////////////////////////////////////////////////////////////////
// ConfTree Methods: conftree interpret keys like a hierarchical file tree
// //////////////////////////////////////////////////////////////////////////

int ConfTree::get(const std::string &name, string &value, const string &sk)
    const
{
    if (sk.empty() || sk[0] != '/') {
	//	LOGDEB((stderr, "ConfTree::get: looking in global space\n"));
	return ConfSimple::get(name, value, sk);
    }

    // Get writable copy of subkey path
    string msk = sk;

    // Handle the case where the config file path has an ending / and not
    // the input sk
    path_catslash(msk);

    // Look in subkey and up its parents until root ('')
    for (;;) {
	//	LOGDEB((stderr,"ConfTree::get: looking for '%s' in '%s'\n",
	//		name.c_str(), msk.c_str()));
	if (ConfSimple::get(name, value, msk))
	    return 1;
	string::size_type pos = msk.rfind("/");
	if (pos != string::npos) {
	    msk.replace(pos, string::npos, string());
	} else
	    break;
    }
    return 0;
}

#else // TEST_CONFTREE

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <vector>

#include "conftree.h"
#include "smallut.h"
#include "readfile.h"

using namespace std;

static char *thisprog;

bool complex_updates(const string& fn)
{
    int fd;
    if ((fd = open(fn.c_str(), O_RDWR|O_TRUNC|O_CREAT, 0666)) < 0) {
	perror("open/create");
	return false;
    }
    close(fd);

    ConfTree conf(fn.c_str());
    if (!conf.ok()) {
	cerr << "Config init failed" << endl;
	return false;
    }

    conf.set("nm-1", "val-1", "");
    conf.set("nm-2", "val-2", "");

    conf.set("nm-1", "val1-1", "/dir1");
    conf.set("nm-2", "val1-2", "/dir1");

    conf.set("nm-1", "val2-1", "/dir2");
    conf.set("nm-2", "val2-2", "/dir2");

    conf.set("nm-1", "val11-1", "/dir1/dir1");
    conf.set("nm-2", "val11-2", "/dir1/dir1");

    conf.eraseKey("/dir2");
    conf.set("nm-1", "val2-1", "/dir2");
    conf.set("nm-2", "val2-2", "/dir2");

    conf.erase("nm-1", "");
    conf.erase("nm-2", "");
    conf.eraseKey("/dir1");
    conf.eraseKey("/dir2");
    conf.eraseKey("/dir1/dir1");

    conf.set("nm-1", "val1-1", "/dir1");
    conf.set("nm-2", "val1-2", "/dir1");
    conf.set("nm-1", "val-1", "");

    conf.set("nm-1", "val2-1", "/dir2");
    conf.set("nm-2", "val2-2", "/dir2");

    conf.set("nm-1", "val11-1", "/dir1/dir1");
    conf.set("nm-2", "val11-2", "/dir1/dir1");

    conf.erase("nm-1", "/dir2");
    conf.erase("nm-2", "/dir2");
    conf.erase("nm-1", "/dir1/dir1");
    conf.erase("nm-2", "/dir1/dir1");

    string data;
    file_to_string(fn, data, 0);
    const string ref =
	"nm-1 = val-1\n"
	"[/dir1]\n"
	"nm-1 = val1-1\n"
	"nm-2 = val1-2\n"
	;
    if (data.compare(ref)) {
	cerr << "Final file:" << endl << data << endl << "Differs from ref:" <<
	    endl << ref << endl;
	return false;
    } else {
	cout << "Updates test Ok" << endl;
    }
    return true;
}

ConfSimple::WalkerCode mywalker(void *, const string &nm, const string &value)
{
    if (nm.empty())
	printf("\n[%s]\n", value.c_str());
    else 
	printf("'%s' -> '%s'\n", nm.c_str(), value.c_str());
    return ConfSimple::WALK_CONTINUE;
}

const char *longvalue = 
"Donnees012345678901234567890123456789012345678901234567890123456789AA"
"0123456789012345678901234567890123456789012345678901234567890123456789FIN"
    ;

void memtest(ConfSimple &c) 
{
    cout << "Initial:" << endl;
    c.showall();
    if (c.set("nom", "avec nl \n 2eme ligne", "")) {
	fprintf(stderr, "set with embedded nl succeeded !\n");
	exit(1);
    }
    if (!c.set(string("parm1"), string("1"), string("subkey1"))) {
	fprintf(stderr, "Set error");
	exit(1);
    }
    if (!c.set("sparm", "Parametre \"string\" bla", "s2")) {
	fprintf(stderr, "Set error");
	exit(1);
    }
    if (!c.set("long", longvalue, "")) {
	fprintf(stderr, "Set error");
	exit(1);
    }

    cout << "Final:" << endl;
    c.showall();
}

bool readwrite(ConfNull *conf)
{
    if (conf->ok()) {
	// It's ok for the file to not exist here
	string value;
		
	if (conf->get("mypid", value)) {
	    cout << "Value for mypid is [" << value << "]" << endl;
	} else {
	    cout << "mypid not set" << endl;
	}
		
	if (conf->get("unstring", value)) {
	    cout << "Value for unstring is ["<< value << "]" << endl;
	} else {
	    cout << "unstring not set" << endl;
	}
    }
    char spid[100];
    sprintf(spid, "%d", getpid());
    if (!conf->set("mypid", spid)) {
	cerr << "Set mypid failed" << endl;
    }

    ostringstream ost;
    ost << "mypid" << getpid();
    if (!conf->set(ost.str(), spid, "")) {
	cerr << "Set mypid failed (2)" << endl;
    }
    if (!conf->set("unstring", "Une jolie phrase pour essayer")) {
	cerr << "Set unstring failed" << endl;
    }
    return true;
}

bool query(ConfNull *conf, const string& nm, const string& sub)
{
    if (!conf->ok()) {
	cerr <<  "Error opening or parsing file\n" << endl;
	return false;
    }
    string value;
    if (!conf->get(nm, value, sub)) {
	cerr << "name [" << nm << "] not found in [" << sub << "]" << endl;
	return false;
    }
    cout << "[" << sub << "] " << nm << " " << value << endl;
    return true;
}

bool erase(ConfNull *conf, const string& nm, const string& sub)
{
    if (!conf->ok()) {
	cerr <<  "Error opening or parsing file\n" << endl;
	return false;
    }

    if (!conf->erase(nm, sub)) {
	cerr <<  "delete name [" << nm <<  "] in ["<< sub << "] failed" << endl;
	return false;
    }
    return true;
}

bool eraseKey(ConfNull *conf, const string& sub)
{
    if (!conf->ok()) {
	cerr <<  "Error opening or parsing file\n" << endl;
	return false;
    }

    if (!conf->eraseKey(sub)) {
	cerr <<  "delete key [" << sub <<  "] failed" << endl;
	return false;
    }
    return true;
}

bool setvar(ConfNull *conf, const string& nm, const string& value, 
	    const string& sub)
{
    if (!conf->ok()) {
	cerr <<  "Error opening or parsing file\n" << endl;
	return false;
    }
    if (!conf->set(nm, value, sub)) {
	cerr <<  "Set error\n" << endl;
	return false;
    }
    return true;
}

static char usage [] =
    "testconftree [opts] filename\n"
    "[-w]  : read/write test.\n"
    "[-s]  : string parsing test. Filename must hold parm 'strings'\n"
    "-a nm value sect : add/set nm,value in 'sect' which can be ''\n"
    "-q nm sect : subsection test: look for nm in 'sect' which can be ''\n"
    "-d nm sect : delete nm in 'sect' which can be ''\n"
    "-E sect : erase key (and all its names)\n"
    "-S : string io test. No filename in this case\n"
    "-V : volatile config test. No filename in this case\n"
    "-U : complex update test. Will erase the named file parameter\n"	 
    ;

void Usage() {
    fprintf(stderr, "%s:%s\n", thisprog, usage);
    exit(1);
}
static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_w	  0x2 
#define OPT_q     0x4
#define OPT_s     0x8
#define OPT_S     0x10
#define OPT_d     0x20
#define OPT_V     0x40
#define OPT_a     0x80
#define OPT_k     0x100
#define OPT_E     0x200
#define OPT_U      0x400

int main(int argc, char **argv)
{
    const char *nm = 0;
    const char *sub = 0;
    const char *value = 0;

    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'a':
		op_flags |= OPT_a;
		if (argc < 4)  
		    Usage();
		nm = *(++argv);argc--;
		value = *(++argv);argc--;
		sub = *(++argv);argc--;		  
		goto b1;
	    case 'd':
		op_flags |= OPT_d;
		if (argc < 3)  
		    Usage();
		nm = *(++argv);argc--;
		sub = *(++argv);argc--;		  
		goto b1;
	    case 'E':   
		op_flags |= OPT_E; 
		if (argc < 2)
		    Usage();
		sub = *(++argv);argc--;		  
		goto b1;
	    case 'k':   op_flags |= OPT_k; break;
	    case 'q':
		op_flags |= OPT_q;
		if (argc < 3)  
		    Usage();
		nm = *(++argv);argc--;
		sub = *(++argv);argc--;		  
		goto b1;
	    case 's':   op_flags |= OPT_s; break;
	    case 'S':   op_flags |= OPT_S; break;
	    case 'V':   op_flags |= OPT_V; break;
	    case 'U':   op_flags |= OPT_U; break;
	    case 'w':	op_flags |= OPT_w; break;

	    default: Usage();	break;
	    }
    b1: argc--; argv++;
    }

    if ((op_flags & OPT_S)) {
	// String storage test
	if (argc != 0)
	    Usage();
	string s;
	ConfSimple c(s);
	memtest(c);
	exit(0);
    } else if  ((op_flags & OPT_V)) {
	// No storage test
	if (argc != 0)
	    Usage();
	ConfSimple c;
	memtest(c);
	exit(0);
    } 

    // Other tests use file(s) as backing store
    if (argc < 1)
	Usage();

    if (op_flags & OPT_U) {
	exit(!complex_updates(argv[0]));
    }
    vector<string> flist;
    while (argc--) {
	flist.push_back(*argv++);
    }
    bool ro = !(op_flags & (OPT_w|OPT_a|OPT_d|OPT_E));
    ConfNull *conf = 0;
    switch (flist.size()) {
    case 0:
	Usage();
	break;
    case 1:
	conf = new ConfTree(flist.front().c_str(), ro);
	break;
    default:
	conf = new ConfStack<ConfTree>(flist, ro);
	break;
    }

    if (op_flags & OPT_w) {
	exit(!readwrite(conf));
    } else if (op_flags & OPT_q) {
	exit(!query(conf, nm, sub));
    } else if (op_flags & OPT_k) {
	if (!conf->ok()) {
	    cerr << "conf init error" << endl;
	    exit(1);
	}
	vector<string>lst = conf->getSubKeys();
	for (vector<string>::const_iterator it = lst.begin(); 
	     it != lst.end(); it++) {
	    cout << *it << endl;
	}
	exit(0);
    } else if (op_flags & OPT_a) {
	exit(!setvar(conf, nm, value, sub));
    } else if (op_flags & OPT_d) {
	exit(!erase(conf, nm, sub));
    } else if (op_flags & OPT_E) {
	exit(!eraseKey(conf, sub));
    } else if (op_flags & OPT_s) {
	if (!conf->ok()) {
	    cerr << "Cant open /parse conf file " << endl;
	    exit(1);
	}
	    
	string source;
	if (!conf->get(string("strings"), source, "")) {
	    cerr << "Cant get param 'strings'" << endl;
	    exit(1);
	}
	cout << "source: [" << source << "]" << endl;
	vector<string> strings;
	if (!stringToStrings(source, strings)) {
	    cerr << "parse failed" << endl;
	    exit(1);
	}
	    
	for (vector<string>::iterator it = strings.begin(); 
	     it != strings.end(); it++) {
	    cout << "[" << *it << "]" << endl;
	}
	     
    } else {
	if (!conf->ok()) {
	    fprintf(stderr, "Open failed\n");
	    exit(1);
	}
	printf("LIST\n");
	conf->showall();
	//printf("WALK\n");conf->sortwalk(mywalker, 0);
	printf("\nNAMES in global space:\n");
	vector<string> names = conf->getNames("");
	for (vector<string>::iterator it = names.begin();
             it!=names.end(); it++) 
	    cout << *it << " ";
        cout << endl;
	printf("\nNAMES in global space matching t* \n");
	names = conf->getNames("", "t*");
	for (vector<string>::iterator it = names.begin();
             it!=names.end(); it++) 
	    cout << *it << " ";
        cout << endl;
    }
}

#endif
