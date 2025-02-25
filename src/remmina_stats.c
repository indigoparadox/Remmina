/*
 * Remmina - The GTK+ Remote Desktop Client
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

/**
 * @file remmina_stats.c
 * @brief Remmina usage statistics module.
 * @author Antenore Gatta and Giovanni Panozzo
 * @date 12 Feb 2018
 *
 * When Remmina starts asks the user if she/he wants to share some usage statistics
 * with the Remmina developers. As per the opt-in model
 * (https://en.wikipedia.org/wiki/Opt-in_email), without the consent of the user,
 * none of these data will be collected.
 * Additionally a user can asks, at any moment, that any data linked to his/her
 * profiles to be deleted, and he/she can change the Remmina settings to stop
 * collecting and sharing usage statistics.
 *
 * All the data are encrypted at client side using RSA, through the OpenSSL
 * libraries, and decrypted offline to maximize security.
 *
 * The following example show which kind of data are collected.
 *
 * @code
 * {
 *
 *    "UID": "P0M20TXN03DWF4-9a1e6da2ad"
 *    "REMMINAVERSION": {
 *        "version": "1.2.0-rcgit-26"
 *        "git_revision": "9c5c4805"
 *        "snap_build": 0
 *    }
 *    "SYSTEM": {
 *        "kernel_name": "Linux"
 *        "kernel_release": "4.14.11-200.fc26.x86_64"
 *        "kernel_arch": "x86_64"
 *        "lsb_distributor": "Fedora"
 *        "lsb_distro_description": "Fedora release 26 (Twenty Six)"
 *        "lsb_distro_release": "26"
 *        "lsb_distro_codename": "TwentySix"
 *        "etc_release": "Fedora release 26 (Twenty Six)"
 *    }
 *    "GTKVERSION": {
 *        "major": 3
 *        "minor": 22
 *        "micro": 21
 *    }
 *    "GTKBACKEND": "X11"
 *    "WINDOWMANAGER": {
 *        "window_manager": "GNOME i3-gnome"
 *    }
 *    "APPINDICATOR": {
 *        "appindicator_supported": 0
 *        "appindicator_compiled": 1
 *        "icon_is_active": 1
 *        "appindicator_type": "AppIndicator on GtkStatusIcon/xembed"
 *    }
 *    "PROFILES": {
 *        "profile_count": 457
 *        "SSH": 431
 *        "NX": 1
 *        "RDP": 7
 *        "TERMINAL": 2
 *        "X2GO": 5
 *        "SFTP": 4
 *        "PYTHON_SIMPLE": 4
 *        "SPICE": 3
 *        "DATE_SSH": "20180209"
 *        "DATE_NX": ""
 *        "DATE_RDP": "20180208"
 *        "DATE_TERMINAL": ""
 *        "DATE_X2GO": ""
 *        "DATE_SFTP": ""
 *        "DATE_PYTHON_SIMPLE": ""
 *        "DATE_SPICE": ""
 *    }
 *    "ENVIRONMENT": {
 *        "language": "en_US.utf8"
 *    }
 *    "ACTIVESECRETPLUGIN": {
 *        "plugin_name": "kwallet"
 *    }
 *    "BUILDHOST": {
 *        "build_host": "local_build"
 *    }
 *
 * }
 * @endcode
 *
 * All of these data are solely transmitted to understand:
 *  - On which type of system Remmina is used
 *	- Operating System
 *	- Architecture (32/64bit)
 *	- Linux distributor or OS vendor
 *  - Desktop Environment type.
 *  - Main library versions installed on the system in use by Remmina.
 *  - Protocols used
 *  - Last time each protocol has been used (globally).
 *
 * @see http://www.remmina.org/wp for more information.
 */


#include "config.h"
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_icon.h"
#include "remmina_log.h"
#include "remmina_pref.h"
#include "remmina_sysinfo.h"
#include "remmina_utils.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_plugin_manager.h"

#ifdef GDK_WINDOWING_WAYLAND
	#include <gdk/gdkwayland.h>
