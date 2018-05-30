/*-
 * Copyright 2018 Aniket Pandey
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/ioctl.h>

#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <atf-c.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <bsm/libbsm.h>
#include <security/audit/audit_ioctl.h>

#include "utils.h"

/*
 * Checks the presence of "auditregex" in auditpipe(4) after the
 * corresponding system call has been triggered.
 */
static bool
get_records(const char *auditregex, FILE *pipestream)
{
	uint8_t *buff;
	tokenstr_t token;
	ssize_t size = 1024;
	char membuff[size];
	char del[] = ",";
	int reclen, bytes = 0;
	FILE *memstream;

	/*
	 * Open a stream on 'membuff' (address to memory buffer) for storing
	 * the audit records in the default mode.'reclen' is the length of the
	 * available records from auditpipe which is passed to the functions
	 * au_fetch_tok(3) and au_print_flags_tok(3) for further use.
	 */
	memstream = fmemopen(membuff, size, "w");
	reclen = au_read_rec(pipestream, &buff);

	/*
	 * Iterate through each BSM token, extracting the bits that are
	 * required to start processing the token sequences.
	 */
	while (bytes < reclen) {
		if (au_fetch_tok(&token, buff + bytes, reclen - bytes) == -1)
			atf_tc_fail("Incomplete Audit Record");

		/* Print the tokens as they are obtained, in the default form */
		au_print_flags_tok(memstream, &token, (char *)del, AU_OFLAG_NONE);
		bytes += token.len;
	}

	free(buff);
	fclose(memstream);
	return (atf_utils_grep_string("%s", membuff, auditregex));
}

/*
 * Override the system-wide audit mask settings in /etc/security/audit_control
 * and set the auditpipe's maximum allowed queue length limit
 */
static void
set_preselect_mode(int filedesc, au_mask_t *fmask)
{
	int qlimit_max;
	int fmode = AUDITPIPE_PRESELECT_MODE_LOCAL;

	/* Set local preselection mode for auditing */
	if (ioctl(filedesc, AUDITPIPE_SET_PRESELECT_MODE, &fmode) < 0)
		atf_tc_fail("Preselection mode: %s", strerror(errno));

	/* Set local preselection flag corresponding to the audit_event */
	if (ioctl(filedesc, AUDITPIPE_SET_PRESELECT_FLAGS, fmask) < 0)
		atf_tc_fail("Preselection flag: %s", strerror(errno));

	/* Set local preselection flag for non-attributable audit_events */
	if (ioctl(filedesc, AUDITPIPE_SET_PRESELECT_NAFLAGS, fmask) < 0)
		atf_tc_fail("Preselection naflag: %s", strerror(errno));

	/* Query the maximum possible queue length limit for auditpipe */
	if (ioctl(filedesc, AUDITPIPE_GET_QLIMIT_MAX, &qlimit_max) < 0)
		atf_tc_fail("Query max-limit: %s", strerror(errno));

	/* Set the queue length limit as obtained from previous step */
	if (ioctl(filedesc, AUDITPIPE_SET_QLIMIT, &qlimit_max) < 0)
		atf_tc_fail("Set max-qlimit: %s", strerror(errno));

	/* This removes any outstanding record on the auditpipe */
	if (ioctl(filedesc, AUDITPIPE_FLUSH) < 0)
		atf_tc_fail("Auditpipe flush: %s", strerror(errno));
}

/*
 * Get the corresponding audit_mask for class-name "name" then set the
 * success and failure bits for fmask to be used as the ioctl argument
 */
static au_mask_t
get_audit_mask(const char *name)
{
	au_mask_t fmask;
	au_class_ent_t *class;

	ATF_REQUIRE((class = getauclassnam(name)) != NULL);
	fmask.am_success = class->ac_class;
	fmask.am_failure = class->ac_class;
	return (fmask);
}

/*
 * Loop until the auditpipe returns something, check if it is what
 * we want, else repeat the procedure until ppoll(2) times out.
 */
static void
check_auditpipe(struct pollfd fd[], const char *auditregex, FILE *pipestream)
{
	struct timespec currtime, endtime, timeout;

	/* Set the expire time for poll(2) while waiting for syscall audit */
	ATF_REQUIRE_EQ(0, clock_gettime(CLOCK_MONOTONIC, &endtime));
	endtime.tv_sec += 5;
	timeout.tv_nsec = endtime.tv_nsec;

	for (;;) {
		/* Update the time left for auditpipe to return any event */
		ATF_REQUIRE_EQ(0, clock_gettime(CLOCK_MONOTONIC, &currtime));
		timeout.tv_sec = endtime.tv_sec - currtime.tv_sec;

		switch (ppoll(fd, 1, &timeout, NULL)) {
		/* ppoll(2) returns, check if it's what we want */
		case 1:
			if (fd[0].revents & POLLIN) {
				if (get_records(auditregex, pipestream))
					return;
			} else {
				atf_tc_fail("Auditpipe returned an "
				"unknown event %#x", fd[0].revents);
			}
			break;

		/* poll(2) timed out */
		case 0:
			atf_tc_fail("%s not found in auditpipe within the "
					"time limit", auditregex);
			break;

		/* poll(2) standard error */
		case -1:
			atf_tc_fail("Poll: %s", strerror(errno));
			break;

		default:
			atf_tc_fail("Poll returned an unknown event");
		}
	}
}

/*
 * Wrapper functions around static "check_auditpipe"
 */
static void
check_audit_startup(struct pollfd fd[], const char *auditrgx, FILE *pipestream) {
	check_auditpipe(fd, auditrgx, pipestream);
}

void
check_audit(struct pollfd fd[], const char *auditrgx, FILE *pipestream) {
	check_auditpipe(fd, auditrgx, pipestream);

	/* Cleanup */
	fclose(pipestream);
	close(fd[0].fd);
}

FILE
*setup(struct pollfd fd[], const char *name)
{
	au_mask_t fmask;
	fmask = get_audit_mask(name);
	FILE *pipestream;

	fd[0].fd = open("/dev/auditpipe", O_RDONLY);
	fd[0].events = POLLIN;
	pipestream = fdopen(fd[0].fd, "r");

	ATF_REQUIRE_EQ(0, system("service auditd onestatus || \
	{ service auditd onestart && touch started_auditd ; }"));

	/* If 'started_auditd' exists, that means we started auditd(8) */
	if (atf_utils_file_exists("started_auditd"))
		check_audit_startup(fd, "audit startup", pipestream);

	/* Set local preselection parameters specific to "name" audit_class */
	set_preselect_mode(fd[0].fd, &fmask);
	return (pipestream);
}

void
cleanup(void)
{
	system("[ -f started_auditd ] && service auditd onestop > \
		/dev/null 2>&1");
}
