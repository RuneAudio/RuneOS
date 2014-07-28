/*	 Copyright (C) 2012 J.F.Dockes
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
#ifndef _WORKQUEUE_H_INCLUDED_
#define _WORKQUEUE_H_INCLUDED_

#include <pthread.h>
#include <time.h>

#include <string>
#include <queue>
#include <unordered_map>
#include <unordered_set>
using std::unordered_map;
using std::unordered_set;
using std::queue;
using std::string;

#include "ptmutex.hxx"

/// Store per-worker-thread data. Just an initialized timespec, and
/// used at the moment.
class WQTData {
	public:
	WQTData() {wstart.tv_sec = 0; wstart.tv_nsec = 0;}
	struct timespec wstart;
};

/**
 * A WorkQueue manages the synchronisation around a queue of work items,
 * where a number of client threads queue tasks and a number of worker
 * threads take and execute them. The goal is to introduce some level
 * of parallelism between the successive steps of a previously single
 * threaded pipeline. For example data extraction / data preparation / index
 * update, but this could have other uses.
 *
 * There is no individual task status return. In case of fatal error,
 * the client or worker sets an end condition on the queue. A second
 * queue could conceivably be used for returning individual task
 * status.
 */
template <class T> class WorkQueue {
public:

	/** Create a WorkQueue
	 * @param name for message printing
	 * @param hi number of tasks on queue before clients blocks. Default 0
	 *	  meaning no limit. hi == -1 means that the queue is disabled.
	 * @param lo minimum count of tasks before worker starts. Default 1.
	 */
	WorkQueue(const string& name, size_t hi = 0, size_t lo = 1)
		: m_name(name), m_high(hi), m_low(lo),
		  m_workers_exited(0), m_clients_waiting(0), m_workers_waiting(0),
		  m_tottasks(0), m_nowake(0), m_workersleeps(0), m_clientsleeps(0)
	{
		m_ok = (pthread_cond_init(&m_ccond, 0) == 0) &&
		(pthread_cond_init(&m_wcond, 0) == 0);
	}

	~WorkQueue()
	{
		if (!m_worker_threads.empty())
			setTerminateAndWait();
	}

	/** Start the worker threads.
	 *
	 * @param nworkers number of threads copies to start.
	 * @param start_routine thread function. It should loop
	 *		taking (QueueWorker::take()) and executing tasks.
	 * @param arg initial parameter to thread function.
	 * @return true if ok.
	 */
	bool start(int nworkers, void *(*workproc)(void *), void *arg)
	{
		PTMutexLocker lock(m_mutex);
		for	 (int i = 0; i < nworkers; i++) {
			int err;
			pthread_t thr;
			if ((err = pthread_create(&thr, 0, workproc, arg))) {
				return false;
			}
			m_worker_threads.insert(pair<pthread_t, WQTData>(thr, WQTData()));
		}
		return true;
	}

	/** Add item to work queue, called from client.
	 *
	 * Sleeps if there are already too many.
	 */
	bool put(T t)
	{
		PTMutexLocker lock(m_mutex);
		if (!lock.ok() || !ok()) {
			return false;
		}

		while (ok() && m_high > 0 && m_queue.size() >= m_high) {
		m_clientsleeps++;
			// Keep the order: we test ok() AFTER the sleep...
		m_clients_waiting++;
			if (pthread_cond_wait(&m_ccond, lock.getMutex()) || !ok()) {
		m_clients_waiting--;
				return false;
			}
		m_clients_waiting--;
		}

		m_queue.push(t);
		if (m_workers_waiting > 0) {
			// Just wake one worker, there is only one new task.
			pthread_cond_signal(&m_wcond);
		} else {
			m_nowake++;
		}

		return true;
	}

	/** Wait until the queue is inactive. Called from client.
	 *
	 * Waits until the task queue is empty and the workers are all
	 * back sleeping. Used by the client to wait for all current work
	 * to be completed, when it needs to perform work that couldn't be
	 * done in parallel with the worker's tasks, or before shutting
	 * down. Work can be resumed after calling this. Note that the
	 * only thread which can call it safely is the client just above
	 * (which can control the task flow), else there could be
	 * tasks in the intermediate queues.
	 * To rephrase: there is no warranty on return that the queue is actually
	 * idle EXCEPT if the caller knows that no jobs are still being created.
	 * It would be possible to transform this into a safe call if some kind
	 * of suspend condition was set on the queue by waitIdle(), to be reset by
	 * some kind of "resume" call. Not currently the case.
	 */
	bool waitIdle()
	{
		PTMutexLocker lock(m_mutex);
		if (!lock.ok() || !ok()) {
			return false;
		}

		// We're done when the queue is empty AND all workers are back
		// waiting for a task.
		while (ok() && (m_queue.size() > 0 ||
						m_workers_waiting != m_worker_threads.size())) {
			m_clients_waiting++;
			if (pthread_cond_wait(&m_ccond, lock.getMutex())) {
				m_clients_waiting--;
				m_ok = false;
				return false;
			}
			m_clients_waiting--;
		}

		return ok();
	}


	/** Tell the workers to exit, and wait for them.
	 *
	 * Does not bother about tasks possibly remaining on the queue, so
	 * should be called after waitIdle() for an orderly shutdown.
	 */
	void* setTerminateAndWait()
	{
		PTMutexLocker lock(m_mutex);

		if (m_worker_threads.empty()) {
			// Already called ?
			return (void*)0;
		}

		// Wait for all worker threads to have called workerExit()
		m_ok = false;
		while (m_workers_exited < m_worker_threads.size()) {
			pthread_cond_broadcast(&m_wcond);
			m_clients_waiting++;
			if (pthread_cond_wait(&m_ccond, lock.getMutex())) {
				m_clients_waiting--;
				return (void*)0;
			}
			m_clients_waiting--;
		}

		// Perform the thread joins and compute overall status
		// Workers return (void*)1 if ok
		void *statusall = (void*)1;
		unordered_map<pthread_t,  WQTData>::iterator it;
		while (!m_worker_threads.empty()) {
			void *status;
			it = m_worker_threads.begin();
			pthread_join(it->first, &status);
			if (status == (void *)0)
				statusall = status;
			m_worker_threads.erase(it);
		}

		// Reset to start state.
		m_workers_exited = m_clients_waiting = m_workers_waiting =
			m_tottasks = m_nowake = m_workersleeps = m_clientsleeps = 0;
		m_ok = true;

		return statusall;
	}

	/** Take task from queue. Called from worker.
	 *
	 * Sleeps if there are not enough. Signal if we go to sleep on empty
	 * queue: client may be waiting for our going idle.
	 */
	bool take(T* tp, size_t *szp = 0)
	{
		PTMutexLocker lock(m_mutex);
		if (!lock.ok() || !ok()) {
			return false;
		}

		while (ok() && m_queue.size() < m_low) {
			m_workersleeps++;
			m_workers_waiting++;
			if (m_queue.empty())
				pthread_cond_broadcast(&m_ccond);
			if (pthread_cond_wait(&m_wcond, lock.getMutex()) || !ok()) {
				m_workers_waiting--;
				return false;
			}
			m_workers_waiting--;
		}

		m_tottasks++;
		*tp = m_queue.front();
		if (szp)
			*szp = m_queue.size();
		m_queue.pop();
		if (m_clients_waiting > 0) {
			// No reason to wake up more than one client thread
			pthread_cond_signal(&m_ccond);
		} else {
			m_nowake++;
		}
		return true;
	}

	/** Advertise exit and abort queue. Called from worker
	 *
	 * This would happen after an unrecoverable error, or when
	 * the queue is terminated by the client. Workers never exit normally,
	 * except when the queue is shut down (at which point m_ok is set to
	 * false by the shutdown code anyway). The thread must return/exit
	 * immediately after calling this.
	 */
	void workerExit()
	{
		PTMutexLocker lock(m_mutex);
		m_workers_exited++;
		m_ok = false;
		pthread_cond_broadcast(&m_ccond);
	}

	size_t qsize()
	{
		PTMutexLocker lock(m_mutex);
		size_t sz = m_queue.size();
		return sz;
	}

private:
	bool ok()
	{
		bool isok = m_ok && m_workers_exited == 0 && !m_worker_threads.empty();
		return isok;
	}

	long long nanodiff(const struct timespec& older,
					   const struct timespec& newer)
	{
		return (newer.tv_sec - older.tv_sec) * 1000000000LL
			+ newer.tv_nsec - older.tv_nsec;
	}

	// Configuration
	string m_name;
	size_t m_high;
	size_t m_low;

	// Status
	// Worker threads having called exit
	unsigned int m_workers_exited;
	bool m_ok;

	// Per-thread data. The data is not used currently, this could be
	// a set<pthread_t>
	unordered_map<pthread_t, WQTData> m_worker_threads;

	// Synchronization
	queue<T> m_queue;
	pthread_cond_t m_ccond;
	pthread_cond_t m_wcond;
	PTMutexInit m_mutex;
	// Client/Worker threads currently waiting for a job
	unsigned int m_clients_waiting;
	unsigned int m_workers_waiting;

	// Statistics
	unsigned int m_tottasks;
	unsigned int m_nowake;
	unsigned int m_workersleeps;
	unsigned int m_clientsleeps;
};

#endif /* _WORKQUEUE_H_INCLUDED_ */
/* Local Variables: */
/* mode: c++ */
/* c-basic-offset: 4 */
/* tab-width: 4 */
/* indent-tabs-mode: t */
/* End: */
