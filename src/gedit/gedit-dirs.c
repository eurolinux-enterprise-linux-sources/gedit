/*
 * gedit-dirs.c
 * This file is part of gedit
 *
 * Copyright (C) 2008 Ignacio Casal Quinteiro
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-dirs.h"

#ifdef OS_OSX
#include <gtkosxapplication.h>
#endif

static gchar *user_config_dir        = NULL;
static gchar *user_cache_dir         = NULL;
static gchar *user_styles_dir        = NULL;
static gchar *user_plugins_dir       = NULL;
static gchar *gedit_data_dir         = NULL;
static gchar *gedit_locale_dir       = NULL;
static gchar *gedit_lib_dir          = NULL;
static gchar *gedit_plugins_dir      = NULL;
static gchar *gedit_plugins_data_dir = NULL;

void
gedit_dirs_init ()
{
#ifdef G_OS_WIN32
	gchar *win32_dir;

	win32_dir = g_win32_get_package_installation_directory_of_module (NULL);

	gedit_data_dir = g_build_filename (win32_dir,
					   "share",
					   "gedit",
					   NULL);
	gedit_locale_dir = g_build_filename (win32_dir,
					     "share",
					     "locale",
					     NULL);
	gedit_lib_dir = g_build_filename (win32_dir,
					  "lib",
					  "gedit",
					  NULL);

	g_free (win32_dir);
#else /* !G_OS_WIN32 */
#ifdef OS_OSX
	if (gtkosx_application_get_bundle_id () != NULL)
	{
		const gchar *bundle_resource_dir = gtkosx_application_get_resource_path ();

		gedit_data_dir = g_build_filename (bundle_resource_dir,
						   "share",
						   "gedit",
						   NULL);
		gedit_locale_dir = g_build_filename (bundle_resource_dir,
						     "share",
						     "locale",
						     NULL);
		gedit_lib_dir = g_build_filename (bundle_resource_dir,
						  "lib",
						  "gedit",
						  NULL);
	}
#endif /* !OS_OSX */
	if (gedit_data_dir == NULL)
	{
		gedit_data_dir = g_build_filename (DATADIR,
						   "gedit",
						   NULL);
		gedit_locale_dir = g_build_filename (DATADIR,
						     "locale",
						     NULL);
		gedit_lib_dir = g_build_filename (LIBDIR,
						  "gedit",
						  NULL);
	}
#endif /* !G_OS_WIN32 */

	user_cache_dir = g_build_filename (g_get_user_cache_dir (),
					   "gedit",
					   NULL);
	user_config_dir = g_build_filename (g_get_user_config_dir (),
					    "gedit",
					    NULL);
	user_styles_dir = g_build_filename (g_get_user_data_dir (),
					    "gedit",
					    "styles",
					    NULL);
	user_plugins_dir = g_build_filename (g_get_user_data_dir (),
					     "gedit",
					     "plugins",
					     NULL);
	gedit_plugins_dir = g_build_filename (gedit_lib_dir,
					      "plugins",
					      NULL);
	gedit_plugins_data_dir = g_build_filename (gedit_data_dir,
						   "plugins",
						   NULL);
}

void
gedit_dirs_shutdown ()
{
	g_free (user_config_dir);
	g_free (user_cache_dir);
	g_free (user_styles_dir);
	g_free (user_plugins_dir);
	g_free (gedit_data_dir);
	g_free (gedit_locale_dir);
	g_free (gedit_lib_dir);
	g_free (gedit_plugins_dir);
	g_free (gedit_plugins_data_dir);
}

const gchar *
gedit_dirs_get_user_config_dir (void)
{
	return user_config_dir;
}

const gchar *
gedit_dirs_get_user_cache_dir (void)
{
	return user_cache_dir;
}

const gchar *
gedit_dirs_get_user_styles_dir (void)
{
	return user_styles_dir;
}

const gchar *
gedit_dirs_get_user_plugins_dir (void)
{
	return user_plugins_dir;
}

const gchar *
gedit_dirs_get_gedit_data_dir (void)
{
	return gedit_data_dir;
}

const gchar *
gedit_dirs_get_gedit_locale_dir (void)
{
	return gedit_locale_dir;
}

const gchar *
gedit_dirs_get_gedit_lib_dir (void)
{
	return gedit_lib_dir;
}

const gchar *
gedit_dirs_get_gedit_plugins_dir (void)
{
	return gedit_plugins_dir;
}

const gchar *
gedit_dirs_get_gedit_plugins_data_dir (void)
{
	return gedit_plugins_data_dir;
}

gchar *
gedit_dirs_get_ui_file (const gchar *file)
{
	gchar *ui_file;

	g_return_val_if_fail (file != NULL, NULL);

	ui_file = g_build_filename (gedit_dirs_get_gedit_data_dir (),
				    "ui",
				    file,
				    NULL);

	return ui_file;
}

/* ex:set ts=8 noet: */