#endif
#ifdef GDK_WINDOWING_X11
	#include <gdk/gdkx.h>
#endif
#include "remmina_stats.h"

struct utsname u;

struct ProfilesData {
	GHashTable *proto_count;
	GHashTable *proto_date;
	const gchar *protocol;          /** Key in the proto_count hash table.*/
	const gchar *pdatestr;          /** Date in string format in the proto_date hash table. */
	gint pcount;
	gchar datestr;
};

static gchar* remmina_stats_gen_random_uuid_prefix()
{
	TRACE_CALL(__func__);
	GRand *rand;
	GTimeVal t;
	gchar *result;
	int i;
	static char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	result = g_malloc0(15);

	g_get_current_time(&t);
	rand = g_rand_new_with_seed((guint32)t.tv_sec ^ (guint32)t.tv_usec);

	for (i = 0; i < 7; i++) {
		result[i] = alpha[g_rand_int_range(rand, 0, sizeof(alpha) - 1)];
	}

	g_rand_set_seed(rand, (guint32)t.tv_usec);
	for (i = 0; i < 7; i++) {
		result[i + 7] = alpha[g_rand_int_range(rand, 0, sizeof(alpha) - 1)];
	}
	g_rand_free(rand);

	return result;
}

JsonNode *remmina_stats_get_uid()
{
	TRACE_CALL(__func__);
	JsonNode *r;
	GChecksum *chs;
	const gchar *uname, *hname;
	const gchar *uid_suffix;
	gchar *uid_prefix;
	gchar *uid;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread
	 */

	if (remmina_pref.periodic_usage_stats_uuid_prefix == NULL || remmina_pref.periodic_usage_stats_uuid_prefix[0] == 0) {
		/* Generate a new UUID_PREFIX for this installation */
		uid_prefix = remmina_stats_gen_random_uuid_prefix();
		if (remmina_pref.periodic_usage_stats_uuid_prefix)
			g_free(remmina_pref.periodic_usage_stats_uuid_prefix);
		remmina_pref.periodic_usage_stats_uuid_prefix = uid_prefix;
		remmina_pref_save();
	}

	uname = g_get_user_name();
	hname = g_get_host_name();
	chs = g_checksum_new(G_CHECKSUM_SHA256);
	g_checksum_update(chs, (const guchar*)uname, strlen(uname));
	g_checksum_update(chs, (const guchar*)hname, strlen(hname));
	uid_suffix = g_checksum_get_string(chs);

	uid = g_strdup_printf("%s-%.10s", remmina_pref.periodic_usage_stats_uuid_prefix, uid_suffix);
	g_checksum_free(chs);

	r = json_node_alloc();
	json_node_init_string(r, uid);

	g_free(uid);

	return r;

}

JsonNode *remmina_stats_get_os_info()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	gchar *kernel_name;
	gchar *kernel_release;
	gchar *kernel_arch;
	gchar *id;
	gchar *description;
	GHashTable *etc_release;
	gchar *release;
	gchar *codename;
	GHashTableIter iter;
	gchar *key, *value;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread */

	b = json_builder_new();
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "kernel_name");
	kernel_name = g_strdup_printf("%s", remmina_utils_get_kernel_name());
	if (!kernel_name || kernel_name[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, kernel_name);
	}
	g_free(kernel_name);

	json_builder_set_member_name(b, "kernel_release");
	kernel_release = g_strdup_printf("%s", remmina_utils_get_kernel_release());
	if (!kernel_release || kernel_release[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, kernel_release);
	}
	g_free(kernel_release);

	json_builder_set_member_name(b, "kernel_arch");
	kernel_arch = g_strdup_printf("%s", remmina_utils_get_kernel_arch());
	if (!kernel_arch || kernel_arch[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, kernel_arch);
	}
	g_free(kernel_arch);

	json_builder_set_member_name(b, "lsb_distributor");
	id = remmina_utils_get_lsb_id();
	if (!id || id[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, id);
	}
	g_free(id);

	json_builder_set_member_name(b, "lsb_distro_description");
	description = remmina_utils_get_lsb_description();
	if (!description || description[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, description);
	}
	g_free(description);

	json_builder_set_member_name(b, "lsb_distro_release");
	release = remmina_utils_get_lsb_release();
	if (!release || release[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, release);
	}
	g_free(release);

	json_builder_set_member_name(b, "lsb_distro_codename");
	codename = remmina_utils_get_lsb_codename();
	if (!codename || codename[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, codename);
	}
	g_free(codename);

	etc_release = remmina_utils_get_etc_release();
	json_builder_set_member_name(b, "etc_release");
	if (etc_release) {
		json_builder_begin_object(b);
		g_hash_table_iter_init (&iter, etc_release);
		while (g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&value)) {
			json_builder_set_member_name(b, key);
			json_builder_add_string_value(b, value);
		}
		json_builder_end_object(b);
		g_hash_table_remove_all(etc_release);
		g_hash_table_unref(etc_release);
	}else {
		json_builder_add_null_value(b);
	}

	/** @todo Add other means to identify a release name/description
	 *        to cover as much OS as possible, like /etc/issue
	 */

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

