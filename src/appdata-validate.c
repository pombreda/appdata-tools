/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <glib.h>
#include <locale.h>

#include "appdata-common.h"
#include "appdata-problem.h"

#define EXIT_CODE_SUCCESS	0
#define EXIT_CODE_USAGE		1
#define EXIT_CODE_WARNINGS	2

/**
 * appdata_validate_and_show_results:
 **/
static gint
appdata_validate_and_show_results (const gchar *filename, AppdataCheck check)
{
	AppdataProblem *problem;
	const gchar *tmp;
	gint retval;
	GList *l;
	GList *problems = NULL;
	guint i;

	/* scan file for problems */
	problems = appdata_check_file_for_problems (filename, check);
	if (problems == NULL) {
		retval = EXIT_CODE_SUCCESS;
		/* TRANSLATORS: the file is valid */
		g_print (_("%s validated OK."), filename);
		g_print ("\n", filename, _("File validated."));
		goto out;
	}

	/* print problems */
	retval = EXIT_CODE_WARNINGS;
	g_print ("%s %i %s\n", 
		 filename,
		 g_list_length (problems),
		 _("problems detected:"));
	for (l = problems; l != NULL; l = l->next) {
		problem = l->data;
		tmp = appdata_problem_kind_to_string (problem->kind);
		g_print ("• %s ", tmp);
		for (i = strlen (tmp); i < 20; i++)
			g_print (" ");
		g_print (" : %s\n", problem->description);
	}
out:
	g_list_free_full (problems, (GDestroyNotify) appdata_problem_free);
	return retval;
}

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	gint retval_tmp;
	gint retval = EXIT_CODE_SUCCESS;
	guint i;
	AppdataCheck check = APPDATA_CHECK_DEFAULT;

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	if (argc < 2) {
		retval = EXIT_CODE_USAGE;
		g_print ("Usage: %s <file>\n", argv[0]);
		goto out;
	}

	/* relax some checks */
	if (g_getenv ("RELAX") != NULL)
		check += APPDATA_CHECK_ALLOW_MISSING_CONTACTDETAILS;

	/* validate each file */
	for (i = 1; i < argc; i++) {
		retval_tmp = appdata_validate_and_show_results (argv[i], check);
		if (retval_tmp != EXIT_CODE_SUCCESS)
			retval = retval_tmp;
	}
out:
	return retval;
}
