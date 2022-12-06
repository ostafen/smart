/*
 * SMART: string matching algorithms research tool.
 * Copyright (C) 2012  Simone Faro and Thierry Lecroq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * contact the authors at: faro@dmi.unict.it, thierry.lecroq@univ-rouen.fr
 * download the tool at: http://www.dmi.unict.it/~faro/smart/
 */

#include "timer.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* global variables used for computing preprocessing and searching times */
double _search_time, _pre_time; // searching time and preprocessing time

TIMER timer;

int search(unsigned char *x, int m, unsigned char *y, int n);

#define BEGIN_PREPROCESSING  \
	{                        \
		timer_start(&timer); \
	}
#define BEGIN_SEARCHING      \
	{                        \
		timer_start(&timer); \
	}
#define END_PREPROCESSING                         \
	{                                             \
		timer_stop(&timer);                       \
		_pre_time = timer_elapsed(&timer) * 1000; \
	}

#define END_SEARCHING                                \
	{                                                \
		timer_stop(&timer);                          \
		_search_time = timer_elapsed(&timer) * 1000; \
	}

int internal_search(unsigned char *x, int m, unsigned char *y, int n, double *search_time, double *pre_time)
{
    _search_time = 0.0;
    _pre_time = 0.0;
    int occ = search(x, m, y, n);
	*search_time = _search_time;
	*pre_time = _pre_time;
	return occ;
}