/**
 * Gets the following user environment:
 *   - Gets the user’s locale (or NULL by default) coresponding to LC_ALL.
 *
 * @return a Json Node structure containg the user’s environment.
 */
JsonNode *remmina_stats_get_user_env()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	gchar *language;

	language = remmina_utils_get_lang();

	b = json_builder_new();
	json_builder_begin_object(b);
	json_builder_set_member_name(b, "language");

	json_builder_add_string_value(b, language);

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;

}

JsonNode *remmina_stats_get_version()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;
	gchar *flatpak_info;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread */

	b = json_builder_new();
	json_builder_begin_object(b);
	json_builder_set_member_name(b, "version");
	json_builder_add_string_value(b, VERSION);
	json_builder_set_member_name(b, "git_revision");
	json_builder_add_string_value(b, REMMINA_GIT_REVISION);
	json_builder_set_member_name(b, "snap_build");
#ifdef SNAP_BUILD
	json_builder_add_int_value(b, 1);
#else
	json_builder_add_int_value(b, 0);
#endif

	/**
	 * Detect if we are running under Flatpak
	 */
	json_builder_set_member_name(b, "flatpak_build");
	/* Flatpak sandbox should contain the file ${XDG_RUNTIME_DIR}/flatpak-info */
	flatpak_info = g_build_filename(g_get_user_runtime_dir(), "flatpak-info", NULL);
	if (g_file_test(flatpak_info, G_FILE_TEST_EXISTS)) {
		json_builder_add_int_value(b, 1);
	} else {
		json_builder_add_int_value(b, 0);
	}
	g_free(flatpak_info);

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

JsonNode *remmina_stats_get_gtk_version()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread
	 */

	b = json_builder_new();
	json_builder_begin_object(b);
	json_builder_set_member_name(b, "major");
	json_builder_add_int_value(b, gtk_get_major_version());
	json_builder_set_member_name(b, "minor");
	json_builder_add_int_value(b, gtk_get_minor_version());
	json_builder_set_member_name(b, "micro");
	json_builder_add_int_value(b, gtk_get_micro_version());
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;

}

JsonNode *remmina_stats_get_gtk_backend()
{
	TRACE_CALL(__func__);
	JsonNode *r;
	GdkDisplay *disp;
	gchar *bkend;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread
	 */

	disp = gdk_display_get_default();

#ifdef GDK_WINDOWING_WAYLAND
	if (GDK_IS_WAYLAND_DISPLAY(disp)) {
		bkend = "Wayland";
	}else
#endif
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(disp)) {
		bkend = "X11";
	}   else
#endif
	bkend = "n/a";

	r = json_node_alloc();
	json_node_init_string(r, bkend);

	return r;

}

