/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "remmina/remmina_trace_calls.h"
#include "remmina_sysinfo.h"

gboolean remmina_sysinfo_is_appindicator_available()
{
	/* Check if we have an appindicator available (which uses
	 * DBUS KDE StatusNotifier)
	 */

	TRACE_CALL(__func__);
	GDBusConnection *con;
	GVariant *v;
	GError *error;
	gboolean available;

	available = FALSE;
	con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	if (con) {
		error = NULL;
		v = g_dbus_connection_call_sync(con,
			"org.kde.StatusNotifierWatcher",
			"/StatusNotifierWatcher",
			"org.freedesktop.DBus.Introspectable",
			"Introspect",
			NULL,
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);
		if (v) {
			available = TRUE;
			g_variant_unref(v);
		}
		g_object_unref(con);
	}
	return available;
}

/**
 * Query DBUS to get gnome shell version.
 * @return the gnome shell version as a string or NULL if error or no gnome shell found.
 * @warning The returned string must be freed with g_free.
 */
gchar *remmina_sysinfo_get_gnome_shell_version()
{
	TRACE_CALL(__func__);
	GDBusConnection *con;
	GDBusProxy *p;
	GVariant *v;
	GError *error;
	gsize sz;
	gchar *ret;

	ret = NULL;

	con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	if (con) {
		error = NULL;
		p = g_dbus_proxy_new_sync(con,
			G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
			NULL,
			"org.gnome.Shell",
			"/org/gnome/Shell",
			"org.gnome.Shell",
			NULL,
			&error);
		if (p) {
			v = g_dbus_proxy_get_cached_property(p, "ShellVersion");
			if (v) {
				if (g_variant_is_of_type(v, G_VARIANT_TYPE_STRING)) {
					ret = g_strdup(g_variant_get_string(v, &sz));
				}
				g_variant_unref(v);
			}
			g_object_unref(p);
		}
		g_object_unref(con);
	}
	return ret;
}

/**
 * Query environment variables to get the Window manager name..
 * @return a string composed by XDG_CURRENT_DESKTOP and GDMSESSION as a string
 * or \0 if nothing has been found.
 * @warning The returned string must be freed with g_free.
 */
gchar *remmina_sysinfo_get_wm_name()
{
	TRACE_CALL(__func__);
	const gchar *xdg_current_desktop;
	const gchar *gdmsession;
	gchar *ret;

	xdg_current_desktop = g_environ_getenv(g_get_environ(), "XDG_CURRENT_DESKTOP");
	gdmsession = g_environ_getenv(g_get_environ(), "GDMSESSION");

	if (!xdg_current_desktop || xdg_current_desktop[0] == '\0') {
		if (!gdmsession || gdmsession[0] == '\0') {
			ret = NULL;
		}else {
			ret = g_strdup_printf("%s", gdmsession);
		}
	}else if (!gdmsession || gdmsession[0] == '\0') {
		ret = g_strdup_printf("%s", xdg_current_desktop);
		return ret;
	}else if (g_strcmp0(xdg_current_desktop,gdmsession) == 0) {
		ret = g_strdup_printf("%s", xdg_current_desktop);
	}else {
		ret = g_strdup_printf("%s %s", xdg_current_desktop, gdmsession);
	}
	return ret;
}

/**
 * Try to get a unique system+user ID to identify this remmina user
 * and avoid some duplicated task, especially on news management
 * @return a string
  * @warning The returned string must be freed with g_free.
 */
gchar *remmina_sysinfo_get_unique_user_id()
{
	unsigned long long uid, sddinodenumber, machineid, id;
	struct stat sb;
	const gchar * const * sdd;
	int fdmid, i;
	gchar *buf;

	uid = (unsigned long long)getuid();

	fdmid = open("/etc/machine-id", O_RDONLY, 0);
	machineid = 0;
	if (fdmid >= 0) {
		/* According to freedesktop specs, machine-id is a 32char hex string
		 * representing a 128bits value. We are fitting it into a 64bit
		 valie (machineid)*/
		buf = g_malloc(32);
		if (read(fdmid, buf, 32) == 32) {
			for(i = 0; i < 32; i++) {
				char c;
				unsigned char b;
				c = buf[i];
				if (c >= '0' && c <= '9')
					b = c - '0';
				else if (c >= 'A' && c <= 'F')
					b = c - 'A' + 10;
				else if (c >= 'a' && c <= 'f')
					b = c - 'a' + 10;
				else
					b = 0;
				machineid = ((machineid >> 60) | (machineid << 4)) ^ b;
			}
		}
		close(fdmid);
		g_free(buf);
	}

	/* Get the 1st inode number of g_get_system_data_dirs() */
	sdd = g_get_system_data_dirs();
	sddinodenumber = 0;
	if (sdd != NULL && sdd[0] != NULL) {
		if ( stat(sdd[0], &sb) == 0 ) {
			sddinodenumber = (unsigned long long)sb.st_ino;
		}
	}

	/* Mix up the three value in a irreversible way */
	id = machineid ^ uid << 32 ^ sddinodenumber;

	/* First 2 chars before '-' are the version number of IDs */
	return g_strdup_printf("01-%llu", id);

}
