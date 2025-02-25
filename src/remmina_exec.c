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
#include "buildflags.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "remmina.h"
#include "remmina_main.h"
#include "remmina_pref.h"
#include "remmina_widget_pool.h"
#include "remmina_unlock.h"
#include "remmina_pref_dialog.h"
#include "remmina_file.h"
#include "remmina_file_editor.h"
#include "rcw.h"
#include "remmina_about.h"
#include "remmina_plugin_manager.h"
#include "remmina_exec.h"
#include "remmina_icon.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_file_manager.h"

#ifdef SNAP_BUILD
#   define ISSNAP "- SNAP Build -"
#else
#   define ISSNAP "-"
#endif

static gboolean cb_closewidget(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	/* The correct way to close a rcw is to send
	 * it a "delete-event" signal. Simply destroying it will not close
	 * all network connections */
	if (REMMINA_IS_CONNECTION_WINDOW(widget))
		return rcw_delete(RCW(widget));
	return TRUE;
}

const gchar* remmina_exec_get_build_config(void)
{
	static const gchar build_config[] =
	    "Build configuration: " BUILD_CONFIG "\n"
	    "Build type:          " BUILD_TYPE "\n"
	    "CFLAGS:              " CFLAGS "\n"
	    "Compiler:            " COMPILER_ID ", " COMPILER_VERSION "\n"
	    "Target architecture: " TARGET_ARCH "\n";
	return build_config;
}

void remmina_exec_exitremmina()
{
	TRACE_CALL(__func__);

	/* Save main window state/position */
	remmina_main_save_before_destroy();

	/* Delete all widgets, main window not included */
	remmina_widget_pool_foreach(cb_closewidget, NULL);

	/* Remove systray menu */
	remmina_icon_destroy();

	/* Exit from Remmina */
	g_application_quit(g_application_get_default());
}

static gboolean disable_rcw_delete_confirm_cb(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *rcw;

	if (REMMINA_IS_CONNECTION_WINDOW(widget)) {
		rcw = (RemminaConnectionWindow*)widget;
		rcw_set_delete_confirm_mode(rcw, RCW_ONDELETE_NOCONFIRM);
	}
	return TRUE;
}

void remmina_application_condexit(RemminaCondExitType why)
{
	TRACE_CALL(__func__);

	/* Exit remmina only if there are no interesting windows left:
	 * no main window, no systray menu, no connection window.
	 * This function is usually called after a disconnection */

	switch (why) {
	case REMMINA_CONDEXIT_ONDISCONNECT:
		// A connection has disconnected, should we exit remmina ?
		if (remmina_widget_pool_count() < 1 && !remmina_main_get_window() && !remmina_icon_is_available())
			remmina_exec_exitremmina();
		break;
	case REMMINA_CONDEXIT_ONMAINWINDELETE:
		/* If we are in Kiosk mode, we just exit */
		if (kioskmode && kioskmode == TRUE)
			remmina_exec_exitremmina();
		// Main window has been deleted
		if (remmina_widget_pool_count() < 1 && !remmina_icon_is_available())
			remmina_exec_exitremmina();
		break;
	case REMMINA_CONDEXIT_ONQUIT:
		// Quit command has been sent from main window or appindicator/systray menu
		// quit means QUIT.
		remmina_widget_pool_foreach(disable_rcw_delete_confirm_cb, NULL);
		remmina_exec_exitremmina();
		break;
	}
}


static void newline_remove(char *s)
{
	char c;
	while((c = *s) != 0 && c != '\r' && c != '\n')
		s++;
	*s = 0;
}

/* used for commandline parameter --update-profile X --set-option Y --set-option Z
 * return a status code for exit()
 */
int remmina_exec_set_setting(gchar *profilefilename, gchar **settings)
{
	RemminaFile *remminafile;
	int i;
	gchar **tk, *value;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	gboolean abort = FALSE;

	remminafile = remmina_file_manager_load_file(profilefilename);

	if (!remminafile) {
		g_print(_("Unable to open profile file %s\n"), profilefilename);
		return 2;
	}

	for(i = 0; settings[i] != NULL && !abort; i++) {
		if (strlen(settings[i]) > 0) {
			tk = g_strsplit(settings[i], "=", 2);
			if (tk[1] == NULL) {
				read = getline(&line, &len, stdin);
				if (read > 0) {
					newline_remove(line);
					value = line;
				} else {
					g_print(_("Error: an extra line of standard input is needed\n"));
					abort = TRUE;
				}
			} else
				value = tk[1];
			remmina_file_set_string(remminafile, tk[0], value);
			g_strfreev(tk);
		}
	}

	if (line) free(line);

	if (!abort) remmina_file_save(remminafile);

	return 0;

}

