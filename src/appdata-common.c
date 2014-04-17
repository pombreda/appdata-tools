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

#include <stdlib.h>
#include <string.h>
#include <libsoup/soup.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "appdata-common.h"
#include "appdata-problem.h"

typedef enum {
	AS_TAG_UNKNOWN,
	AS_TAG_APPLICATION,
	AS_TAG_DESCRIPTION,
	AS_TAG_DESCRIPTION_PARA,
	AS_TAG_DESCRIPTION_UL,
	AS_TAG_DESCRIPTION_UL_LI,
	AS_TAG_ID,
	AS_TAG_METADATA_LICENSE,
	AS_TAG_NAME,
	AS_TAG_SCREENSHOT,
	AS_TAG_SCREENSHOTS,
	AS_TAG_SUMMARY,
	AS_TAG_UPDATE_CONTACT,
	AS_TAG_PROJECT_GROUP,
	AS_TAG_URL,
	AS_TAG_COMPULSORY_FOR_DESKTOP,
	AS_TAG_METADATA,
	AS_TAG_VALUE,
	AS_TAG_LAST
} AppdataSection;

/**
 * as_tag_from_string:
 */
static AppdataSection
as_tag_from_string (const gchar *element_name)
{
	if (g_strcmp0 (element_name, "application") == 0)
		return AS_TAG_APPLICATION;
	if (g_strcmp0 (element_name, "id") == 0)
		return AS_TAG_ID;
	if (g_strcmp0 (element_name, "licence") == 0)
		return AS_TAG_METADATA_LICENSE;
	if (g_strcmp0 (element_name, "metadata_license") == 0)
		return AS_TAG_METADATA_LICENSE;
	if (g_strcmp0 (element_name, "screenshots") == 0)
		return AS_TAG_SCREENSHOTS;
	if (g_strcmp0 (element_name, "screenshot") == 0)
		return AS_TAG_SCREENSHOT;
	if (g_strcmp0 (element_name, "name") == 0)
		return AS_TAG_NAME;
	if (g_strcmp0 (element_name, "summary") == 0)
		return AS_TAG_SUMMARY;
	if (g_strcmp0 (element_name, "url") == 0)
		return AS_TAG_URL;
	if (g_strcmp0 (element_name, "description") == 0)
		return AS_TAG_DESCRIPTION;
	if (g_strcmp0 (element_name, "p") == 0)
		return AS_TAG_DESCRIPTION_PARA;
	if (g_strcmp0 (element_name, "ul") == 0)
		return AS_TAG_DESCRIPTION_UL;
	if (g_strcmp0 (element_name, "li") == 0)
		return AS_TAG_DESCRIPTION_UL_LI;
	if (g_strcmp0 (element_name, "updatecontact") == 0)
		return AS_TAG_UPDATE_CONTACT;
	if (g_strcmp0 (element_name, "project_group") == 0)
		return AS_TAG_PROJECT_GROUP;
	if (g_strcmp0 (element_name, "compulsory_for_desktop") == 0)
		return AS_TAG_COMPULSORY_FOR_DESKTOP;
	if (g_strcmp0 (element_name, "metadata") == 0)
		return AS_TAG_METADATA;
	if (g_strcmp0 (element_name, "value") == 0)
		return AS_TAG_VALUE;
	return AS_TAG_UNKNOWN;
}

/**
 * as_tag_to_string:
 */
static const gchar *
as_tag_to_string (AppdataSection section)
{
	if (section == AS_TAG_APPLICATION)
		return "application";
	if (section == AS_TAG_ID)
		return "id";
	if (section == AS_TAG_METADATA_LICENSE)
		return "metadata_license";
	if (section == AS_TAG_SCREENSHOTS)
		return "screenshots";
	if (section == AS_TAG_SCREENSHOT)
		return "screenshot";
	if (section == AS_TAG_NAME)
		return "name";
	if (section == AS_TAG_SUMMARY)
		return "summary";
	if (section == AS_TAG_URL)
		return "url";
	if (section == AS_TAG_DESCRIPTION)
		return "description";
	if (section == AS_TAG_DESCRIPTION_PARA)
		return "p";
	if (section == AS_TAG_DESCRIPTION_UL)
		return "ul";
	if (section == AS_TAG_DESCRIPTION_UL_LI)
		return "li";
	if (section == AS_TAG_UPDATE_CONTACT)
		return "updatecontact";
	if (section == AS_TAG_PROJECT_GROUP)
		return "project_group";
	if (section == AS_TAG_COMPULSORY_FOR_DESKTOP)
		return "compulsory_for_desktop";
	if (section == AS_TAG_METADATA)
		return "metadata";
	if (section == AS_TAG_VALUE)
		return "value";
	return NULL;
}

typedef enum {
	AS_ID_KIND_UNKNOWN,
	AS_ID_KIND_DESKTOP,
	AS_ID_KIND_FONT,
	AS_ID_KIND_INPUT_METHOD,
	AS_ID_KIND_CODEC,
	AS_ID_KIND_LAST
} AsIdKind;

