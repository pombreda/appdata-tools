/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013-2014 Richard Hughes <richard@hughsie.com>
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

#include <appstream-glib.h>

#define EXIT_CODE_SUCCESS	0
#define EXIT_CODE_USAGE		1
#define EXIT_CODE_WARNINGS	2
#define EXIT_CODE_FAILURE	3

/**
 * gs_string_replace:
 **/
static guint
gs_string_replace (GString *string, const gchar *search, const gchar *replace)
{
	gchar *tmp;
	guint count = 0;
	guint replace_len;
	guint search_len;

	search_len = strlen (search);
	replace_len = strlen (replace);

	do {
		tmp = g_strstr_len (string->str, -1, search);
		if (tmp == NULL)
			goto out;

		/* reallocate the string if required */
		if (search_len > replace_len) {
			g_string_erase (string,
					tmp - string->str,
					search_len - replace_len);
		}
		if (search_len < replace_len) {
			g_string_insert_len (string,
					    tmp - string->str,
					    search,
					    replace_len - search_len);
		}

		/* just memcmp in the new string */
		memcpy (tmp, replace, replace_len);
		count++;
	} while (TRUE);
out:
	return count;
}

/**
 * appdata_validate_format_html:
 **/
static void
appdata_validate_format_html (const gchar *filename, GPtrArray *probs)
{
	AsProblem *problem;
	GString *tmp;
	guint i;

	g_print ("<html>\n");
	g_print ("<head>\n");
	g_print ("<style type=\"text/css\">\n");
	g_print ("body {width: 70%%; font: 12px/20px Arial, Helvetica;}\n");
	g_print ("p {color: #333;}\n");
	g_print ("</style>\n");
	g_print ("<title>AppData Validation Results for %s</title>\n", filename);
	g_print ("</head>\n");
	g_print ("<body>\n");
	if (probs->len == 0) {
		g_print ("<h1>Success!</h1>\n");
		g_print ("<p>%s validated successfully.</p>\n", filename);
	} else {
		g_print ("<h1>Validation failed!</h1>\n");
		g_print ("<p>%s did not validate:</p>\n", filename);
		g_print ("<ul>\n");
		for (i = 0; i < probs->len; i++) {
			problem = g_ptr_array_index (probs, i);
			tmp = g_string_new (as_problem_get_message (problem));
			gs_string_replace (tmp, "&", "&amp;");
			gs_string_replace (tmp, "<", "[");
			gs_string_replace (tmp, ">", "]");
			g_print ("<li>");
			g_print ("%s\n", tmp->str);
			if (as_problem_get_line_number (problem) > 0) {
				g_print (" (line %i)",
					 as_problem_get_line_number (problem));
			}
			g_print ("</li>\n");
			g_string_free (tmp, TRUE);
		}
		g_print ("</ul>\n");
	}
	g_print ("</body>\n");
	g_print ("</html>\n");
}

/**
 * appdata_validate_format_xml:
 **/
static void
appdata_validate_format_xml (const gchar *filename, GPtrArray *probs)
{
	AsProblem *problem;
	GString *tmp;
	guint i;

	g_print ("<results version=\"1\">\n");
	g_print ("  <filename>%s</filename>\n", filename);
	if (probs->len > 0) {
		g_print ("  <problems>\n");
		for (i = 0; i < probs->len; i++) {
			problem = g_ptr_array_index (probs, i);
			tmp = g_string_new (as_problem_get_message (problem));
			gs_string_replace (tmp, "&", "&amp;");
			gs_string_replace (tmp, "<", "");
			gs_string_replace (tmp, ">", "");
			if (as_problem_get_line_number (problem) > 0) {
				g_print ("    <problem type=\"%s\" line=\"%i\">%s</problem>\n",
					 as_problem_kind_to_string (as_problem_get_kind (problem)),
					 as_problem_get_line_number (problem),
					 tmp->str);
			} else {
				g_print ("    <problem type=\"%s\">%s</problem>\n",
					 as_problem_kind_to_string (as_problem_get_kind (problem)),
					 tmp->str);
			}
			g_string_free (tmp, TRUE);
		}
		g_print ("  </problems>\n");
	}
	g_print ("</results>\n");
}

/**
 * appdata_validate_format_text:
 **/
