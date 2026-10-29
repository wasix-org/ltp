// SPDX-License-Identifier: GPL-2.0-or-later
/* The library process must survive test-local changes and retain setup buffers. */
#include <signal.h>
#include <unistd.h>
#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"

static pid_t library_pid;
static char *buffer;

static void setup(void)
{
	TST_EXP_EQ_LI(buffer[0], 0);
	buffer[0] = 42;
}

static void run(void)
{
	struct sigaction action;

	TST_EXP_EXPR(getpid() != library_pid);
	SAFE_SIGACTION(SIGUSR1, NULL, &action);
	TST_EXP_EXPR(action.sa_handler == SIG_DFL);
	TST_EXP_EQ_LI(buffer[0], 42);
	buffer[0] = 0;
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.test_variants = 2,
	.bufs = (struct tst_buffers []) {
		{&buffer, .size = 1},
		{}
	},
};

int main(int argc, char *argv[])
{
	library_pid = getpid();
	tst_run_tcases(argc, argv, &test);
}
