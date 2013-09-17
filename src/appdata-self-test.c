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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib.h>

#include "appdata-common.h"

/**
 * appdata_test_get_data_file:
 **/
static gchar *
appdata_test_get_data_file (const gchar *filename)
{
	gboolean ret;
	gchar *full;
	char full_tmp[PATH_MAX];

	/* check to see if we are being run in the build root */
	full = g_build_filename ("..", "data", filename, NULL);
	ret = g_file_test (full, G_FILE_TEST_EXISTS);
	if (ret)
		goto out;
	g_free (full);

	/* check to see if we are being run in make check */
	full = g_build_filename ("..", "..", "data", filename, NULL);
	ret = g_file_test (full, G_FILE_TEST_EXISTS);
	if (ret)
		goto out;
	g_free (full);
	full = NULL;
out:
	/* canonicalize path */
	if (full != NULL && !g_str_has_prefix (full, "/")) {
		realpath (full, full_tmp);
		g_free (full);
		full = g_strdup (full_tmp);
	}
	return full;
}

static void
print_failures (GList *failures)
{
	GList *l;
	for (l = failures; l != NULL; l = l->next) {
		g_print ("%s\n", l->data);
	}
}

static gboolean
ensure_failure (GList *failures, const gchar *description)
{
	GList *l;
	for (l = failures; l != NULL; l = l->next) {
		if (g_strcmp0 (description, l->data) == 0)
			return TRUE;
	}
	print_failures (failures);
	return FALSE;
}

static void
appdata_success_func (void)
{
	gchar *filename;
	GList *list;

	filename = appdata_test_get_data_file ("success.appdata.xml");
	list = app_data_check_file_for_problems (filename);
	print_failures (list);
	g_assert_cmpint (g_list_length (list), ==, 0);

	g_list_free_full (list, g_free);
	g_free (filename);
}

static void
appdata_wrong_extension_func (void)
{
	gchar *filename;
	GList *list;

	filename = appdata_test_get_data_file ("wrong-extension.xml");
	list = app_data_check_file_for_problems (filename);
	g_assert (ensure_failure (list, "incorrect extension, expected '.appdata.xml'"));
	g_assert (ensure_failure (list, "<id> is not present"));
	g_assert (ensure_failure (list, "<url> is not present"));
	g_assert (ensure_failure (list, "<updatecontact> is not present"));
	g_assert (ensure_failure (list, "<licence> is not present"));
	g_assert_cmpint (g_list_length (list), >, 0);

	g_list_free_full (list, g_free);
	g_free (filename);
}

static void
appdata_broken_func (void)
{
	gchar *filename;
	GList *list;

	filename = appdata_test_get_data_file ("broken.appdata.xml");
	list = app_data_check_file_for_problems (filename);
	g_assert (ensure_failure (list, "<updatecontact> is too short"));
	g_assert (ensure_failure (list, "<url> does not start with 'http://'"));
	g_assert (ensure_failure (list, "<licence> is not valid"));
	g_assert (ensure_failure (list, "<id> does not end in 'desktop'"));
	g_assert (ensure_failure (list, "<id> has invalid type attribute"));
	g_assert (ensure_failure (list, "<p> is too short"));
	g_assert (ensure_failure (list, "Not enough <p> tags for a good description"));
	g_assert (ensure_failure (list, "<li> is too short"));
	g_assert (ensure_failure (list, "<ul> cannot start a description"));
	g_assert (ensure_failure (list, "<url> has invalid type attribute"));
	g_assert (ensure_failure (list, "Not enough <screenshot> tags"));
	g_assert_cmpint (g_list_length (list), >, 0);

	g_list_free_full (list, g_free);
	g_free (filename);
}

int
main (int argc, char **argv)
{
	gint retval;
	g_type_init ();
	g_test_init (&argc, &argv, NULL);

	/* only critical and error are fatal */
	g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);

	/* tests go here */
	g_test_add_func ("/appdata/success", appdata_success_func);
	g_test_add_func ("/appdata/wrong-extension", appdata_wrong_extension_func);
	g_test_add_func ("/appdata/broken", appdata_broken_func);

	/* go go go! */
	retval = g_test_run ();
	return retval;
}

