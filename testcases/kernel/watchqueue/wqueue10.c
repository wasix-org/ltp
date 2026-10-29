// SPDX-License-Identifier: GPL-2.0-or-later
/*\
 * Check that the key-event consumer accepts a key GC removal record alongside
 * the expected key event, in either order, without passing removal to the key
 * callback. Feed deterministic records through an ordinary pipe: a real
 * notification pipe cannot be written by userspace.
 */

#define _GNU_SOURCE
#include <fcntl.h>
#include "tst_test.h"
#include "common.h"

static unsigned int seen;

static void saw_key(struct watch_notification *n, size_t len, unsigned int type)
{
	const struct key_notification *key = (const struct key_notification *)n;

	if (type != WATCH_TYPE_KEY_NOTIFY ||
	    len != sizeof(struct key_notification) ||
	    n->subtype != NOTIFY_KEY_CLEARED || key->key_id != 2 || key->aux)
		tst_res(TFAIL, "Unexpected record passed to key callback");
	else
		seen++;
}

static void run(unsigned int n)
{
	struct watch_notification_removal removal = {
		.watch = {
			.type = WATCH_TYPE_META,
			.subtype = WATCH_META_REMOVAL_NOTIFICATION,
			.info = sizeof(removal),
		},
		.id = 1,
	};
	struct key_notification key = {
		.watch = {
			.type = WATCH_TYPE_KEY_NOTIFY,
			.subtype = NOTIFY_KEY_CLEARED,
			.info = sizeof(key),
		},
		.key_id = 2,
	};
	int fds[2];
	unsigned int order;

	removal.watch.info = n & 2 ? sizeof(removal.watch) : sizeof(removal);
	for (order = 0; order < 2; order++) {
		SAFE_PIPE2(fds, n & 1 ? O_DIRECT : 0);
		if (!order)
			SAFE_WRITE(SAFE_WRITE_ALL, fds[1], &removal, removal.watch.info);
		SAFE_WRITE(SAFE_WRITE_ALL, fds[1], &key, sizeof(key));
		if (order)
			SAFE_WRITE(SAFE_WRITE_ALL, fds[1], &removal, removal.watch.info);
		SAFE_CLOSE(fds[1]);

		seen = 0;
		wqueue_key_consumer(fds[0], saw_key);
		TST_EXP_EQ_LI(seen, 1);
		SAFE_CLOSE(fds[0]);
	}
}

static struct tst_test test = {
	.test = run,
	.tcnt = 4,
};
