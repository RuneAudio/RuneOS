/*
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
#ifndef _CONFTREE_H_
#define  _CONFTREE_H_

/**
 * A simple configuration file implementation.
 *
 * Configuration files have lines like 'name = value', and/or like '[subkey]'
 *
 * Lines like '[subkey]' in the file define subsections, with independant
 * configuration namespaces. Only subsections holding at least one variable are 
 * significant (empty subsections may be deleted during an update, or not)
 *
 * Whitespace around name and value is insignificant.
 *
 * The names are case-sensitive but don't depend on it, this might change
 *
 * Values can be queried for, or set. 
 *
 * Any line without a '=' is a comment (a line like #var = value
 * actually assigns a variable named '#var', which is not a big issue)
 *
 * A configuration object can be created empty or by reading from a file or
 * a string.
 * All 'set' calls cause an immediate rewrite of the backing object if any 
 * (file or string) 
 * 
 * The ConfTree derived class interprets the subkeys as file paths and
 * lets subdir keys hierarchically inherit the properties from
 * parents.
 *
 * The ConfStack class stacks several Con(Simple/Tree) objects so that
 * parameters from the top of the stack override the values from lower
 * (useful to have central/personal config files)
 */

#include <string>
#include <map>
#include <vector>
#include <algorithm>

// rh7.3 likes iostream better...
#if defined(__GNUC__) && __GNUC__ < 3
#include <iostream>
#else
#include <istream>
#include <ostream>
#endif

using std::string;
using std::vector;
using std::map;
using std::istream;
using std::ostream;

#include "upmpdutils.hxx"

/** Internal class used for storing presentation information */
class ConfLine {
public:
    enum Kind {CFL_COMMENT, CFL_SK, CFL_VAR};
    Kind m_kind;
    string m_data;
    ConfLine(Kind k, const string& d) 
	: m_kind(k), m_data(d) 
    {
    }
    bool operator==(const ConfLine& o) 
    {
	return o.m_kind == m_kind && o.m_data == m_data;
    }
};

/** 
 * Virtual base class used to define an interface mostly useful for testing
 */
class ConfNull {
public:
    enum StatusCode {STATUS_ERROR=0, STATUS_RO=1, STATUS_RW=2};
    virtual ~ConfNull() {};
    virtual int get(const string &name, string &value, 
		    const string &sk = string()) const = 0;
    virtual bool hasNameAnywhere(const string& nm) const = 0;
    virtual int set(const string &nm, const string &val, 
		    const string &sk = string()) = 0;
    virtual bool ok() const = 0;
    virtual vector<string> getNames(const string &sk, const char* = 0)const = 0;
    virtual int erase(const string &, const string &) = 0;
    virtual int eraseKey(const string &) = 0;
    virtual void showall() const {};
    virtual vector<string> getSubKeys() const = 0;
    virtual vector<string> getSubKeys(bool) const = 0;
    virtual bool holdWrites(bool) = 0;
    virtual bool sourceChanged() const = 0;
};

/** 
 * Manages a simple configuration file with subsections.
 */
class ConfSimple : public ConfNull {
public:

    /**
     * Build the object by reading content from file.
     * @param filename file to open
     * @param readonly if true open readonly, else rw
     * @param tildexp  try tilde (home dir) expansion for subkey values
     */
    ConfSimple(const char *fname, int readonly = 0, bool tildexp = false);

    /**
     * Build the object by reading content from a string
     * @param data points to the data to parse. 
     * @param readonly if true open readonly, else rw
     * @param tildexp  try tilde (home dir) expansion for subsection names
     */
    ConfSimple(const string& data, int readonly = 0, bool tildexp = false);

    /**
     * Build an empty object. This will be memory only, with no backing store.
     * @param readonly if true open read only, else rw
     * @param tildexp  try tilde (home dir) expansion for subsection names
     */
    ConfSimple(int readonly = 0, bool tildexp = false);

    virtual ~ConfSimple() {};

    /** Origin file changed. Only makes sense if we read the data from a file */
    virtual bool sourceChanged() const;

