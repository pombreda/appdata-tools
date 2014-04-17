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

#include <glib/gi18n.h>

#include "appdata-problem.h"

/**
 * as_problem_new:
 */
AsProblem *
as_problem_new (AsProblemKind kind)
{
	AsProblem *problem;
	problem = g_slice_new0 (AsProblem);
	problem->kind = kind;
	return problem;
}

/**
 * as_problem_free:
 */
void
as_problem_free (AsProblem *problem)
{
	g_slice_free (AsProblem, problem);
}

/**
 * as_problem_kind_to_string:
 */
const gchar *
as_problem_kind_to_string (AsProblemKind kind)
{
	if (kind == AS_PROBLEM_KIND_TAG_DUPLICATED)
		return "tag-duplicated";
	if (kind == AS_PROBLEM_KIND_TAG_MISSING)
		return "tag-missing";
	if (kind == AS_PROBLEM_KIND_TAG_INVALID)
		return "tag-invalid";
	if (kind == AS_PROBLEM_KIND_ATTRIBUTE_MISSING)
		return "attribute-missing";
	if (kind == AS_PROBLEM_KIND_ATTRIBUTE_INVALID)
		return "attribute-invalid";
	if (kind == AS_PROBLEM_KIND_MARKUP_INVALID)
		return "markup-invalid";
	if (kind == AS_PROBLEM_KIND_STYLE_INCORRECT)
		return "style-invalid";
	if (kind == AS_PROBLEM_KIND_FILENAME_INVALID)
		return "filename-invalid";
	if (kind == AS_PROBLEM_KIND_FAILED_TO_OPEN)
		return "failed-to-open";
	if (kind == AS_PROBLEM_KIND_TRANSLATIONS_REQUIRED)
		return "translations-required";
	if (kind == AS_PROBLEM_KIND_DUPLICATE_DATA)
		return "duplicate-data";
	if (kind == AS_PROBLEM_KIND_VALUE_MISSING)
		return "value-missing";
	if (kind == AS_PROBLEM_KIND_FILE_INVALID)
		return "file-invalid";
	if (kind == AS_PROBLEM_KIND_ASPECT_RATIO_INCORRECT)
		return "aspect-ratio-invalid";
	if (kind == AS_PROBLEM_KIND_RESOLUTION_INCORRECT)
		return "resolution-invalid";
	return "unknown";
}
