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
#include <stdio.h>
#include <unistd.h>

#include "appdata-common.h"
#include "appdata-problem.h"

#define EXIT_CODE_SUCCESS	0
#define EXIT_CODE_USAGE		1
#define EXIT_CODE_WARNINGS	2

/**
 * appdata_validate_and_show_results:
 **/
static gint
appdata_validate_and_show_results (GKeyFile *config, const gchar *filename)
{
	AppdataProblem *problem;
	const gchar *tmp;
	gint retval;
	GList *l;
	GList *problems = NULL;
	guint i;

	/* scan file for problems */
	problems = appdata_check_file_for_problems (config, filename);
	if (problems == NULL) {
		retval = EXIT_CODE_SUCCESS;
		/* TRANSLATORS: the file is valid */
		g_print (_("%s validated OK."), filename);
		g_print ("\n");
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
 * appdata_validate_log_ignore_cb:
 **/
static void
appdata_validate_log_ignore_cb (const gchar *log_domain, GLogLevelFlags log_level,
				const gchar *message, gpointer user_data)
{
}

/**
 * appdata_validate_log_handler_cb:
 **/
static void
appdata_validate_log_handler_cb (const gchar *log_domain, GLogLevelFlags log_level,
				 const gchar *message, gpointer user_data)
{
	/* not a console */
	if (isatty (fileno (stdout)) == 0) {
		g_print ("%s\n", message);
		return;
	}

	/* critical is also in red */
	if (log_level == G_LOG_LEVEL_CRITICAL ||
	    log_level == G_LOG_LEVEL_WARNING ||
	    log_level == G_LOG_LEVEL_ERROR) {
		g_print ("%c[%dm%s\n%c[%dm", 0x1B, 31, message, 0x1B, 0);
	} else {
		/* debug in blue */
		g_print ("%c[%dm%s\n%c[%dm", 0x1B, 34, message, 0x1B, 0);
	}
}

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	gboolean nonet = FALSE;
	gboolean relax = FALSE;
	gboolean ret;
	gboolean strict = FALSE;
	gboolean verbose = FALSE;
	gboolean version = FALSE;
	gchar *config_dump = NULL;
	GError *error = NULL;
	gint retval = EXIT_CODE_SUCCESS;
	gint retval_tmp;
	GKeyFile *config = NULL;
	GOptionContext *context;
	guint i;
	const GOptionEntry options[] = {
		{ "relax", 'r', 0, G_OPTION_ARG_NONE, &relax,
			/* TRANSLATORS: this is the --relax argument */
			_("Be less strict when checking files"), NULL },
		{ "strict", 's', 0, G_OPTION_ARG_NONE, &strict,
			/* TRANSLATORS: this is the --relax argument */
			_("Be more strict when checking files"), NULL },
		{ "nonet", '\0', 0, G_OPTION_ARG_NONE, &nonet,
			/* TRANSLATORS: this is the --nonet argument */
			_("Do not use network access"), NULL },
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
			/* TRANSLATORS: this is the --verbose argument */
			_("Show extra debugging information"), NULL },
		{ "version", '\0', 0, G_OPTION_ARG_NONE, &version,
			/* TRANSLATORS: this is the --version argument */
			_("Show the version number and then quit"), NULL },
		{ NULL}
	};

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_type_init ();

	context = g_option_context_new ("AppData Validation Program");
	g_option_context_add_main_entries (context, options, NULL);
	ret = g_option_context_parse (context, &argc, &argv, &error);
	if (!ret) {
		/* TRANSLATORS: this is where the user used unknown command
		 * line switches -- the exact error follows */
		g_print ("%s %s\n", _("Failed to parse command line:"), error->message);
		g_error_free (error);
		goto out;
	}

	/* just show the version */
	if (version) {
		g_print ("%s\n", PACKAGE_VERSION);
		goto out;
	}

	/* verbose? */
	if (verbose) {
		g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);
		g_log_set_handler ("AppDataTools",
				   G_LOG_LEVEL_ERROR |
				   G_LOG_LEVEL_CRITICAL |
				   G_LOG_LEVEL_DEBUG |
				   G_LOG_LEVEL_WARNING,
				   appdata_validate_log_handler_cb, NULL);
	} else {
		/* hide all debugging */
		g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);
		g_log_set_handler ("AppDataTools",
				   G_LOG_LEVEL_DEBUG,
				   appdata_validate_log_ignore_cb, NULL);
	}

	if (argc < 2) {
		retval = EXIT_CODE_USAGE;
		/* TRANSLATORS: this is explaining how to use the tool */
		g_print ("%s %s %s\n", _("Usage:"), argv[0], _("<file>"));
		goto out;
	}

	/* set some config values */
	config = g_key_file_new ();
	if (relax || g_getenv ("RELAX") != NULL) {
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthUpdatecontactMin", 6);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthNameMin", 3);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthNameMax", 100);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthSummaryMin", 8);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthSummaryMax", 200);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthParaMin", 10);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthParaMax", 1000);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthListItemMin", 4);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthListItemMax", 1000);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"NumberParaMin", 1);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"NumberParaMax", 10);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"NumberScreenshotsMin", 0);
		g_key_file_set_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"RequireContactdetails", FALSE);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"NumberScreenshotsMax", 10);
		g_key_file_set_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"RequireUrl", FALSE);
		g_key_file_set_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"HasNetworkAccess", FALSE);
	} else {
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthUpdatecontactMin", 6);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthNameMin", 3);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthNameMax", 30);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthSummaryMin", 8);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthSummaryMax", 100);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthParaMin", 50);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthParaMax", 600);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthListItemMin", 20);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"LengthListItemMax", 100);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"NumberParaMin", 2);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"NumberParaMax", 4);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"NumberScreenshotsMin", 1);
		g_key_file_set_integer (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"NumberScreenshotsMax", 5);
		g_key_file_set_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"RequireContactdetails", TRUE);
		g_key_file_set_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"RequireUrl", TRUE);
		g_key_file_set_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"HasNetworkAccess", TRUE);
	}
	if (strict) {
		g_key_file_set_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"RequireTranslations", TRUE);
	}
	if (nonet) {
		g_key_file_set_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					"HasNetworkAccess", FALSE);
	}
	config_dump = g_key_file_to_data (config, NULL, &error);
	g_debug ("%s", config_dump);

	/* validate each file */
	for (i = 1; i < argc; i++) {
		retval_tmp = appdata_validate_and_show_results (config, argv[i]);
		if (retval_tmp != EXIT_CODE_SUCCESS)
			retval = retval_tmp;
	}
out:
	if (config != NULL)
		g_key_file_free (config);
	g_free (config_dump);
	g_option_context_free (context);
	return retval;
}
