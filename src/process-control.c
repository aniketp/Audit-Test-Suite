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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <atf-c.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.h"

static pid_t pid;
static int status;
static int filedesc;
static struct pollfd fds[1];
static char pcregex[60];
static char bin[] = "/usr/bin/true";
static char argument[] = "sample-argument";
static char *arg[] = {bin, argument, NULL};


ATF_TC_WITH_CLEANUP(chdir_success);
ATF_TC_HEAD(chdir_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"chdir(2) call");
}

ATF_TC_BODY(chdir_success, tc)
{
	/* Build an absolute path to the test-case directory */
	char dirpath[50];
	ATF_REQUIRE(getcwd(dirpath, sizeof(dirpath)) != NULL);
	snprintf(pcregex, 60, "chdir.*%s.*return,success", dirpath);

	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(0, chdir(dirpath));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(chdir_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(chdir_failure);
ATF_TC_HEAD(chdir_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"chdir(2) call");
}

ATF_TC_BODY(chdir_failure, tc)
{
	const char *regex = "chdir.*return,failure : Bad address";
	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(-1, chdir(NULL));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(chdir_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fchdir_success);
ATF_TC_HEAD(fchdir_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fchdir(2) call");
}

ATF_TC_BODY(fchdir_success, tc)
{
	/* Build an absolute path to the test-case directory */
	char dirpath[50];
	ATF_REQUIRE(getcwd(dirpath, sizeof(dirpath)) != NULL);
	filedesc = open(dirpath, O_RDONLY);

	/* audit record generated by fchdir(2) does contain the filedesc */
	pid = getpid();
	snprintf(pcregex, 60, "fchdir.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(0, fchdir(filedesc));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(fchdir_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fchdir_failure);
ATF_TC_HEAD(fchdir_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fchdir(2) call");
}

ATF_TC_BODY(fchdir_failure, tc)
{
	/* Audit record should contain hex(-1)*/
	snprintf(pcregex, 60, "fchdir.*0x%x.*failure.*Bad file descriptor", -1);
	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(-1, fchdir(-1));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(fchdir_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(chroot_success);
ATF_TC_HEAD(chroot_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"chroot(2) call");
}

ATF_TC_BODY(chroot_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "chroot.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(0, chroot("/"));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(chroot_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(chroot_failure);
ATF_TC_HEAD(chroot_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"chroot(2) call");
}

ATF_TC_BODY(chroot_failure, tc)
{
	const char *regex = "chroot.*return,failure : Bad address";
	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(-1, chroot(NULL));
	check_audit(fds, regex, pipefd);
}

ATF_TC_CLEANUP(chroot_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(umask_success);
ATF_TC_HEAD(umask_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"umask(2) call");
}

ATF_TC_BODY(umask_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "umask.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	umask(0);
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(umask_success, tc)
{
	cleanup();
}

/*
 * umask(2) system call is always successful. So no test case for failure mode
 */


ATF_TC_WITH_CLEANUP(setuid_success);
ATF_TC_HEAD(setuid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"setuid(2) call");
}

ATF_TC_BODY(setuid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "setuid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(0, setuid(0));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(setuid_success, tc)
{
	cleanup();
}

/*
 * setuid(2) fails only when the current user is not root. So no test case for
 * failure mode since the required_user="root"
 */


ATF_TC_WITH_CLEANUP(seteuid_success);
ATF_TC_HEAD(seteuid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"seteuid(2) call");
}

ATF_TC_BODY(seteuid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "seteuid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(0, seteuid(0));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(seteuid_success, tc)
{
	cleanup();
}

/*
 * seteuid(2) fails only when the current user is not root. So no test case for
 * failure mode since the required_user="root"
 */


ATF_TC_WITH_CLEANUP(setgid_success);
ATF_TC_HEAD(setgid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"setgid(2) call");
}

ATF_TC_BODY(setgid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "setgid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(0, setgid(0));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(setgid_success, tc)
{
	cleanup();
}

/*
 * setgid(2) fails only when the current user is not root. So no test case for
 * failure mode since the required_user="root"
 */


ATF_TC_WITH_CLEANUP(setegid_success);
ATF_TC_HEAD(setegid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"setegid(2) call");
}

ATF_TC_BODY(setegid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "setegid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(0, setegid(0));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(setegid_success, tc)
{
	cleanup();
}

/*
 * setegid(2) fails only when the current user is not root. So no test case for
 * failure mode since the required_user="root"
 */


ATF_TC_WITH_CLEANUP(setregid_success);
ATF_TC_HEAD(setregid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"setregid(2) call");
}

ATF_TC_BODY(setregid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "setregid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	/* setregid(-1, -1) does not change any GIDs */
	ATF_REQUIRE_EQ(0, setregid(-1, -1));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(setregid_success, tc)
{
	cleanup();
}

/*
 * setregid(2) fails only when the current user is not root. So no test case for
 * failure mode since the required_user="root"
 */


ATF_TC_WITH_CLEANUP(setreuid_success);
ATF_TC_HEAD(setreuid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"setreuid(2) call");
}

ATF_TC_BODY(setreuid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "setreuid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	/* setreuid(-1, -1) does not change any UIDs */
	ATF_REQUIRE_EQ(0, setreuid(-1, -1));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(setreuid_success, tc)
{
	cleanup();
}

/*
 * setregid(2) fails only when the current user is not root. So no test case for
 * failure mode since the required_user="root"
 */


ATF_TC_WITH_CLEANUP(setresuid_success);
ATF_TC_HEAD(setresuid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"setresuid(2) call");
}

ATF_TC_BODY(setresuid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "setresuid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	/* setresuid(-1, -1, -1) does not change any UIDs */
	ATF_REQUIRE_EQ(0, setresuid(-1, -1, -1));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(setresuid_success, tc)
{
	cleanup();
}

/*
 * setresuid(2) fails only when the current user is not root. So no test case
 * for failure mode since the required_user="root"
 */


ATF_TC_WITH_CLEANUP(setresgid_success);
ATF_TC_HEAD(setresgid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"setresgid(2) call");
}

ATF_TC_BODY(setresgid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "setresgid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	/* setresgid(-1, -1, -1) does not change any GIDs */
	ATF_REQUIRE_EQ(0, setresgid(-1, -1, -1));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(setresgid_success, tc)
{
	cleanup();
}

/*
 * setresgid(2) fails only when the current user is not root. So no test case
 * for failure mode since the required_user="root"
 */


ATF_TC_WITH_CLEANUP(getresuid_success);
ATF_TC_HEAD(getresuid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"getresuid(2) call");
}

ATF_TC_BODY(getresuid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "getresuid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(0, getresuid(NULL, NULL, NULL));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(getresuid_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(getresuid_failure);
ATF_TC_HEAD(getresuid_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"getresuid(2) call");
}

ATF_TC_BODY(getresuid_failure, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "getresuid.*%d.*return,fail.*Bad address", pid);

	FILE *pipefd = setup(fds, "pc");
	/* Failure reason: Invalid address "-1" */
	ATF_REQUIRE_EQ(-1, getresuid((uid_t *)-1, NULL, NULL));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(getresuid_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(getresgid_success);
ATF_TC_HEAD(getresgid_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"getresgid(2) call");
}

ATF_TC_BODY(getresgid_success, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "getresgid.*%d.*return,success", pid);

	FILE *pipefd = setup(fds, "pc");
	ATF_REQUIRE_EQ(0, getresgid(NULL, NULL, NULL));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(getresgid_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(getresgid_failure);
ATF_TC_HEAD(getresgid_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"getresgid(2) call");
}

ATF_TC_BODY(getresgid_failure, tc)
{
	pid = getpid();
	snprintf(pcregex, 60, "getresgid.*%d.*return,fail.*Bad address", pid);

	FILE *pipefd = setup(fds, "pc");
	/* Failure reason: Invalid address "-1" */
	ATF_REQUIRE_EQ(-1, getresgid((gid_t *)-1, NULL, NULL));
	check_audit(fds, pcregex, pipefd);
}

ATF_TC_CLEANUP(getresgid_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(execve_success);
ATF_TC_HEAD(execve_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"execve(2) call");
}

ATF_TC_BODY(execve_success, tc)
{
	const char *regex = "execve.*sample-argument.*Unknown error: 201";
	FILE *pipefd = setup(fds, "ex");

	ATF_REQUIRE((pid = fork()) != -1);
	if (pid) {
		ATF_REQUIRE(wait(&status) != -1);
		check_audit(fds, regex, pipefd);
	}
	else
		ATF_REQUIRE(execve(bin, arg, NULL) != -1);
}

ATF_TC_CLEANUP(execve_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(execve_failure);
ATF_TC_HEAD(execve_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"execve(2) call");
}

ATF_TC_BODY(execve_failure, tc)
{
	const char *regex = "execve.*return,failure : Bad address";
	FILE *pipefd = setup(fds, "ex");

	ATF_REQUIRE((pid = fork()) != -1);
	if (pid) {
		ATF_REQUIRE(wait(&status) != -1);
		check_audit(fds, regex, pipefd);
	}
	else
		ATF_REQUIRE_EQ(-1, execve(bin, arg, (char *const *)(-1)));
}

ATF_TC_CLEANUP(execve_failure, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fexecve_success);
ATF_TC_HEAD(fexecve_success, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of a successful "
					"fexecve(2) call");
}

ATF_TC_BODY(fexecve_success, tc)
{
	filedesc = open(bin, O_RDONLY | O_EXEC);
	const char *regex = "fexecve.*sample-argument.*Unknown error: 201";
	FILE *pipefd = setup(fds, "pc");

	ATF_REQUIRE((pid = fork()) != -1);
	if (pid) {
		ATF_REQUIRE(wait(&status) != -1);
		check_audit(fds, regex, pipefd);
	}
	else
		ATF_REQUIRE(fexecve(filedesc, arg, NULL) != -1);
}

ATF_TC_CLEANUP(fexecve_success, tc)
{
	cleanup();
}


ATF_TC_WITH_CLEANUP(fexecve_failure);
ATF_TC_HEAD(fexecve_failure, tc)
{
	atf_tc_set_md_var(tc, "descr", "Tests the audit of an unsuccessful "
					"fexecve(2) call");
}

ATF_TC_BODY(fexecve_failure, tc)
{
	filedesc = open(bin, O_RDONLY | O_EXEC);
	const char *regex = "execve.*return,failure : Bad address";
	FILE *pipefd = setup(fds, "pc");

	ATF_REQUIRE((pid = fork()) != -1);
	if (pid) {
		ATF_REQUIRE(wait(&status) != -1);
		check_audit(fds, regex, pipefd);
	}
	else
		ATF_REQUIRE_EQ(-1, fexecve(filedesc, arg, (char *const *)(-1)));
}

ATF_TC_CLEANUP(fexecve_failure, tc)
{
	cleanup();
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, chdir_success);
	ATF_TP_ADD_TC(tp, chdir_failure);
	ATF_TP_ADD_TC(tp, fchdir_success);
	ATF_TP_ADD_TC(tp, fchdir_failure);
	ATF_TP_ADD_TC(tp, chroot_success);
	ATF_TP_ADD_TC(tp, chroot_failure);

	ATF_TP_ADD_TC(tp, umask_success);
	ATF_TP_ADD_TC(tp, setuid_success);
	ATF_TP_ADD_TC(tp, seteuid_success);
	ATF_TP_ADD_TC(tp, setgid_success);
	ATF_TP_ADD_TC(tp, setegid_success);

	ATF_TP_ADD_TC(tp, setreuid_success);
	ATF_TP_ADD_TC(tp, setregid_success);
	ATF_TP_ADD_TC(tp, setresuid_success);
	ATF_TP_ADD_TC(tp, setresgid_success);

	ATF_TP_ADD_TC(tp, getresuid_success);
	ATF_TP_ADD_TC(tp, getresuid_failure);
	ATF_TP_ADD_TC(tp, getresgid_success);
	ATF_TP_ADD_TC(tp, getresgid_failure);

	ATF_TP_ADD_TC(tp, execve_success);
	ATF_TP_ADD_TC(tp, execve_failure);
	ATF_TP_ADD_TC(tp, fexecve_success);
	ATF_TP_ADD_TC(tp, fexecve_failure);

	return (atf_no_error());
}