typedef struct {
	GMarkupParseContext *context;
	AppdataSection	 section;
	gchar		*id;
	AsIdKind	 kind;
	gchar		*name;
	gchar		*summary;
	gchar		*metadata_license;
	gchar		*updatecontact;
	gchar		*project_group;
	gchar		*url;
	GList		*problems;
	guint		 number_paragraphs;
	gboolean	 tag_translated;
	gboolean	 previous_para_was_short;
	gboolean	 seen_application;
	guint		 translations_name;
	guint		 translations_summary;
	guint		 translations_description;
	GKeyFile	*config;
	GPtrArray	*screenshots;
	gboolean	 has_default_screenshot;
	gboolean	 has_xml_header;
	gboolean	 has_copyright_info;
	gboolean	 got_network;
	SoupSession	*session;
	guint		 screenshot_width;
	guint		 screenshot_height;
	guint		 para_chars_before_list;
} AppdataHelper;

/**
 * appdata_add_problem:
 */
static void
appdata_add_problem (AppdataHelper *helper,
		     AsProblemKind kind,
		     const gchar *str)
{
	AsProblem *problem;

	/* add new problem to list */
	problem = as_problem_new (kind);
	problem->description = g_strdup (str);
	g_markup_parse_context_get_position (helper->context,
					     &problem->line_number,
					     &problem->char_number);
	g_debug ("Adding %s '%s', [%i,%i]",
		 as_problem_kind_to_string (kind),
		 str, problem->line_number, problem->char_number);
	helper->problems = g_list_prepend (helper->problems, problem);
}

/**
 * appdata_id_type_from_string:
 */
static gboolean
appdata_id_type_from_string (const gchar *id_type)
{
	if (g_strcmp0 (id_type, "desktop") == 0)
		return AS_ID_KIND_DESKTOP;
	if (g_strcmp0 (id_type, "font") == 0)
		return AS_ID_KIND_FONT;
	if (g_strcmp0 (id_type, "inputmethod") == 0)
		return AS_ID_KIND_INPUT_METHOD;
	if (g_strcmp0 (id_type, "codec") == 0)
		return AS_ID_KIND_CODEC;
	return AS_ID_KIND_UNKNOWN;
}

/**
 * appdata_start_element_fn:
 */
