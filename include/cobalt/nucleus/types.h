/*
 * Copyright (C) 2001,2002,2003 Philippe Gerum <rpm@xenomai.org>.
 *
 * Xenomai is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _XENO_NUCLEUS_TYPES_H
#define _XENO_NUCLEUS_TYPES_H

#ifdef __KERNEL__
#include <linux/errno.h>
#include <linux/compiler.h>
#else /* !__KERNEL__ */
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#ifndef BITS_PER_LONG
#include <stdint.h>
#define BITS_PER_LONG __WORDSIZE
#endif /* !BITS_PER_LONG */
#endif /* __KERNEL__ */

#include <nucleus/assert.h>
#include <asm/xenomai/atomic.h>

#ifdef CONFIG_LTT
#include <linux/marker.h>
#else
#undef trace_mark
#define trace_mark(channel, ev, fmt, args...)	do { } while (0)
#endif

#if BITS_PER_LONG == 32
#define __natural_word_type int
#else  /* defaults to long otherwise */
#define __natural_word_type long
#endif

typedef unsigned long xnsigmask_t;

typedef unsigned long long xnticks_t;

typedef long long xnsticks_t;

typedef unsigned long long xntime_t; /* ns */

typedef long long xnstime_t;

typedef unsigned long xnhandle_t;

#define XN_NO_HANDLE ((xnhandle_t)0)

#define XN_HANDLE_SPARE0	((xnhandle_t)0x10000000)
#define XN_HANDLE_SPARE1	((xnhandle_t)0x20000000)
#define XN_HANDLE_SPARE2	((xnhandle_t)0x40000000)
#define XN_HANDLE_SPARE3	((xnhandle_t)0x80000000)
#define XN_HANDLE_SPARE_MASK	((xnhandle_t)0xf0000000)

#define xnhandle_mask_spare(handle)  ((handle) & ~XN_HANDLE_SPARE_MASK)
#define xnhandle_test_spare(handle, bits)  (!!((handle) & (bits)))
#define xnhandle_set_spare(handle, bits) \
	do { (handle) |= (bits); } while (0)
#define xnhandle_clear_spare(handle, bits) \
	do { (handle) &= ~(bits); } while (0)

struct xnintr;

typedef int (*xnisr_t)(struct xnintr *intr);

typedef void (*xniack_t)(unsigned irq, void *arg);

#define XN_INFINITE   (0)
#define XN_NONBLOCK   ((xnticks_t)-1)

/* Timer modes */
typedef enum xntmode {
	XN_RELATIVE,
	XN_ABSOLUTE,
	XN_REALTIME
} xntmode_t;

#define XN_APERIODIC_TICK  0

#define testbits(flags,mask) ((flags) & (mask))
#define setbits(flags,mask)  xnarch_atomic_set_mask(&(flags),mask)
#define clrbits(flags,mask)  xnarch_atomic_clear_mask(&(flags),mask)
#define __testbits(flags,mask) testbits(flags,mask)
#define __setbits(flags,mask)  do { (flags) |= (mask); } while(0)
#define __clrbits(flags,mask)  do { (flags) &= ~(mask); } while(0)

typedef atomic_flags_t xnflags_t;

#ifndef NULL
#define NULL 0
#endif

#define XNOBJECT_NAME_LEN 32

#define xnobject_copy_name(dst, src)					\
	do {								\
		strncpy((dst),						\
			((const char *)(src)) ?: "", XNOBJECT_NAME_LEN-1) \
			[XNOBJECT_NAME_LEN-1] = '\0';			\
	} while (0)

#define xnobject_create_name(dst, n, obj)	\
	snprintf(dst, n, "%p", obj)

#define minval(a,b) ((a) < (b) ? (a) : (b))
#define maxval(a,b) ((a) > (b) ? (a) : (b))

#define xnprintf(fmt,args...)  printk(KERN_INFO "Xenomai: " fmt , ##args)
#define xnloginfo(fmt,args...) printk(KERN_INFO "Xenomai: " fmt , ##args)
#define xnlogwarn(fmt,args...) printk(KERN_WARNING "Xenomai: " fmt , ##args)
#define xnlogerr(fmt,args...)  printk(KERN_ERR "Xenomai: " fmt , ##args)
#define xnlogerr_noprompt(fmt,args...) printk(KERN_ERR fmt , ##args)

void xnpod_fatal(const char *format, ...);

#endif /* !_XENO_NUCLEUS_TYPES_H */
