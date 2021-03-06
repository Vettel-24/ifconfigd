/*	$OpenBSD: uthread_write.c,v 1.14 2010/01/03 23:05:35 fgsch Exp $	*/
/*
 * Copyright (c) 1995-1998 John Birrell <jb@cimlogic.com.au>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by John Birrell.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOHN BIRRELL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: uthread_write.c,v 1.12 1999/08/28 00:03:54 peter Exp $
 *
 */
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#ifdef _THREAD_SAFE
#include <pthread.h>
#include "pthread_private.h"

ssize_t
write(int fd, const void *buf, size_t nbytes)
{
	struct pthread	*curthread = _get_curthread();
	int	blocking;
	int	type;
	size_t num = 0;
	ssize_t n;
	ssize_t	ret;

	/* This is a cancellation point: */
	_thread_enter_cancellation_point();

	/* POSIX says to do just this: */
	if (nbytes == 0)
		ret = 0;
	else if (nbytes > SSIZE_MAX) {
		errno = EINVAL;
		ret = -1;
	/* Lock the file descriptor for write: */
	} else if ((ret = _FD_LOCK(fd, FD_WRITE, NULL)) == 0) {
		/* Get the read/write mode type: */
		type = _thread_fd_table[fd]->status_flags->flags & O_ACCMODE;

		/* Check if the file is not open for write: */
		if (type != O_WRONLY && type != O_RDWR) {
			/* File is not open for write: */
			errno = EBADF;
			ret = -1;
		}

		else {
		/* Check if file operations are to block */
		blocking = ((_thread_fd_table[fd]->status_flags->flags & O_NONBLOCK) == 0);

		/*
		 * Loop while no error occurs and until the expected number
		 * of bytes are written if performing a blocking write:
		 */
		while (ret == 0) {
			/* Perform a non-blocking write syscall: */
			n = _thread_sys_write(fd, (caddr_t)buf + (ptrdiff_t)num, 
			    nbytes - num);

			/* Check if one or more bytes were written: */
			if (n > 0)
				/*
				 * Keep a count of the number of bytes
				 * written:
				 */
				num += (size_t)n;

			/*
			 * If performing a blocking write, check if the
			 * write would have blocked or if some bytes
			 * were written but there are still more to
			 * write:
			 */
			if (blocking && ((n < 0 && (errno == EWOULDBLOCK ||
			    errno == EAGAIN)) || (n > 0 && num < nbytes))) {
				curthread->data.fd.fd = fd;
				_thread_kern_set_timeout(_FD_SNDTIMEO(fd));

				/* Reset the interrupted operation flag: */
				curthread->interrupted = 0;
				curthread->closing_fd = 0;
				curthread->timeout = 0;

				_thread_kern_sched_state(PS_FDW_WAIT,
				    __FILE__, __LINE__);

				/*
				 * Check if the operation was
				 * interrupted by a signal
				 */
				if (curthread->interrupted ||
				    curthread->closing_fd ||
				    curthread->timeout) {
					if (num > 0) {
						/* Return partial success: */
						ret = (ssize_t)num;
					} else {
						/* Return an error: */
						if (curthread->closing_fd)
							errno = EBADF;
						else if (curthread->interrupted)
							errno = EINTR;
						else
							errno = EWOULDBLOCK;
						ret = -1;
					}
				}

			/*
			 * If performing a non-blocking write,
			 * just return whatever the write syscall did:
			 */
			} else if (!blocking) {
				/* A non-blocking call might return zero: */
				ret = n;
				break;

			/*
			 * If there was an error, return partial success
			 * (if any bytes were written) or else the error:
			 */
			} else if (n <= 0) {
				if (num > 0)
					ret = (ssize_t)num;
				else
					ret = n;
				if (n == 0)                                                                            
					break;

			/* Check if the write has completed: */
			} else if (num >= nbytes)
				/* Return the number of bytes written: */
				ret = (ssize_t)num;
		}
		}
		_FD_UNLOCK(fd, FD_WRITE);
	}

	/* No longer in a cancellation point: */
	_thread_leave_cancellation_point();

	return (ret);
}
#endif
