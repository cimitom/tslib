/*
 *  tslib/plugins/bsd-raw.c
 *
 *  Copyright (C) 2015 Oleksandr Tymoshenko <gonzo@freebsd.org>
 *
 * This file is placed under the LGPL.  Please see the file
 * COPYING for more details.
 *
 *
 * Read raw x, y, and timestamp from a touchscreen device.
 */
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <limits.h>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/time.h>
#include <sys/types.h>

#include "tslib-private.h"

struct tsc_event
{
	struct timeval	ev_time;
	uint32_t	ev_type;
	uint32_t	ev_value;
};

struct tslib_input {
	struct tslib_module_info module;

	int	current_x;
	int	current_y;
	int	current_p;
};

#define	EVENT_PEN	1
#define	EVENT_ABS_X	2
#define	EVENT_ABS_Y	3

static int ts_input_read(struct tslib_module_info *inf,
			 struct ts_sample *samp, int nr)
{
	struct tslib_input *i = (struct tslib_input *)inf;
	struct tsdev *ts = inf->dev;
	struct tsc_event ev;
	int ret = nr;
	int total = 0;

	while (total < nr) {
		ret = read(ts->fd, &ev, sizeof(struct tsc_event));
		if (ret < (int)sizeof(struct tsc_event)) {
			total = -1;
			break;
		}

		switch (ev.ev_type) {
		case EVENT_PEN:
			if (ev.ev_value) {
				/* Fill out a new complete event */
				samp->x = i->current_x;
				samp->y = i->current_y;
				samp->pressure = 1;
			}
			else {
				samp->x = 0;
				samp->y = 0;
				samp->pressure = 0;
			}
			samp->tv = ev.ev_time;

			samp++;
			total++;
			break;
		case EVENT_ABS_X:
			i->current_x = ev.ev_value;
			break;
		case EVENT_ABS_Y:
			i->current_y = ev.ev_value;
			break;
		default:
			break;
		}
		ret = total;
	}

	return ret;
}

static int ts_input_fini(struct tslib_module_info *inf)
{

	free(inf);
	return 0;
}

static const struct tslib_ops __ts_input_ops = {
	.read	= ts_input_read,
	.fini	= ts_input_fini,
};

TSAPI struct tslib_module_info *input_mod_init(struct tsdev *dev, const char *params)
{
	struct tslib_input *i;

	i = malloc(sizeof(struct tslib_input));
	if (i == NULL)
		return NULL;

	i->module.ops = &__ts_input_ops;
	i->current_x = 0;
	i->current_y = 0;
	i->current_p = 0;

	return &(i->module);
}

#ifndef TSLIB_STATIC_INPUT_MODULE
	TSLIB_MODULE_INIT(input_mod_init);
#endif 