static void
appdata_start_element_fn (GMarkupParseContext *context,
			  const gchar *element_name,
			  const gchar **attribute_names,
			  const gchar **attribute_values,
			  gpointer user_data,
			  GError **error)
{
	AppdataHelper *helper = (AppdataHelper *) user_data;
	AppdataSection new;
	const gchar *tmp;
	gboolean ret;
	gint len;
	guint i;

	new = as_tag_from_string (element_name);

	/* using deprecated names */
	if (g_strcmp0 (element_name, "licence") == 0) {
		ret = g_key_file_get_boolean (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "DeprecatedFailure",
					      NULL);
		if (ret) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
					     "<licence> is deprecated, use "
					     "<metadata_license> instead");
		}
	}

	/* all tags are untranslated until found otherwise */
	helper->tag_translated = FALSE;
	for (i = 0; attribute_names[i] != NULL; i++) {
		if (g_strcmp0 (attribute_names[i], "xml:lang") == 0) {
			tmp = attribute_values[i];
			if (g_strcmp0 (tmp, "C") == 0) {
				appdata_add_problem (helper,
						     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
						     "xml:lang should never be 'C'");
			}
			helper->tag_translated = TRUE;
			if (new == AS_TAG_NAME)
				helper->translations_name++;
			else if (new == AS_TAG_SUMMARY)
				helper->translations_summary++;
			else if (new == AS_TAG_DESCRIPTION_PARA)
				helper->translations_description++;
			break;
		}
	}

	g_debug ("START\t<%s> (%s)",
		 element_name,
		 helper->tag_translated ? "translated" : "untranslated");

	/* unknown -> application */
	if (helper->section == AS_TAG_UNKNOWN) {
		if (new == AS_TAG_APPLICATION) {
			/* valid */
			helper->section = new;
			if (helper->seen_application) {
				appdata_add_problem (helper,
						     AS_PROBLEM_KIND_MARKUP_INVALID,
						     "<application> used more than once");
			}
			helper->seen_application = TRUE;
			return;
		}
		g_set_error (error, 1, 0,
			     "start tag <%s> not allowed from section <%s>",
			     element_name,
			     as_tag_to_string (helper->section));
		return;
	}

	/* application -> various */
	if (helper->section == AS_TAG_APPLICATION) {
		switch (new) {
		case AS_TAG_ID:
			tmp = NULL;
			for (i = 0; attribute_names[i] != NULL; i++) {
				if (g_strcmp0 (attribute_names[i], "type") == 0) {
					tmp = attribute_values[i];
					break;
				}
			}
			if (tmp == NULL) {
				helper->kind = AS_ID_KIND_DESKTOP;
				appdata_add_problem (helper,
						     AS_PROBLEM_KIND_ATTRIBUTE_MISSING,
						     "no type attribute in <id>");
			} else {
				helper->kind = appdata_id_type_from_string (tmp);
				if (helper->kind == AS_ID_KIND_UNKNOWN) {
					appdata_add_problem (helper,
							     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
							     "<id> has invalid type attribute");
				}
			}
			helper->section = new;
			break;
		case AS_TAG_URL:
			tmp = NULL;
			for (i = 0; attribute_names[i] != NULL; i++) {
				if (g_strcmp0 (attribute_names[i], "type") == 0) {
					tmp = attribute_values[i];
					break;
				}
			}
			if (tmp == NULL) {
				appdata_add_problem (helper,
						     AS_PROBLEM_KIND_ATTRIBUTE_MISSING,
						     "no type attribute in <url>");
			}
			if (g_strcmp0 (tmp, "homepage") != 0) {
				appdata_add_problem (helper,
						     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
						     "<url> has invalid type attribute");
			}
			helper->section = new;
			break;
		case AS_TAG_NAME:
		case AS_TAG_SUMMARY:
		case AS_TAG_METADATA_LICENSE:
		case AS_TAG_DESCRIPTION:
		case AS_TAG_SCREENSHOTS:
		case AS_TAG_UPDATE_CONTACT:
		case AS_TAG_COMPULSORY_FOR_DESKTOP:
		case AS_TAG_PROJECT_GROUP:
		case AS_TAG_METADATA:
			/* valid */
			helper->section = new;
			break;
		default:
			g_set_error (error, 1, 0,
				     "start tag <%s> not allowed from section <%s>",
				     element_name,
				     as_tag_to_string (helper->section));
		}
		return;
	}

	/* metadata -> value */
	if (helper->section == AS_TAG_METADATA) {
		switch (new) {
		case AS_TAG_VALUE:
			helper->section = new;
			break;
		default:
			g_set_error (error, 1, 0,
				     "start tag <%s> not allowed from section <%s>",
				     element_name,
				     as_tag_to_string (helper->section));
		}
		return;
	}

	/* description -> p or -> ul */
	if (helper->section == AS_TAG_DESCRIPTION) {
		switch (new) {
		case AS_TAG_DESCRIPTION_PARA:
			helper->section = new;

			/* previous paragraph wasn't long enough */
			if (!helper->tag_translated) {
				if (helper->previous_para_was_short) {
					appdata_add_problem (helper,
							     AS_PROBLEM_KIND_STYLE_INCORRECT,
							     "<p> is too short [p]");
				}
				helper->previous_para_was_short = FALSE;
			}
			break;
		case AS_TAG_DESCRIPTION_UL:
			/* ul without a leading para */
			if (helper->number_paragraphs < 1) {
				appdata_add_problem (helper,
						     AS_PROBLEM_KIND_STYLE_INCORRECT,
						     "<ul> cannot start a description");
			}
			len = g_key_file_get_integer (helper->config,
						      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
						      "LengthParaCharsBeforeList",
						      NULL);
			if (helper->para_chars_before_list != 0 &&
			    helper->para_chars_before_list < (guint) len) {
				appdata_add_problem (helper,
						     AS_PROBLEM_KIND_STYLE_INCORRECT,
						     "Not enough <p> content before <ul>");
			}
			/* we allow the previous paragraph to be short to
			 * introduce the list */
			helper->previous_para_was_short = FALSE;
			helper->para_chars_before_list = 0;
			helper->section = new;
			break;
		default:
			g_set_error (error, 1, 0,
				     "start tag <%s> not allowed from section <%s>",
				     element_name,
				     as_tag_to_string (helper->section));
		}
		return;
	}

	/* ul -> li */
	if (helper->section == AS_TAG_DESCRIPTION_UL) {
		switch (new) {
		case AS_TAG_DESCRIPTION_UL_LI:
			/* valid */
			helper->section = new;
			break;
		default:
			g_set_error (error, 1, 0,
				     "start tag <%s> not allowed from section <%s>",
				     element_name,
				     as_tag_to_string (helper->section));
		}
		return;
	}

	/* unknown -> application */
	if (helper->section == AS_TAG_SCREENSHOTS) {
		if (new == AS_TAG_SCREENSHOT) {
			tmp = NULL;
			helper->screenshot_width = 0;
			helper->screenshot_height = 0;
			for (i = 0; attribute_names[i] != NULL; i++) {
				if (g_strcmp0 (attribute_names[i], "type") == 0)
					tmp = attribute_values[i];
				if (g_strcmp0 (attribute_names[i], "height") == 0)
					helper->screenshot_height = atoi (attribute_values[i]);
				if (g_strcmp0 (attribute_names[i], "width") == 0)
					helper->screenshot_width = atoi (attribute_values[i]);
			}
			if (tmp != NULL && g_strcmp0 (tmp, "default") != 0) {
				appdata_add_problem (helper,
						     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
						     "<screenshot> has unknown type");
			}
			if (g_strcmp0 (tmp, "default") == 0) {
				if (helper->has_default_screenshot) {
					appdata_add_problem (helper,
							     AS_PROBLEM_KIND_MARKUP_INVALID,
							     "<screenshot> has more than one default");
				}
				helper->has_default_screenshot = TRUE;
			}
			helper->section = new;
			return;
		}
		g_set_error (error, 1, 0,
			     "start tag <%s> not allowed from section <%s>",
			     element_name,
			     as_tag_to_string (helper->section));
		return;
	}

	g_set_error (error, 1, 0,
		     "start tag <%s> not allowed from section <%s>",
		     element_name,
		     as_tag_to_string (helper->section));
}