static void
appdata_validate_format_text (const gchar *filename, GPtrArray *probs)
{
	AsProblem *problem;
	const gchar *tmp;
	guint i;
	guint j;

	if (probs->len == 0) {
		/* TRANSLATORS: the file is valid */
		g_print (_("%s validated OK."), filename);
		g_print ("\n");
		return;
	}
	g_print ("%s %i %s\n", filename, probs->len,
		 _("problems detected:"));
	for (i = 0; i < probs->len; i++) {
		problem = g_ptr_array_index (probs, i);
		tmp = as_problem_kind_to_string (as_problem_get_kind (problem));
		g_print ("• %s ", tmp);
		for (j = strlen (tmp); j < 20; j++)
			g_print (" ");
		if (as_problem_get_line_number (problem) > 0) {
			g_print (" : %s [ln:%i]\n",
				 as_problem_get_message (problem),
				 as_problem_get_line_number (problem));
		} else {
			g_print (" : %s\n", as_problem_get_message (problem));
		}
	}
}

/**
 * appdata_validate_and_show_results:
 **/
static gint
appdata_validate_and_show_results (const gchar *filename_original,
				   const gchar *filename,
				   const gchar *output_format,
				   AsAppValidateFlags flags)
{
	AsApp *app;
	GError *error = NULL;
	GPtrArray *problems = NULL;
	const gchar *tmp;
	gboolean ret;
	gint retval = EXIT_CODE_SUCCESS;

	/* scan file for problems */
	app = as_app_new ();
	ret = as_app_parse_file (app, filename,
				 AS_APP_PARSE_FLAG_NONE,
				 &error);
	if (!ret) {
		retval = EXIT_CODE_FAILURE;
		g_print ("Failed: %s\n", error->message);
		g_error_free (error);
		goto out;
	}
	problems = as_app_validate (app, flags, &error);
	if (problems == NULL) {
		retval = EXIT_CODE_FAILURE;
		g_print ("Failed: %s\n", error->message);
		g_error_free (error);
		goto out;
	}
	if (problems->len > 0)
		retval = EXIT_CODE_WARNINGS;

	/* print problems */
	tmp = filename_original != NULL ? filename_original : filename;
	if (g_strcmp0 (output_format, "html") == 0) {
		appdata_validate_format_html (tmp, problems);
	} else if (g_strcmp0 (output_format, "xml") == 0) {
		appdata_validate_format_xml (tmp, problems);
	} else {
		appdata_validate_format_text (tmp, problems);
	}
out:
	g_object_unref (app);
	if (problems)
		g_ptr_array_unref (problems);
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
	AsAppValidateFlags validate_flags = 0;
	gboolean nonet = FALSE;
	gboolean relax = FALSE;
	gboolean ret;
	gboolean strict = FALSE;
	gboolean verbose = FALSE;
	gboolean version = FALSE;
	gchar *filename = NULL;
	gchar *output_format = NULL;
	GError *error = NULL;
	gint i;
	gint retval = EXIT_CODE_SUCCESS;
	gint retval_tmp;
	GOptionContext *context;
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
		{ "filename", '\0', 0, G_OPTION_ARG_STRING, &filename,
			/* TRANSLATORS: this is the --filename argument */
			_("The source filename when using a temporary file"), NULL },
		{ "output-format", '\0', 0, G_OPTION_ARG_STRING, &output_format,
			/* TRANSLATORS: this is the --output-format argument */
			_("The output format [text|html|xml]"), NULL },
		{ NULL}
	};

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

#if !GLIB_CHECK_VERSION(2,36,0)
	g_type_init ();
#endif
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

	/* make more strict or relaxed */
	if (strict)
		validate_flags |= AS_APP_VALIDATE_FLAG_STRICT;
	else if (relax)
		validate_flags |= AS_APP_VALIDATE_FLAG_RELAX;

	/* the user has forced no network mode */
	if (nonet)
		validate_flags |= AS_APP_VALIDATE_FLAG_NO_NETWORK;

	/* validate each file */
	for (i = 1; i < argc; i++) {
		retval_tmp = appdata_validate_and_show_results (filename,
								argv[i],
								output_format,
								validate_flags);
		if (retval_tmp != EXIT_CODE_SUCCESS)
			retval = retval_tmp;
	}
out:
	g_free (filename);
	g_free (output_format);
	g_option_context_free (context);
	return retval;
}