    /** 
     * Decide if we actually rewrite the backing-store after modifying the
     * tree.
     */
    virtual bool holdWrites(bool on)
    {
	m_holdWrites = on;
	if (on == false) {
	    return write();
	} else
	    return true;
    }

    /** Clear, then reparse from string */
    void reparse(const string& in);

    /** Clear all content */
    void clear()
    {
	m_submaps.clear();
	m_order.clear();
    }

    /** 
     * Get value for named parameter, from specified subsection (looks in 
     * global space if sk is empty).
     * @return 0 if name not found, 1 else
     */
    virtual int get(const string &name, string &value, 
                    const string &sk = string()) const;

    /** 
     * Set value for named parameter in specified subsection (or global)
     * @return 0 for error, 1 else
     */
    virtual int set(const string &nm, const string &val, 
		    const string &sk = string());

    /**
     * Remove name and value from config
     */
    virtual int erase(const string &name, const string &sk);

    /**
     * Erase all names under given subkey (and subkey itself)
     */
    virtual int eraseKey(const string &sk);

    virtual StatusCode getStatus() const;
    virtual bool ok() const {return getStatus() != STATUS_ERROR;}

    /** 
     * Walk the configuration values, calling function for each.
     * The function is called with a null nm when changing subsections (the 
     * value is then the new subsection name)
     * @return WALK_STOP when/if the callback returns WALK_STOP, 
     *         WALK_CONTINUE else (got to end of config)
     */
    enum WalkerCode {WALK_STOP, WALK_CONTINUE};
    virtual WalkerCode sortwalk(WalkerCode 
				(*wlkr)(void *cldata, const string &nm, 
					const string &val),
				void *clidata) const;

    /** Print all values to stdout */
    virtual void showall() const;

    /** Return all names in given submap. */
    virtual vector<string> getNames(const string &sk, const char *pattern = 0) 
	const;

    /** Check if name is present in any submap. This is relatively expensive
     * but useful for saving further processing sometimes */
    virtual bool hasNameAnywhere(const string& nm) const;

    /**
     * Return all subkeys 
     */
    virtual vector<string> getSubKeys(bool) const 
    {
	return getSubKeys();
    }
    virtual vector<string> getSubKeys() const;
    /** Test for subkey existence */
    virtual bool hasSubKey(const string& sk) const
    {
	return m_submaps.find(sk) != m_submaps.end();
    }

    virtual string getFilename() const 
    {return m_filename;}

    /**
     * Copy constructor. Expensive but less so than a full rebuild
     */
    ConfSimple(const ConfSimple &rhs) 
	: ConfNull()
    {
	if ((status = rhs.status) == STATUS_ERROR)
	    return;
	m_filename = rhs.m_filename;
	m_submaps = rhs.m_submaps;
    }

    /**
     * Assignement. This is expensive
     */
    ConfSimple& operator=(const ConfSimple &rhs) 
    {
	if (this != &rhs && (status = rhs.status) != STATUS_ERROR) {
	    m_filename = rhs.m_filename;
	    m_submaps = rhs.m_submaps;
	}
	return *this;
    }

    /**
     * Write in file format to out
     */
    bool write(ostream& out) const;

protected:
    bool dotildexpand;
    StatusCode status;
private:
    // Set if we're working with a file
    string                            m_filename; 
    time_t                            m_fmtime;
    // Configuration data submaps (one per subkey, the main data has a
    // null subkey)
    map<string, map<string, string> > m_submaps;
    // Presentation data. We keep the comments, empty lines and
    // variable and subkey ordering information in there (for
    // rewriting the file while keeping hand-edited information)
    vector<ConfLine>                    m_order;
    // Control if we're writing to the backing store
    bool                              m_holdWrites;

    void parseinput(istream& input);
    bool write();
    // Internal version of set: no RW checking
    virtual int i_set(const string &nm, const string &val, 
		      const string &sk, bool init = false);
    bool i_changed(bool upd);
};