/**
 * appdata_end_element_fn:
 */
static void
appdata_end_element_fn (GMarkupParseContext *context,
			const gchar *element_name,
			gpointer user_data,
			GError **error)
{
	AppdataHelper *helper = (AppdataHelper *) user_data;
	AppdataSection new;

	g_debug ("END\t<%s>", element_name);

	new = as_tag_from_string (element_name);
	if (helper->section == AS_TAG_APPLICATION) {
		if (new == AS_TAG_APPLICATION) {
			/* valid */
			helper->section = AS_TAG_UNKNOWN;
			return;
		}
		g_set_error (error, 1, 0,
			     "end tag <%s> not allowed from section <%s>",
			     element_name,
			     as_tag_to_string (helper->section));
		return;
	}

	/* </id> */
	if (helper->section == AS_TAG_ID &&
	    new == AS_TAG_ID) {
		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </metadata_license> */
	if (helper->section == AS_TAG_METADATA_LICENSE &&
	    new == AS_TAG_METADATA_LICENSE) {
		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </p> */
	if (helper->section == AS_TAG_DESCRIPTION_PARA &&
	    new == AS_TAG_DESCRIPTION_PARA) {
		/* valid */
		helper->section = AS_TAG_DESCRIPTION;
		return;
	}

	/* </li> */
	if (helper->section == AS_TAG_DESCRIPTION_UL_LI &&
	    new == AS_TAG_DESCRIPTION_UL_LI) {
		/* valid */
		helper->section = AS_TAG_DESCRIPTION_UL;
		return;
	}

	/* </ul> */
	if (helper->section == AS_TAG_DESCRIPTION_UL &&
	    new == AS_TAG_DESCRIPTION_UL) {
		/* valid */
		helper->section = AS_TAG_DESCRIPTION;
		return;
	}

	/* </description> */
	if (helper->section == AS_TAG_DESCRIPTION &&
	    new == AS_TAG_DESCRIPTION) {

		/* previous paragraph wasn't long enough */
		if (helper->previous_para_was_short) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<p> is too short");
		}

		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </metadata> */
	if (helper->section == AS_TAG_METADATA &&
	    new == AS_TAG_METADATA) {
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </value> */
	if (helper->section == AS_TAG_VALUE &&
	    new == AS_TAG_VALUE) {
		helper->section = AS_TAG_METADATA;
		return;
	}

	/* </screenshot> */
	if (helper->section == AS_TAG_SCREENSHOT &&
	    new == AS_TAG_SCREENSHOT) {
		/* valid */
		helper->section = AS_TAG_SCREENSHOTS;
		return;
	}

	/* </screenshots> */
	if (helper->section == AS_TAG_SCREENSHOTS &&
	    new == AS_TAG_SCREENSHOTS) {
		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </url> */
	if (helper->section == AS_TAG_URL &&
	    new == AS_TAG_URL) {
		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </name> */
	if (helper->section == AS_TAG_NAME &&
	    new == AS_TAG_NAME) {
		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </summary> */
	if (helper->section == AS_TAG_SUMMARY &&
	    new == AS_TAG_SUMMARY) {
		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </updatecontact> */
	if (helper->section == AS_TAG_UPDATE_CONTACT &&
	    new == AS_TAG_UPDATE_CONTACT) {
		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </compulsory_for_desktop> */
	if (helper->section == AS_TAG_COMPULSORY_FOR_DESKTOP &&
	    new == AS_TAG_COMPULSORY_FOR_DESKTOP) {
		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	/* </project_group> */
	if (helper->section == AS_TAG_PROJECT_GROUP &&
	    new == AS_TAG_PROJECT_GROUP) {
		/* valid */
		helper->section = AS_TAG_APPLICATION;
		return;
	}

	g_set_error (error, 1, 0,
		     "end tag <%s> not allowed from section <%s>",
		     element_name,
		     as_tag_to_string (helper->section));
}

/**
 * appdata_check_id_for_kind:
 */
static gboolean
appdata_check_id_for_kind (const gchar *id, AsIdKind kind)
{
	if (kind == AS_ID_KIND_DESKTOP)
		return g_str_has_suffix (id, ".desktop");
	if (kind == AS_ID_KIND_FONT)
		return g_str_has_suffix (id, ".ttf") ||
			g_str_has_suffix (id, ".otf");
	if (kind == AS_ID_KIND_INPUT_METHOD)
		return g_str_has_suffix (id, ".xml") ||
			g_str_has_suffix (id, ".db");
	if (kind == AS_ID_KIND_CODEC)
		return g_str_has_prefix (id, "gstreamer");
	return FALSE;
}

/**
 * appdata_has_fullstop_ending:
 *
 * Returns %TRUE if the string ends in a full stop, unless the string contains
 * multiple dots. This allows names such as "0 A.D." and summaries to end
 * with "..."
 */
static gboolean
appdata_has_fullstop_ending (const gchar *tmp)
{
	guint cnt = 0;
	guint i;
	guint str_len;

	for (i = 0; tmp[i] != '\0'; i++)
		if (tmp[i] == '.')
			cnt++;
	if (cnt++ > 1)
		return FALSE;
	str_len = strlen (tmp);
	if (str_len == 0)
		return FALSE;
	return tmp[str_len - 1] == '.';
}

/**
 * appdata_screenshot_already_exists:
 */
static gboolean
appdata_screenshot_already_exists (AppdataHelper *helper, const gchar *search)
{
	const gchar *tmp;
	guint i;

	for (i = 0; i < helper->screenshots->len; i++) {
		tmp = g_ptr_array_index (helper->screenshots, i);
		if (g_strcmp0 (tmp, search) == 0)
			return TRUE;
	}
	return FALSE;
}

/**
 * appdata_screenshot_check_remote_url:
 */
static gboolean
appdata_screenshot_check_remote_url (AppdataHelper *helper, const gchar *url)
{
	GError *error = NULL;
	GInputStream *stream = NULL;
	GdkPixbuf *pixbuf = NULL;
	SoupMessage *msg = NULL;
	SoupURI *base_uri = NULL;
	gboolean ret = TRUE;
	gboolean require_aspect;
	gdouble desired_aspect = 1.777777778;
	gdouble screenshot_aspect;
	gint status_code;
	guint policy;
	guint screenshot_height;
	guint screenshot_width;

	/* have we got network access */
	if (!helper->got_network)
		goto out;

	/* GET file */
	g_debug ("checking %s", url);
	base_uri = soup_uri_new (url);
	if (base_uri == NULL) {
		ret = FALSE;
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_URL_NOT_FOUND,
				     "<screenshot> url not valid");
		goto out;
	}
	msg = soup_message_new_from_uri (SOUP_METHOD_GET, base_uri);
	if (msg == NULL) {
		g_warning ("Failed to setup message");
		goto out;
	}

	/* send sync */
	status_code = soup_session_send_message (helper->session, msg);
	if (status_code != SOUP_STATUS_OK) {
		ret = FALSE;
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_URL_NOT_FOUND,
				     "<screenshot> url not found");
		goto out;
	}

	/* check if it's a zero sized file */
	if (msg->response_body->length == 0) {
		ret = FALSE;
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_FILE_INVALID,
				     "<screenshot> url is a zero length file");
		goto out;
	}

	/* create a buffer with the data */
	stream = g_memory_input_stream_new_from_data (msg->response_body->data,
					      msg->response_body->length,
					      NULL);
	if (stream == NULL) {
		ret = FALSE;
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_URL_NOT_FOUND,
				     "<screenshot> failed to load data");
		goto out;
	}

	/* load the image */
	pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, &error);
	if (pixbuf == NULL) {
		ret = FALSE;
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_FILE_INVALID,
				     "<screenshot> failed to load image");
		goto out;
	}

	/* check width matches */
	screenshot_width = gdk_pixbuf_get_width (pixbuf);
	screenshot_height = gdk_pixbuf_get_height (pixbuf);
	if (helper->screenshot_width != 0 &&
	    helper->screenshot_width != screenshot_width) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
				     "<screenshot> width did not match specified");
	}

	/* check height matches */
	if (helper->screenshot_height != 0 &&
	    helper->screenshot_height != screenshot_height) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
				     "<screenshot> height did not match specified");
	}

	/* check size is reasonable */
	policy = g_key_file_get_integer (helper->config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					 "ScreenshotSizeWidthMin", NULL);
	if (screenshot_width < policy) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
				     "<screenshot> width was too small");
	}
	policy = g_key_file_get_integer (helper->config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					 "ScreenshotSizeHeightMin", NULL);
	if (screenshot_height < policy) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
				     "<screenshot> height was too small");
	}
	policy = g_key_file_get_integer (helper->config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					 "ScreenshotSizeWidthMax", NULL);
	if (screenshot_width > policy) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
				     "<screenshot> width was too large");
	}
	policy = g_key_file_get_integer (helper->config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					 "ScreenshotSizeHeightMax", NULL);
	if (screenshot_height > policy) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
				     "<screenshot> height was too large");
	}

	/* check aspect ratio */
	require_aspect = g_key_file_get_boolean (helper->config,
						 APPDATA_TOOLS_VALIDATE_GROUP_NAME,
						 "RequireCorrectAspectRatio", NULL);
	if (require_aspect) {
		desired_aspect = g_key_file_get_double (helper->config,
							APPDATA_TOOLS_VALIDATE_GROUP_NAME,
							"DesiredAspectRatio", NULL);
		screenshot_aspect = (gdouble) screenshot_width / (gdouble) screenshot_height;
		if (ABS (screenshot_aspect - desired_aspect) > 0.1) {
			g_debug ("got aspect %.2f, wanted %.2f",
				 screenshot_aspect, desired_aspect);
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_ASPECT_RATIO_INCORRECT,
					     "<screenshot> aspect ratio was not 16:9");
		}
	}
