/*	$OpenBSD: parser.h,v 1.16 2011/04/13 20:53:18 gilles Exp $	*/

/*
 * Copyright (c) 2006 Pierre-Yves Ritschard <pyr@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

enum actions {
	NONE,
	SHUTDOWN,
	RELOAD,
	MONITOR,
	LOG_VERBOSE,
	LOG_BRIEF,
	SHOW_QUEUE,
	SHOW_RUNQUEUE,
	SHOW_STATS,
	PAUSE_MDA,
	PAUSE_MTA,
	PAUSE_SMTP,
	RESUME_MDA,
	RESUME_MTA,
	RESUME_SMTP,
};

struct parse_result {
	struct ctl_id	id;
	enum actions	action;
	const char     *data;
};

struct parse_result	*parse(int, char *[]);