void remmina_exec_command(RemminaCommandType command, const gchar* data)
{
	TRACE_CALL(__func__);
	gchar* s1;
	gchar* s2;
	GtkWidget* widget;
	GtkWindow* mainwindow;
	GtkDialog* prefdialog;
	RemminaEntryPlugin* plugin;

	mainwindow = remmina_main_get_window();

	switch (command) {
	case REMMINA_COMMAND_MAIN:
		if (mainwindow) {
			gtk_window_present(mainwindow);
			gtk_window_deiconify(GTK_WINDOW(mainwindow));
		}else  {
			widget = remmina_main_new();
			gtk_widget_show(widget);
		}
		break;

	case REMMINA_COMMAND_PREF:
		if (remmina_unlock_new(mainwindow) == 0)
			break;
		prefdialog = remmina_pref_dialog_get_dialog();
		if (prefdialog) {
			gtk_window_present(GTK_WINDOW(prefdialog));
			gtk_window_deiconify(GTK_WINDOW(prefdialog));
		}else  {
			/* Create a new preference dialog */
			widget = GTK_WIDGET(remmina_pref_dialog_new(atoi(data), NULL));
			gtk_widget_show(widget);
		}
		break;

	case REMMINA_COMMAND_NEW:
		s1 = (data ? strchr(data, ',') : NULL);
		if (s1) {
			s1 = g_strdup(data);
			s2 = strchr(s1, ',');
			*s2++ = '\0';
			widget = remmina_file_editor_new_full(s2, s1);
			g_free(s1);
		}else  {
			widget = remmina_file_editor_new_full(NULL, data);
		}
		gtk_widget_show(widget);
		break;

	case REMMINA_COMMAND_CONNECT:
		/** @todo This should be a G_OPTION_ARG_FILENAME_ARRAY (^aay) so that
		 * we can implement multi profile connection:
		 *    https://gitlab.com/Remmina/Remmina/issues/915
		 */
		rcw_open_from_filename(data);
		break;

	case REMMINA_COMMAND_EDIT:
		widget = remmina_file_editor_new_from_filename(data);
		if (widget)
			gtk_widget_show(widget);
		break;

	case REMMINA_COMMAND_ABOUT:
		remmina_about_open(NULL);
		break;

	case REMMINA_COMMAND_VERSION:
		mainwindow = remmina_main_get_window();
		if (mainwindow) {
			remmina_about_open(NULL);
		}else  {
			g_print("%s %s %s (git %s)\n", g_get_application_name(), ISSNAP, VERSION, REMMINA_GIT_REVISION);
			/* As we do not use the "handle-local-options" signal, we have to exit Remmina */
			remmina_exec_command(REMMINA_COMMAND_EXIT, NULL);
		}

		break;

	case REMMINA_COMMAND_FULL_VERSION:
		mainwindow = remmina_main_get_window();
		if (mainwindow) {
			/* Show th widget with the list of plugins and versions */
			remmina_plugin_manager_show(mainwindow);
		}else  {
			g_print("\n%s %s %s (git %s)\n\n", g_get_application_name(), ISSNAP, VERSION, REMMINA_GIT_REVISION);

			remmina_plugin_manager_show_stdout();
			g_print("\n%s\n", remmina_exec_get_build_config());
			remmina_exec_command(REMMINA_COMMAND_EXIT, NULL);
		}

		break;


	case REMMINA_COMMAND_PLUGIN:
		plugin = (RemminaEntryPlugin*)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_ENTRY, data);
		if (plugin) {
			plugin->entry_func();
		}else  {
			widget = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				_("Plugin %s is not registered."), data);
			g_signal_connect(G_OBJECT(widget), "response", G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_widget_show(widget);
			remmina_widget_pool_register(widget);
		}
		break;

	case REMMINA_COMMAND_EXIT:
		remmina_widget_pool_foreach(disable_rcw_delete_confirm_cb, NULL);
		remmina_exec_exitremmina();
		break;

	default:
		break;
	}
}