out:
	if (base_uri != NULL)
		soup_uri_free (base_uri);
	if (msg != NULL)
		g_object_unref (msg);
	if (stream != NULL)
		g_object_unref (stream);
	if (pixbuf != NULL)
		g_object_unref (pixbuf);
	return ret;
}

/**
 * appdata_text_fn:
 */
static void
appdata_text_fn (GMarkupParseContext *context,
		 const gchar *text,
		 gsize text_len,
		 gpointer user_data,
		 GError **error)
{
	AppdataHelper *helper = (AppdataHelper *) user_data;
	gchar **licenses = NULL;
	gchar *temp;
	guint i;
	guint len;
	guint str_len;
	gboolean ret;
	gboolean valid;

	/* ignore translations */
	if (helper->tag_translated)
		return;

	switch (helper->section) {
	case AS_TAG_ID:
		helper->id = g_strstrip (g_strndup (text, text_len));
		ret = appdata_check_id_for_kind (helper->id, helper->kind);
		if (!ret) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_MARKUP_INVALID,
					     "<id> does not have correct extension for kind");
		}
		break;
	case AS_TAG_METADATA_LICENSE:
		if (helper->metadata_license != NULL) {
			g_free (helper->metadata_license);
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TAG_DUPLICATED,
					     "<metadata_license> is duplicated");
		}
		helper->metadata_license = g_strstrip (g_strndup (text, text_len));
		licenses = g_key_file_get_string_list (helper->config,
						       APPDATA_TOOLS_VALIDATE_GROUP_NAME,
						       "AcceptableLicenses",
						       NULL, NULL);
		valid = FALSE;
		for (i = 0; licenses[i] != NULL; i++) {
			if (g_strcmp0 (helper->metadata_license, licenses[i]) == 0) {
				valid = TRUE;
				break;
			}
		}
		if (!valid) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TAG_INVALID,
					     "<metadata_license> is not valid");
		}
		break;
	case AS_TAG_URL:
		if (helper->url != NULL) {
			g_free (helper->url);
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TAG_DUPLICATED,
					     "<url> is duplicated");
		}
		helper->url = g_strstrip (g_strndup (text, text_len));
		if (!g_str_has_prefix (helper->url, "http://") &&
		    !g_str_has_prefix (helper->url, "https://"))
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TAG_INVALID,
					     "<url> does not start with 'http://'");
		break;
	case AS_TAG_UPDATE_CONTACT:
		if (helper->updatecontact != NULL) {
			g_free (helper->updatecontact);
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TAG_DUPLICATED,
					     "<updatecontact> is duplicated");
		}
		helper->updatecontact = g_strstrip (g_strndup (text, text_len));
		if (g_strcmp0 (helper->updatecontact,
			       "someone_who_cares@upstream_project.org") == 0) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TAG_INVALID,
					     "<updatecontact> is still set to a dummy value");
		}
		len = g_key_file_get_integer (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "LengthUpdatecontactMin", NULL);
		if (strlen (helper->updatecontact) < len) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<updatecontact> is too short");
		}
		break;
	case AS_TAG_PROJECT_GROUP:
		if (helper->project_group != NULL) {
			g_free (helper->project_group);
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TAG_DUPLICATED,
					     "<project_group> is duplicated");
		}
		helper->project_group = g_strstrip (g_strndup (text, text_len));
		break;
	case AS_TAG_NAME:
		if (helper->name != NULL) {
			g_free (helper->name);
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TAG_DUPLICATED,
					     "<name> is duplicated");
		}
		helper->name = g_strstrip (g_strndup (text, text_len));
		len = g_key_file_get_integer (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "LengthNameMin", NULL);
		str_len = strlen (helper->name);
		if (str_len < len) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<name> is too short");
		}
		len = g_key_file_get_integer (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "LengthNameMax", NULL);
		if (str_len > len) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<name> is too long");
		}
		if (appdata_has_fullstop_ending (helper->name)) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<name> cannot end in '.'");
		}
		break;
	case AS_TAG_SUMMARY:
		if (helper->summary != NULL) {
			g_free (helper->summary);
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TAG_DUPLICATED,
					     "<summary> is duplicated");
		}
		helper->summary = g_strstrip (g_strndup (text, text_len));
		len = g_key_file_get_integer (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "LengthSummaryMin", NULL);
		str_len = strlen (helper->summary);
		if (str_len < len) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<summary> is too short");
		}
		len = g_key_file_get_integer (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "LengthSummaryMax", NULL);
		if (str_len > len) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<summary> is too long");
		}
		if (appdata_has_fullstop_ending (helper->summary)) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<summary> cannot end in '.'");
		}
		break;
	case AS_TAG_DESCRIPTION_PARA:
		temp = g_strstrip (g_strndup (text, text_len));
		len = g_key_file_get_integer (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "LengthParaMin", NULL);
		if (strlen (temp) < len) {
			/* we don't add the problem now, as we allow a short
			 * paragraph as an introduction to a list */
			helper->previous_para_was_short = TRUE;
		}
		len = g_key_file_get_integer (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "LengthParaMax", NULL);
		str_len = strlen (temp);
		if (str_len > len) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<p> is too long");
		}
		if (g_str_has_prefix (temp, "This application")) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<p> should not start with 'This application'");
		}
		if (temp[str_len - 1] != '.' &&
		    temp[str_len - 1] != '!' &&
		    temp[str_len - 1] != ':') {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<p> does not end in '.|:|!'");
		}
		g_free (temp);
		helper->number_paragraphs++;
		helper->para_chars_before_list += text_len;
		break;
	case AS_TAG_DESCRIPTION_UL_LI:
		temp = g_strstrip (g_strndup (text, text_len));
		len = g_key_file_get_integer (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "LengthListItemMin", NULL);
		str_len = strlen (temp);
		if (str_len < len) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<li> is too short");
		}
		len = g_key_file_get_integer (helper->config,
					      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
					      "LengthListItemMax", NULL);
		if (str_len > len) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<li> is too long");
		}
		if (appdata_has_fullstop_ending (temp)) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_STYLE_INCORRECT,
					     "<li> cannot end in '.'");
		}
		g_free (temp);
		break;
	case AS_TAG_SCREENSHOT:
		temp = g_strstrip (g_strndup (text, text_len));
		if (strlen (temp) == 0) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_VALUE_MISSING,
					     "<screenshot> has no content");
		}
		ret = appdata_screenshot_already_exists (helper, temp);
		if (ret) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_DUPLICATE_DATA,
					     "<screenshot> has duplicated data");
		} else {
			ret = appdata_screenshot_check_remote_url (helper, temp);
			if (ret) {
				g_ptr_array_add (helper->screenshots,
						 g_strdup (temp));
			}
		}
		g_free (temp);
		break;
	default:
		/* ignore */
		break;
	}
	g_free (licenses);
}

