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
 * contact the authors at: faro@dmi.unict.it and thierry.lecroq@univ-rouen.fr
 * download the tool at: http://www.dmi.unict.it/~faro/smart/
 */
#define _GNU_SOURCE // Must be defined as the first include of the program to enable use of CPU pinning macros.

#include "config.h"
#include "parser.h"
#include "select.h"
#include "run.h"

int main(int argc, const char *argv[])
{
	smart_config_t smart_config;
    init_config(&smart_config);

    smart_subcommand_t subcommand;
	parse_args(argc, argv, &subcommand);

	if (!strcmp(subcommand.subcommand, SELECT_COMMAND))
	{
		exec_select((select_command_opts_t *)subcommand.opts, &smart_config);
	}
    else if (!strcmp(subcommand.subcommand, RUN_COMMAND))
	{
		exec_run((run_command_opts_t *)subcommand.opts, &smart_config);
	}

    exit(0);
}