JsonNode *remmina_stats_get_wm_name()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;
	gchar *wmver;
	gchar *wmname;

	b = json_builder_new();
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "window_manager");

	/** We try to get the Gnome SHELL version */
	wmver = remmina_sysinfo_get_gnome_shell_version();
	if (!wmver || wmver[0] == '\0') {
		remmina_log_print("Gnome Shell not found\n");
	}else {
		remmina_log_printf("Gnome Shell version: %s\n", wmver);
		json_builder_add_string_value(b, "Gnome Shell");
		json_builder_set_member_name(b, "gnome_shell_ver");
		json_builder_add_string_value(b, wmver);
		goto end;
	}
	g_free(wmver);

	wmname = remmina_sysinfo_get_wm_name();
	if (!wmname || wmname[0] == '\0') {
		/** When everything else fails with set the WM name to NULL **/
		remmina_log_print("Cannot determine the Window Manger name\n");
		json_builder_add_string_value(b, "n/a");
	}else {
		remmina_log_printf("Window Manger names %s\n", wmname);
		json_builder_add_string_value(b, wmname);
	}
	g_free(wmname);

 end:
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

JsonNode *remmina_stats_get_indicator()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;
	gboolean sni;           /** Support for StatusNotifier or AppIndicator */

	b = json_builder_new();
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "appindicator_supported");
	sni = remmina_sysinfo_is_appindicator_available();
	if (sni) {
		/** StatusNotifier/Appindicator supported by desktop */
		json_builder_add_int_value(b, 1);
		json_builder_set_member_name(b, "appindicator_compiled");
#ifdef HAVE_LIBAPPINDICATOR
		/** libappindicator is compiled in remmina. */
		json_builder_add_int_value(b, 1);
#else
		/** Remmina not compiled with -DWITH_APPINDICATOR=on */
		json_builder_add_int_value(b, 0);
#endif
	}
	/** StatusNotifier/Appindicator NOT supported by desktop */
	json_builder_add_int_value(b, 0);
	json_builder_set_member_name(b, "icon_is_active");
	if (remmina_icon_is_available()) {
		/** Remmina icon is active */
		json_builder_add_int_value(b, 1);
		json_builder_set_member_name(b, "appindicator_type");
#ifdef HAVE_LIBAPPINDICATOR
		/** libappindicator fallback to GtkStatusIcon/xembed"); */
		json_builder_add_string_value(b, "AppIndicator on GtkStatusIcon/xembed");
#else
		/** Remmina fallback to GtkStatusIcon/xembed */
		json_builder_add_string_value(b, "Remmina icon on GtkStatusIcon/xembed");
#endif
	}else {
		/** Remmina icon is NOT active */
		json_builder_add_int_value(b, 0);
	}
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

/**
 * Given a remmina file, fills a structure containing profiles keys/value tuples.
 *
 * This is used as a callback function with remmina_file_manager_iterate.
 * @todo Move this in a separate file.
 */