/**
 * appdata_passthrough_fn:
 */
static void
appdata_passthrough_fn (GMarkupParseContext *context,
			const gchar *passthrough_text,
			gsize text_len,
			gpointer user_data,
			GError **error)
{
	AppdataHelper *helper = (AppdataHelper *) user_data;
	gchar *temp;

	/* check for XML header */
	temp = g_strndup (passthrough_text, text_len);
	if (g_strcmp0 (temp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>") == 0)
		helper->has_xml_header = TRUE;

	/* check for copyright */
	if (g_str_has_prefix (g_strstrip (temp), "<!-- Copyright"))
		helper->has_copyright_info = TRUE;

	g_free (temp);
}

/**
 * appdata_check_file_for_problems:
 */
GList *
appdata_check_file_for_problems (GKeyFile *config,
				 const gchar *filename)
{
	AppdataHelper *helper = NULL;
	AsProblem *problem;
	gboolean ret;
	gchar *data;
	GError *error = NULL;
	GList *problems = NULL;
	gchar *original_filename = NULL;
	gsize data_len;
	guint len;
	const GMarkupParser parser = {
		appdata_start_element_fn,
		appdata_end_element_fn,
		appdata_text_fn,
		appdata_passthrough_fn,
		NULL };

	g_return_val_if_fail (filename != NULL, NULL);

	/* open file */
	ret = g_file_get_contents (filename, &data, &data_len, &error);
	if (!ret) {
		problem = as_problem_new (AS_PROBLEM_KIND_FAILED_TO_OPEN);
		problem->description = g_strdup (error->message);
		problems = g_list_prepend (problems, problem);
		g_error_free (error);
		goto out;
	}

	/* we set the orignal filename */
	original_filename = g_key_file_get_string (config,
						   APPDATA_TOOLS_VALIDATE_GROUP_NAME,
						   "OriginalFilename", NULL);
	if (original_filename == NULL)
		original_filename = g_strdup (filename);

	/* check file has the correct ending */
	if (!g_str_has_suffix (original_filename, ".appdata.xml")) {
		problem = as_problem_new (AS_PROBLEM_KIND_FILENAME_INVALID);
		problem->description = g_strdup ("incorrect extension, expected '.appdata.xml'");
		problems = g_list_prepend (problems, problem);
	}

	/* parse */
	helper = g_new0 (AppdataHelper, 1);
	helper->problems = problems;
	helper->section = AS_TAG_UNKNOWN;
	helper->config = config;
	helper->screenshots = g_ptr_array_new_with_free_func (g_free);
	helper->got_network = g_key_file_get_boolean (helper->config,
						      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
						      "HasNetworkAccess", NULL);
	if (helper->got_network) {
		helper->session = soup_session_sync_new_with_options (SOUP_SESSION_USER_AGENT,
								      "appdata-validate",
								      SOUP_SESSION_TIMEOUT,
								      5000,
								      NULL);
		if (helper->session == NULL) {
			g_warning ("Failed to setup networking");
			goto out;
		}
		soup_session_add_feature_by_type (helper->session,
						  SOUP_TYPE_PROXY_RESOLVER_DEFAULT);
	}

	helper->context = g_markup_parse_context_new (&parser, 0, helper, NULL);
	ret = g_markup_parse_context_parse (helper->context, data, data_len, &error);
	if (!ret) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_MARKUP_INVALID,
				     error->message);
		g_error_free (error);
		goto out;
	}

	/* check for things that have to exist */
	if (helper->id == NULL) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_TAG_MISSING,
				     "<id> is not present");
	}
	if (!helper->has_xml_header) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_MARKUP_INVALID,
				     "<?xml> header not found");
	}
	ret = g_key_file_get_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
				      "RequireCopyright", NULL);
	if (ret && !helper->has_copyright_info) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_VALUE_MISSING,
				     "<!-- Copyright [year] [name] --> is not present");
	}
	ret = g_key_file_get_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
				      "RequireContactdetails", NULL);
	if (ret && helper->updatecontact == NULL) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_TAG_MISSING,
				     "<updatecontact> is not present");
	}
	ret = g_key_file_get_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
				      "RequireUrl", NULL);
	if (ret && helper->url == NULL) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_TAG_MISSING,
				     "<url> is not present");
	}
	if (helper->metadata_license == NULL) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_TAG_MISSING,
				     "<metadata_license> is not present");
	}
	len = g_key_file_get_integer (helper->config,
				      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
				      "NumberParaMin", NULL);
	if (helper->number_paragraphs < len) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_STYLE_INCORRECT,
				     "Not enough <p> tags for a good description");
	}
	len = g_key_file_get_integer (helper->config,
				      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
				      "NumberParaMax", NULL);
	if (helper->number_paragraphs > len) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_STYLE_INCORRECT,
				     "Too many <p> tags for a good description");
	}
	len = g_key_file_get_integer (helper->config,
				      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
				      "NumberScreenshotsMin", NULL);
	if (helper->screenshots->len < len) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_STYLE_INCORRECT,
				     "Not enough <screenshot> tags");
	}
	len = g_key_file_get_integer (helper->config,
				      APPDATA_TOOLS_VALIDATE_GROUP_NAME,
				      "NumberScreenshotsMax", NULL);
	if (helper->screenshots->len > len) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_STYLE_INCORRECT,
				     "Too many <screenshot> tags");
	}
	if (helper->screenshots->len > 0 && !helper->has_default_screenshot) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_MARKUP_INVALID,
				     "<screenshots> has no default <screenshot>");
	}
	if (helper->summary != NULL && helper->name != NULL &&
	    strlen (helper->summary) < strlen (helper->name)) {
		appdata_add_problem (helper,
				     AS_PROBLEM_KIND_STYLE_INCORRECT,
				     "<summary> is shorter than <name>");
	}
	ret = g_key_file_get_boolean (config, APPDATA_TOOLS_VALIDATE_GROUP_NAME,
				      "RequireTranslations", NULL);
	if (ret) {
		if (helper->name != NULL &&
		    helper->translations_name == 0) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TRANSLATIONS_REQUIRED,
					     "<name> has no translations");
		}
		if (helper->summary != NULL &&
		    helper->translations_summary == 0) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TRANSLATIONS_REQUIRED,
					     "<summary> has no translations");
		}
		if (helper->number_paragraphs > 0 &&
		    helper->translations_description == 0) {
			appdata_add_problem (helper,
					     AS_PROBLEM_KIND_TRANSLATIONS_REQUIRED,
					     "<description> has no translations");
		}
	}
out:
	if (helper != NULL) {
		problems = helper->problems;
		g_free (helper->id);
		g_free (helper->metadata_license);
		g_free (helper->url);
		g_free (helper->name);
		g_free (helper->summary);
		g_free (helper->updatecontact);
		g_free (helper->project_group);
		if (helper->session != NULL)
			g_object_unref (helper->session);
		if (helper->context != NULL)
			g_markup_parse_context_unref (helper->context);
		g_ptr_array_unref (helper->screenshots);
	}
	g_free (helper);
	g_free (original_filename);
	g_free (data);
	return problems;
}
