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

#include <glib.h>

typedef enum {
	AS_PROBLEM_KIND_TAG_DUPLICATED,
	AS_PROBLEM_KIND_TAG_MISSING,
	AS_PROBLEM_KIND_TAG_INVALID,
	AS_PROBLEM_KIND_ATTRIBUTE_MISSING,
	AS_PROBLEM_KIND_ATTRIBUTE_INVALID,
	AS_PROBLEM_KIND_MARKUP_INVALID,
	AS_PROBLEM_KIND_STYLE_INCORRECT,
	AS_PROBLEM_KIND_FILENAME_INVALID,
	AS_PROBLEM_KIND_FAILED_TO_OPEN,
	AS_PROBLEM_KIND_TRANSLATIONS_REQUIRED,
	AS_PROBLEM_KIND_DUPLICATE_DATA,
	AS_PROBLEM_KIND_VALUE_MISSING,
	AS_PROBLEM_KIND_URL_NOT_FOUND,
	AS_PROBLEM_KIND_FILE_INVALID,
	AS_PROBLEM_KIND_ASPECT_RATIO_INCORRECT,
	AS_PROBLEM_KIND_RESOLUTION_INCORRECT,
	AS_PROBLEM_KIND_LAST
} AsProblemKind;

typedef struct {
	AsProblemKind		 kind;
	gchar			*description;
	gint			 line_number;
	gint			 char_number;
} AsProblem;

AsProblem		*as_problem_new			(AsProblemKind	 kind);
void			 as_problem_free		(AsProblem	*problem);
const gchar		*as_problem_kind_to_string	(AsProblemKind	 kind);