static void remmina_profiles_get_data(RemminaFile *remminafile, gpointer user_data)
{
	TRACE_CALL(__func__);

	gint count = 0;
	gpointer pcount, kpo;
	gpointer pdate, kdo;
	gchar *sday, *smonth, *syear;
	gchar *dday, *dmonth, *dyear;

	GDateTime *ds;          /** Source date -> from profile */
	GDateTime *dd;          /** Destination date -> The date in the pdata structure */

	struct ProfilesData* pdata;
	pdata = (struct ProfilesData*)user_data;

	pdata->protocol = remmina_file_get_string(remminafile, "protocol");
	pdata->pdatestr = remmina_file_get_string(remminafile, "last_success");

	ds = dd = NULL;
	if (pdata->pdatestr && pdata->pdatestr[0] != '\0' && strlen(pdata->pdatestr) >= 6) {
		dyear = g_strdup_printf("%.4s", pdata->pdatestr);
		dmonth = g_strdup_printf("%.2s", pdata->pdatestr + 4);
		dday = g_strdup_printf("%.2s", pdata->pdatestr + 6);
		dd = g_date_time_new_local(g_ascii_strtoll(dyear, NULL, 0),
				g_ascii_strtoll(dmonth, NULL, 0),
				g_ascii_strtoll(dday, NULL, 0), 0, 0, 0.0);
		g_free(dyear);
		g_free(dmonth);
		g_free(dday);
	}


	if (pdata->protocol && pdata->protocol[0] != '\0') {
		if (g_hash_table_lookup_extended(pdata->proto_count, pdata->protocol, &kpo, &pcount)) {
			count = GPOINTER_TO_INT(pcount) + 1;
		}else {
			count = 1;
			g_hash_table_insert(pdata->proto_count, g_strdup(pdata->protocol), GINT_TO_POINTER(count));
		}
		g_hash_table_replace(pdata->proto_count, g_strdup(pdata->protocol), GINT_TO_POINTER(count));
		pdate = NULL;
		if (g_hash_table_lookup_extended(pdata->proto_date, pdata->protocol, &kdo, &pdate)) {

			ds = NULL;
			if (pdate && strlen(pdate) >= 6) {
				syear = g_strdup_printf("%.4s", (char*)pdate);
				smonth = g_strdup_printf("%.2s", (char*)pdate + 4);
				sday = g_strdup_printf("%.2s", (char*)pdate + 6);
				ds = g_date_time_new_local(g_ascii_strtoll(syear, NULL, 0),
						g_ascii_strtoll(smonth, NULL, 0),
						g_ascii_strtoll(sday, NULL, 0), 0, 0, 0.0);
				g_free(syear);
				g_free(smonth);
				g_free(sday);
			}

			/** When both date in the has and in the profile are valid we compare the date */
			if (ds && dd) {
				gint res = g_date_time_compare( ds, dd );
				/** If the date in the hash less than the date in the profile, we take the latter */
				if (res < 0 ) {
					g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), g_strdup(pdata->pdatestr));
				}
			}
			/** If the date in the hash is NOT valid and the date in the profile is valid we keep the latter */
			if (!ds && dd) {
				g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), g_strdup(pdata->pdatestr));
			}
			/** If both date are NULL, we insert NULL for that protocol */
			if ((!ds && !dd) && pdata->pdatestr) {
				g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), NULL);
			}
		}else {
			/** If there is not the protocol in the hash, we add it */
			/** If the date in the profile is not NULL we use it */
			if (pdata->pdatestr) {
				g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), g_strdup(pdata->pdatestr));
			}else {
				/** Otherwise we set it to NULL */
				g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), NULL);
			}
		}
	}
	if (dd)
		g_date_time_unref(dd);
	if (ds)
		g_date_time_unref(ds);
}

/**
 * Add a json member profile_count with a child for each protocol used by the user.
 * Count how many profiles are in use and for each protocol in use counts of how many
 * profiles that uses such protocol.
 *
 * The data can be expressed as follows:
 *
 * | PROTO  | PROF COUNT |
 * |:-------|-----------:|
 * | RDP    |  2560      |
 * | SPICE  |  334       |
 * | SSH    |  1540      |
 * | VNC    |  2         |
 *
 * | PROTO  | LAST USED |
 * |:-------|----------:|
 * | RDP    |  20180129 |
 * | SPICE  |  20171122 |
 * | SSH    |  20180111 |
 *
 * @return a Json Node structure containg the protocol usage statistics.
 *
 */
