/* Copyright (C) 2011 J.F.Dockes
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
#ifndef _PTMUTEX_H_INCLUDED_
#define _PTMUTEX_H_INCLUDED_

#include <pthread.h>

/// A trivial wrapper/helper for pthread mutex locks


/// Lock storage with auto-initialization. Must be created before any
/// lock-using thread of course (possibly as a static object).
class PTMutexInit {
public:
	pthread_mutex_t m_mutex;
	int m_status;
	PTMutexInit()
	{
		m_status = pthread_mutex_init(&m_mutex, 0);
	}
};

/// Take the lock when constructed, release when deleted. Can be disabled
/// by constructor params for conditional use.
class PTMutexLocker {
public:
	// The nolock arg enables conditional locking
	PTMutexLocker(PTMutexInit& l, bool nolock = false)
	: m_lock(l), m_status(-1)
	{
		if (!nolock)
			m_status = pthread_mutex_lock(&m_lock.m_mutex);
	}
	~PTMutexLocker()
	{
		if (m_status == 0)
			pthread_mutex_unlock(&m_lock.m_mutex);
	}
	int ok() {return m_status == 0;}
	// For pthread_cond_wait etc.
	pthread_mutex_t *getMutex()
	{
		return &m_lock.m_mutex;
	}
private:
	PTMutexInit& m_lock;
	int m_status;
};

#endif /* _PTMUTEX_H_INCLUDED_ */
/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
