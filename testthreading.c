#include <stdio.h>
#include <stdlib.h>
#include "tinycthread.h"

int loading;
int done_loading;

int func1(void* data)
{
	thrd_sleep(&(struct timespec){ .tv_sec=5}, NULL);

	loading = 0;
	done_loading = 1;
	return 0;
}



int main()
{
	thrd_t thr1;
	loading = 1;
	done_loading = 0;
	if (thrd_success != thrd_create(&thr1, func1, NULL)) {
		puts("couldn't create thread");
	}

	while (1) {


		puts("doing work");
		thrd_sleep(&(struct timespec){ .tv_sec=1}, NULL);

		if (done_loading) {
			puts("Done loading");
			break;
		}
	}

	return 0;
}