JsonNode *remmina_stats_get_profiles()
{
	TRACE_CALL(__func__);

	JsonBuilder *b;
	JsonNode *r;
	gchar *s;

	gint profiles_count;
	GHashTableIter pcountiter, pdateiter;
	gpointer pcountkey, pcountvalue;
	gpointer pdatekey, pdatevalue;

	struct ProfilesData *pdata;
	pdata = g_malloc0(sizeof(struct ProfilesData));

	b = json_builder_new();
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "profile_count");

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread */

	pdata->proto_date = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)g_free);
	pdata->proto_count = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, NULL);

	profiles_count = remmina_file_manager_iterate(
		(GFunc)remmina_profiles_get_data,
		(gpointer)pdata);

	json_builder_add_int_value(b, profiles_count);

	g_hash_table_iter_init(&pcountiter, pdata->proto_count);
	while (g_hash_table_iter_next(&pcountiter, &pcountkey, &pcountvalue)) {
		json_builder_set_member_name(b, (gchar*)pcountkey);
		json_builder_add_int_value(b, GPOINTER_TO_INT(pcountvalue));
	}

	g_hash_table_iter_init(&pdateiter, pdata->proto_date);
	while (g_hash_table_iter_next(&pdateiter, &pdatekey, &pdatevalue)) {
		s = g_strdup_printf("DATE_%s", (gchar*)pdatekey);
		json_builder_set_member_name(b, s);
		g_free(s);
		json_builder_add_string_value(b, (gchar*)pdatevalue);
	}

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);

	g_hash_table_remove_all(pdata->proto_date);
	g_hash_table_unref(pdata->proto_date);
	g_hash_table_remove_all(pdata->proto_count);
	g_hash_table_unref(pdata->proto_count);

	g_free(pdata);

	return r;
}

/**
 * Add a json member ACTIVESECRETPLUGIN which shows the current secret plugin in use by remmina.
 *
 * @return a Json Node structure containg the secret plugin in use
 *
 */
JsonNode *remmina_stats_get_secret_plugin()
{
	TRACE_CALL(__func__);

	JsonBuilder *b;
	JsonNode *r;
	RemminaSecretPlugin *secret_plugin;
	secret_plugin = remmina_plugin_manager_get_secret_plugin();

	b = json_builder_new();
	json_builder_begin_object(b);

	if (secret_plugin && secret_plugin->is_service_available) {
		json_builder_set_member_name(b, "plugin_name");
		json_builder_add_string_value(b, secret_plugin->name);
	}
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);

	return r;
}


/**
 * Get all statistics in json format to send periodically to the PHP server.
 * The caller should free the returned buffer with g_free()
 * @warning This function is usually executed on a dedicated thread,
 * not on the main thread.
 * @return a pointer to the JSON string.
 */
JsonNode *remmina_stats_get_all()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *n;

	b = json_builder_new();
	json_builder_begin_object(b);

	n = remmina_stats_get_uid();
	json_builder_set_member_name(b, "UID");
	json_builder_add_value(b, n);

	n = remmina_stats_get_version();
	json_builder_set_member_name(b, "REMMINAVERSION");
	json_builder_add_value(b, n);

	if (uname(&u) == -1)
		g_print("uname:");

	n = remmina_stats_get_os_info();
	json_builder_set_member_name(b, "SYSTEM");
	json_builder_add_value(b, n);

	/**
	 * The section ENVIRONMENT collect all the user’s environment related
	 * settings.
	 */
	n = remmina_stats_get_user_env();
	json_builder_set_member_name(b, "ENVIRONMENT");
	json_builder_add_value(b, n);

	n = remmina_stats_get_gtk_version();
	json_builder_set_member_name(b, "GTKVERSION");
	json_builder_add_value(b, n);

	n = remmina_stats_get_gtk_backend();
	json_builder_set_member_name(b, "GTKBACKEND");
	json_builder_add_value(b, n);

	n = remmina_stats_get_wm_name();
	json_builder_set_member_name(b, "WINDOWMANAGER");
	json_builder_add_value(b, n);

	n = remmina_stats_get_indicator();
	json_builder_set_member_name(b, "APPINDICATOR");
	json_builder_add_value(b, n);

	n = remmina_stats_get_profiles();
	json_builder_set_member_name(b, "PROFILES");
	json_builder_add_value(b, n);

	n = remmina_stats_get_secret_plugin();
	json_builder_set_member_name(b, "ACTIVESECRETPLUGIN");
	json_builder_add_value(b, n);

	json_builder_end_object(b);
	n = json_builder_get_root(b);
	g_object_unref(b);

	return n;

}