/**
 * This is a configuration class which attaches tree-like signification to the
 * submap names.
 *
 * If a given variable is not found in the specified section, it will be 
 * looked up the tree of section names, and in the global space.
 *
 * submap names should be '/' separated paths (ie: /sub1/sub2). No checking
 * is done, but else the class adds no functionality to ConfSimple.
 *
 * NOTE: contrary to common behaviour, the global or root space is NOT
 * designated by '/' but by '' (empty subkey). A '/' subkey will not
 * be searched at all.
 *
 * Note: getNames() : uses ConfSimple method, this does *not* inherit 
 *     names from englobing submaps.
 */
class ConfTree : public ConfSimple {

public:
    /* The constructors just call ConfSimple's, asking for key tilde 
     * expansion */
    ConfTree(const char *fname, int readonly = 0) 
	: ConfSimple(fname, readonly, true) {}
    ConfTree(const string &data, int readonly = 0)
	: ConfSimple(data, readonly, true) {}
    ConfTree(int readonly = 0)
	: ConfSimple(readonly, true) {}
    virtual ~ConfTree() {};
    ConfTree(const ConfTree& r)	: ConfSimple(r) {};
    ConfTree& operator=(const ConfTree& r) 
    {
	ConfSimple::operator=(r);
	return *this;
    }

    /** 
     * Get value for named parameter, from specified subsection, or its 
     * parents.
     * @return 0 if name not found, 1 else
     */
    virtual int get(const string &name, string &value, const string &sk) const;
};

/** 
 * Use several config files, trying to get values from each in order. Used to
 * have a central config, with possible overrides from more specific
 * (ie personal) ones.
 *
 * Notes: it's ok for some of the files not to exist, but the last
 * one must or we generate an error. We open all trees readonly, except the 
 * topmost one if requested. All writes go to the topmost file. Note that
 * erase() won't work except for parameters only defined in the topmost
 * file (it erases only from there).
 */
