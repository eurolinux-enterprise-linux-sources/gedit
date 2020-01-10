/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-automatic-spell-checker.h
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* This is a modified version of gtkspell 2.0.2  (gtkspell.sf.net) */
/* gtkspell - a spell-checking addon for GTK's TextView widget
 * Copyright (c) 2002 Evan Martin.
 */

#ifndef __GEDIT_AUTOMATIC_SPELL_CHECKER_H__
#define __GEDIT_AUTOMATIC_SPELL_CHECKER_H__

#include <gedit/gedit-document.h>
#include <gedit/gedit-view.h>

#include "gedit-spell-checker.h"

typedef struct _GeditAutomaticSpellChecker GeditAutomaticSpellChecker;

GeditAutomaticSpellChecker	*gedit_automatic_spell_checker_new (
							GeditDocument 			*doc,
							GeditSpellChecker		*checker);

GeditAutomaticSpellChecker	*gedit_automatic_spell_checker_get_from_document (
							const GeditDocument 		*doc);

void				 gedit_automatic_spell_checker_free (
							GeditAutomaticSpellChecker 	*spell);

void 				 gedit_automatic_spell_checker_attach_view (
							GeditAutomaticSpellChecker 	*spell,
							GeditView 			*view);

void 				 gedit_automatic_spell_checker_detach_view (
							GeditAutomaticSpellChecker 	*spell,
							GeditView 			*view);

void				 gedit_automatic_spell_checker_recheck_all (
							GeditAutomaticSpellChecker 	*spell);

#endif  /* __GEDIT_AUTOMATIC_SPELL_CHECKER_H__ */

/* ex:set ts=8 noet: */
