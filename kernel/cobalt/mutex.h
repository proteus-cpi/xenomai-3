/*
 * Written by Gilles Chanteperdrix <gilles.chanteperdrix@xenomai.org>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _POSIX_MUTEX_H
#define _POSIX_MUTEX_H

#include <pthread.h>
#include <asm/xenomai/atomic.h>

struct cobalt_mutex;

struct mutex_dat {
	xnarch_atomic_t owner;
	unsigned long flags;

#define COBALT_MUTEX_COND_SIGNAL 0x00000001
#define COBALT_MUTEX_ERRORCHECK  0x00000002
};

union __xeno_mutex {
	pthread_mutex_t native_mutex;
	struct __shadow_mutex {
		unsigned magic;
		unsigned lockcnt;
		struct cobalt_mutex *mutex;
		union {
			unsigned dat_offset;
			struct mutex_dat *dat;
		};
		struct cobalt_mutexattr attr;
	} shadow_mutex;
};

#define COBALT_MUTEX_MAGIC (0x86860303)

#ifdef __KERNEL__

#include "internal.h"
#include "thread.h"
#include "cond.h"

typedef struct cobalt_mutex {
	unsigned magic;
	xnsynch_t synchbase;
	xnholder_t link;            /* Link in cobalt_mutexq */

#define link2mutex(laddr) container_of(laddr, cobalt_mutex_t, link)

	xnqueue_t conds;

	pthread_mutexattr_t attr;
	cobalt_kqueues_t *owningq;
} cobalt_mutex_t;

extern pthread_mutexattr_t cobalt_default_mutex_attr;

/* must be called with nklock locked, interrupts off. */
static inline int cobalt_mutex_acquire_unchecked(xnthread_t *cur,
						 cobalt_mutex_t *mutex,
						 int timed,
						 xnticks_t abs_to)

{
	if (timed)
		xnsynch_acquire(&mutex->synchbase, abs_to, XN_REALTIME);
	else
		xnsynch_acquire(&mutex->synchbase, XN_INFINITE, XN_RELATIVE);

	if (xnthread_test_info(cur, XNBREAK | XNRMID | XNTIMEO)) {
		if (xnthread_test_info(cur, XNBREAK))
			return -EINTR;
		else if (xnthread_test_info(cur, XNTIMEO))
			return -ETIMEDOUT;
		else /* XNRMID */
			return -EINVAL;
	}

	return 0;
}

static inline int cobalt_mutex_release(xnthread_t *cur, cobalt_mutex_t *mutex)
{
	struct mutex_dat *datp;
	unsigned long flags;
	xnholder_t *holder;
	int need_resched;

	if (!cobalt_obj_active(mutex, COBALT_MUTEX_MAGIC, struct cobalt_mutex))
		 return -EINVAL;

#if XENO_DEBUG(POSIX)
	if (mutex->owningq != cobalt_kqueues(mutex->attr.pshared))
		return -EPERM;
#endif /* XENO_DEBUG(POSIX) */

	need_resched = 0;
	datp = container_of(mutex->synchbase.fastlock, struct mutex_dat, owner);
	flags = datp->flags;
	if ((flags & COBALT_MUTEX_COND_SIGNAL)) {
		datp->flags = flags & ~COBALT_MUTEX_COND_SIGNAL;
		for (holder = getheadq(&mutex->conds);
		     holder; holder = nextq(&mutex->conds, holder)) {
			struct cobalt_cond *cond = mutex_link2cond(holder);
			need_resched |= cobalt_cond_deferred_signals(cond);
		}
	}
	need_resched |= xnsynch_release(&mutex->synchbase, cur) != NULL;

	return need_resched;
}

int cobalt_mutexattr_init(pthread_mutexattr_t __user *u_attr);

int cobalt_mutexattr_destroy(pthread_mutexattr_t __user *u_attr);

int cobalt_mutexattr_gettype(const pthread_mutexattr_t __user *u_attr,
			     int __user *u_type);

int cobalt_mutexattr_settype(pthread_mutexattr_t __user *u_attr,
			     int type);

int cobalt_mutexattr_getprotocol(const pthread_mutexattr_t __user *u_attr,
				 int __user *u_proto);

int cobalt_mutexattr_setprotocol(pthread_mutexattr_t __user *u_attr,
				 int proto);

int cobalt_mutexattr_getpshared(const pthread_mutexattr_t __user *u_attr,
				int __user *u_pshared);

int cobalt_mutexattr_setpshared(pthread_mutexattr_t __user *u_attr,
				int pshared);

int cobalt_mutex_check_init(struct __shadow_mutex __user *u_mx);

int cobalt_mutex_init(struct __shadow_mutex __user *u_mx,
		      const pthread_mutexattr_t __user *u_attr);

int cobalt_mutex_destroy(struct __shadow_mutex __user *u_mx);

int cobalt_mutex_trylock(struct __shadow_mutex __user *u_mx);

int cobalt_mutex_lock(struct __shadow_mutex __user *u_mx);

int cobalt_mutex_timedlock(struct __shadow_mutex __user *u_mx,
			   const struct timespec __user *u_ts);

int cobalt_mutex_unlock(struct __shadow_mutex __user *u_mx);

void cobalt_mutexq_cleanup(cobalt_kqueues_t *q);

void cobalt_mutex_pkg_init(void);

void cobalt_mutex_pkg_cleanup(void);

#else /* ! __KERNEL__ */

extern unsigned long xeno_sem_heap[2];

static inline struct mutex_dat *mutex_get_datp(struct __shadow_mutex *shadow)
{
	if (shadow->attr.pshared)
		return (struct mutex_dat *)(xeno_sem_heap[1] + shadow->dat_offset);

	return shadow->dat;
}

static inline xnarch_atomic_t *mutex_get_ownerp(struct __shadow_mutex *shadow)
{
	return &mutex_get_datp(shadow)->owner;
}

#endif /* __KERNEL__ */

#endif /* !_POSIX_MUTEX_H */