template <class T> class ConfStack : public ConfNull {
public:
    /// Construct from configuration file names. The earler
    /// files in have priority when fetching values. Only the first
    /// file will be updated if ro is false and set() is used.
    ConfStack(const vector<string> &fns, bool ro = true) 
    {
	construct(fns, ro);
    }
    /// Construct out of single file name and multiple directories
    ConfStack(const string& nm, const vector<string>& dirs, bool ro = true) 
    {
	vector<string> fns;
	for (vector<string>::const_iterator it = dirs.begin(); 
	     it != dirs.end(); it++){
	    fns.push_back(path_cat(*it, nm));
	}
	ConfStack::construct(fns, ro);
    }

    ConfStack(const ConfStack &rhs) 
	: ConfNull()
    {
	init_from(rhs);
    }

    virtual ~ConfStack() 
    {
	clear();
	m_ok = false;
    }

    ConfStack& operator=(const ConfStack &rhs) 
    {
	if (this != &rhs){
	    clear();
	    m_ok = rhs.m_ok;
	    if (m_ok)
		init_from(rhs);
	}
	return *this;
    }

    virtual bool sourceChanged() const
    {
	typename vector<T*>::const_iterator it;
	for (it = m_confs.begin();it != m_confs.end();it++) {
	    if ((*it)->sourceChanged())
		return true;
	}
	return false;
    }

    virtual int get(const string &name, string &value, const string &sk) const
    {
	typename vector<T*>::const_iterator it;
	for (it = m_confs.begin();it != m_confs.end();it++) {
	    if ((*it)->get(name, value, sk))
		return true;
	}
	return false;
    }

    virtual bool hasNameAnywhere(const string& nm) const
    {
	typename vector<T*>::const_iterator it;
	for (it = m_confs.begin();it != m_confs.end();it++) {
	    if ((*it)->hasNameAnywhere(nm))
		return true;
	}
	return false;
    }

    virtual int set(const string &nm, const string &val, 
                    const string &sk = string()) 
    {
	if (!m_ok)
	    return 0;
	//LOGDEB2(("ConfStack::set [%s]:[%s] -> [%s]\n", sk.c_str(),
	//nm.c_str(), val.c_str()));
	// Avoid adding unneeded entries: if the new value matches the
	// one out from the deeper configs, erase or dont add it
	// from/to the topmost file
	typename vector<T*>::iterator it = m_confs.begin();
	it++;
	while (it != m_confs.end()) {
	    string value;
	    if ((*it)->get(nm, value, sk)) {
		// This file has value for nm/sk. If it is the same as the new
		// one, no need for an entry in the topmost file. Else, stop
		// looking and add the new entry
		if (value == val) {
		    m_confs.front()->erase(nm, sk);
		    return true;
		} else {
		    break;
		}
	    }
	    it++;
	}

	return m_confs.front()->set(nm, val, sk);
    }

    virtual int erase(const string &nm, const string &sk) 
    {
	return m_confs.front()->erase(nm, sk);
    }
    virtual int eraseKey(const string &sk) 
    {
	return m_confs.front()->eraseKey(sk);
    }
    virtual bool holdWrites(bool on)
    {
	return m_confs.front()->holdWrites(on);
    }

    virtual vector<string> getNames(const string &sk, const char *pattern = 0)
	const
    {
	return getNames1(sk, pattern, false);
    }
    virtual vector<string> getNamesShallow(const string &sk, 
					   const char *patt = 0) const
    {
	return getNames1(sk, patt, true);
    }

    virtual vector<string> getNames1(const string &sk, const char *pattern,
				   bool shallow) const
    {
	vector<string> nms;
	typename vector<T*>::const_iterator it;
	bool skfound = false;
	for (it = m_confs.begin(); it != m_confs.end(); it++) {
	    if ((*it)->hasSubKey(sk)) {
		skfound = true;
		vector<string> lst = (*it)->getNames(sk, pattern);
		nms.insert(nms.end(), lst.begin(), lst.end());
	    }
	    if (shallow && skfound)
		break;
	}
	sort(nms.begin(), nms.end());
	vector<string>::iterator uit = unique(nms.begin(), nms.end());
	nms.resize(uit - nms.begin());
	return nms;
    }

    virtual vector<string> getSubKeys() const
    {
	return getSubKeys(false);
    }
    virtual vector<string> getSubKeys(bool shallow) const
    {
	vector<string> sks;
	typename vector<T*>::const_iterator it;
	for (it = m_confs.begin(); it != m_confs.end(); it++) {
	    vector<string> lst;
	    lst = (*it)->getSubKeys();
	    sks.insert(sks.end(), lst.begin(), lst.end());
	    if (shallow)
		break;
	}
	sort(sks.begin(), sks.end());
	vector<string>::iterator uit = unique(sks.begin(), sks.end());
	sks.resize(uit - sks.begin());
	return sks;
    }

    virtual bool ok() const {return m_ok;}

private:
    bool     m_ok;
    vector<T*> m_confs;

    /// Reset to pristine
    void clear() {
	typename vector<T*>::iterator it;
	for (it = m_confs.begin();it != m_confs.end();it++) {
	    delete (*it);
	}
	m_confs.clear();
    }

    /// Common code to initialize from existing object
    void init_from(const ConfStack &rhs) {
	if ((m_ok = rhs.m_ok)) {
	    typename vector<T*>::const_iterator it;
	    for (it = rhs.m_confs.begin();it != rhs.m_confs.end();it++) {
		m_confs.push_back(new T(**it));
	    }
	}
    }

    /// Common construct from file names code
    void construct(const vector<string> &fns, bool ro) {
	vector<string>::const_iterator it;
	bool lastok = false;
	for (it = fns.begin(); it != fns.end(); it++) {
	    T* p = new T(it->c_str(), ro);
	    if (p && p->ok()) {
		m_confs.push_back(p);
		lastok = true;
	    } else {
		delete p;
		lastok = false;
		if (!ro) {
		    // For rw acccess, the topmost file needs to be ok
		    // (ro is set to true after the first file)
		    break;
		}
	    }
	    ro = true;
	}
	m_ok = lastok;
    }
};

#endif /*_CONFTREE_H_ */
