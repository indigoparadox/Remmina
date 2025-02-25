/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

#ifdef HAVE_LIBSSH

/* Define this before stdlib.h to have posix_openpt */
#define _XOPEN_SOURCE 600

#define LIBSSH_STATIC 1
#include <libssh/libssh.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PTY_H
#include <pty.h>
#endif
#include "remmina_public.h"
#include "remmina/types.h"
#include "remmina_file.h"
#include "remmina_log.h"
#include "remmina_pref.h"
#include "remmina_ssh.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"


#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif
#endif

#endif


/*-----------------------------------------------------------------------------*
*                           SSH Base                                          *
*-----------------------------------------------------------------------------*/

#define LOCK_SSH(ssh) pthread_mutex_lock(&REMMINA_SSH(ssh)->ssh_mutex);
#define UNLOCK_SSH(ssh) pthread_mutex_unlock(&REMMINA_SSH(ssh)->ssh_mutex);

static const gchar *common_identities[] =
{
	".ssh/id_ed25519",
	".ssh/id_rsa",
	".ssh/id_dsa",
	".ssh/identity",
	NULL
};

gchar *
remmina_ssh_identity_path(const gchar *id)
{
	TRACE_CALL(__func__);
	if (id == NULL) return NULL;
	if (id[0] == '/') return g_strdup(id);
	return g_strdup_printf("%s/%s", g_get_home_dir(), id);
}

gchar *
remmina_ssh_find_identity(void)
{
	TRACE_CALL(__func__);
	gchar *path;
	gint i;

	for (i = 0; common_identities[i]; i++) {
		path = remmina_ssh_identity_path(common_identities[i]);
		if (g_file_test(path, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
			return path;
		g_free(path);
	}
	return NULL;
}

void
remmina_ssh_set_error(RemminaSSH *ssh, const gchar *fmt)
{
	TRACE_CALL(__func__);
	const gchar *err;

	err = ssh_get_error(ssh->session);
	ssh->error = g_strdup_printf(fmt, err);
}

void
remmina_ssh_set_application_error(RemminaSSH *ssh, const gchar *fmt, ...)
{
	TRACE_CALL(__func__);
	va_list args;

	va_start(args, fmt);
	ssh->error = g_strdup_vprintf(fmt, args);
	va_end(args);
}

static gint
remmina_ssh_auth_interactive(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint ret;
	gint n;
	gint i;

	ret = SSH_AUTH_ERROR;
	if (ssh->authenticated) return 1;
	if (ssh->password == NULL) return -1;

	while ((ret = ssh_userauth_kbdint(ssh->session, NULL, NULL)) == SSH_AUTH_INFO) {
		n = ssh_userauth_kbdint_getnprompts(ssh->session);
		for (i = 0; i < n; i++)
			ssh_userauth_kbdint_setanswer(ssh->session, i, ssh->password);
	}

	if (ret != SSH_AUTH_SUCCESS)
		return 0;       // Generic error

	ssh->authenticated = TRUE;
	return 1;
}

static gint
remmina_ssh_auth_password(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint ret;

	ret = SSH_AUTH_ERROR;
	if (ssh->authenticated) return 1;
	if (ssh->password == NULL) return -1;

	ret = ssh_userauth_password(ssh->session, NULL, ssh->password);
	if (ret != SSH_AUTH_SUCCESS) {
		remmina_ssh_set_error(ssh, _("SSH password authentication failed: %s"));
		return 0;
	}

	ssh->authenticated = TRUE;
	return 1;
}

static gint
remmina_ssh_auth_pubkey(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);

	ssh_key key = NULL;
	gchar pubkey[132] = { 0 }; // +".pub"
	gint ret;

	if (ssh->authenticated) return 1;

	if (ssh->privkeyfile == NULL) {
		ssh->error = g_strdup_printf(_("SSH public key authentication failed: %s"),
					     _("SSH Key file not yet set."));
		return 0;
	}

	g_snprintf(pubkey, sizeof(pubkey), "%s.pub", ssh->privkeyfile);

	/*G_FILE_TEST_EXISTS*/
	if (g_file_test(pubkey, G_FILE_TEST_EXISTS)) {
		ret = ssh_pki_import_pubkey_file(pubkey, &key);
		if (ret != SSH_OK) {
			remmina_ssh_set_error(ssh, _("SSH public key cannot be imported: %s"));
			return 0;
		}
		ssh_key_free(key);
	}


	if (ssh_pki_import_privkey_file(ssh->privkeyfile, (ssh->passphrase ? ssh->passphrase : ""),
					NULL, NULL, &key) != SSH_OK) {
		if (ssh->passphrase == NULL || ssh->passphrase[0] == '\0') return -1;

		remmina_ssh_set_error(ssh, _("SSH public key authentication failed: %s"));
		return 0;
	}

	ret = ssh_userauth_publickey(ssh->session, NULL, key);
	ssh_key_free(key);

	if (ret != SSH_AUTH_SUCCESS) {
		remmina_ssh_set_error(ssh, _("SSH public key authentication failed: %s"));
		return 0;
	}

	ssh->authenticated = TRUE;
	return 1;
}

static gint
remmina_ssh_auth_auto_pubkey(RemminaSSH *ssh, RemminaProtocolWidget *gp, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);

	gboolean disablepasswordstoring;
	gboolean save_password;
	gchar *pwd;
	gchar *pwdtype = "ssh_passphrase";
	gint ret;

	if (!ssh->passphrase) {
		disablepasswordstoring = remmina_file_get_int(remminafile, "disablepasswordstoring", FALSE);
		ret = remmina_protocol_widget_panel_authpwd(gp, REMMINA_AUTHPWD_TYPE_SSH_PRIVKEY, !disablepasswordstoring);
		save_password = remmina_protocol_widget_get_savepassword(gp);

		if (ret == GTK_RESPONSE_OK) {
			if (save_password) {
				pwd = remmina_protocol_widget_get_password(gp);
				remmina_file_set_string(remminafile, pwdtype, pwd);
				g_free(pwd);
			}
		} else {
			return -1;
		}
		ssh->passphrase = remmina_protocol_widget_get_password(gp);
	}
	ret = ssh_userauth_publickey_auto(ssh->session, NULL, ssh->passphrase);

	if (ret != SSH_AUTH_SUCCESS) {
		remmina_ssh_set_error(ssh, _("SSH automatic public key authentication failed: %s"));
		return -1;
	}

	ssh->authenticated = TRUE;
	return 1;
}

static gint
remmina_ssh_auth_agent(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint ret;
	ret = ssh_userauth_agent(ssh->session, NULL);

	if (ret != SSH_AUTH_SUCCESS) {
		remmina_ssh_set_error(ssh, _("SSH public key authentication with SSH agent failed: %s"));
		return 0;
	}

	ssh->authenticated = TRUE;
	return 1;
}

static gint
remmina_ssh_auth_gssapi(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint ret;

	if (ssh->authenticated) return 1;

	ret = ssh_userauth_gssapi(ssh->session);

	if (ret != SSH_AUTH_SUCCESS) {
		remmina_ssh_set_error(ssh, _("SSH Kerberos/GSSAPI authentication failed: %s"));
		return 0;
	}

	ssh->authenticated = TRUE;
	return 1;
}

gint
remmina_ssh_auth(RemminaSSH *ssh, const gchar *password, RemminaProtocolWidget *gp, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	gint method;

	/* Check known host again to ensure it’s still the original server when user forks
	 * a new session from existing one */
	if (ssh_is_server_known(ssh->session) != SSH_SERVER_KNOWN_OK) {
		remmina_ssh_set_application_error(ssh, _("SSH public key has changed!"));
		return 0;
	}

	if (password) {
		g_free(ssh->password);
		g_free(ssh->passphrase);
		ssh->password = g_strdup(password);
		ssh->passphrase = g_strdup(password);
	}

	/** @todo Here we should call
	 * gint method;
	 * method = ssh_userauth_list(ssh->session, NULL);
	 * And than test both the method and the option selected by the user
	 */
	method = ssh_userauth_list(ssh->session, NULL);
	switch (ssh->auth) {
	case SSH_AUTH_PASSWORD:
		if (ssh->authenticated)
			return 1;
		if (method & SSH_AUTH_METHOD_PASSWORD)
			if (remmina_ssh_auth_password(ssh) <= 0)
				return -1;              // Re-prompt password
		if (method & SSH_AUTH_METHOD_INTERACTIVE) {
			/* SSH server is requesting us to do interactive auth.
			 * But we just send the saved password */
			if (remmina_ssh_auth_interactive(ssh) <= 0)
				return -1;      // Re-prompt password
		}
		return 1;

	case SSH_AUTH_PUBLICKEY:
		if (method & SSH_AUTH_METHOD_PUBLICKEY)
			return remmina_ssh_auth_pubkey(ssh);

	case SSH_AUTH_AGENT:
		return remmina_ssh_auth_agent(ssh);

	case SSH_AUTH_AUTO_PUBLICKEY:
		/* ssh_agent or none */
		return remmina_ssh_auth_auto_pubkey(ssh, gp, remminafile);

#if 0
	/* Not yet supported by libssh */
	case SSH_AUTH_HOSTBASED:
		if (method & SSH_AUTH_METHOD_HOSTBASED)
			//return remmina_ssh_auth_hostbased;
			return 0;
#endif

	case SSH_AUTH_GSSAPI:
		if (method & SSH_AUTH_METHOD_GSSAPI_MIC)
			return remmina_ssh_auth_gssapi(ssh);

	default:
		return 0;
	}
}

gint
remmina_ssh_auth_gui(RemminaSSH *ssh, RemminaProtocolWidget *gp, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	gchar *keyname;
	gchar *pwdtype;
	gchar *message;
	gint ret;
	size_t len;
	guchar *pubkey;
	ssh_key server_pubkey;
	gboolean disablepasswordstoring;
	gboolean save_password;

	/* Check if the server’s public key is known */
	ret = ssh_is_server_known(ssh->session);
	switch (ret) {
	case SSH_SERVER_KNOWN_OK:
		break;                          /* ok */

	case SSH_SERVER_FILE_NOT_FOUND:
	/*  fallback to SSH_SERVER_NOT_KNOWN behavior */
	case SSH_SERVER_NOT_KNOWN:
	case SSH_SERVER_KNOWN_CHANGED:
	case SSH_SERVER_FOUND_OTHER:
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 6)
		if (ssh_get_server_publickey(ssh->session, &server_pubkey) != SSH_OK) {
			remmina_ssh_set_error(ssh, _("ssh_get_server_publickey() has failed: %s"));
			return 0;
		}
#else
		if (ssh_get_publickey(ssh->session, &server_pubkey) != SSH_OK) {
			remmina_ssh_set_error(ssh, _("ssh_get_publickey() has failed: %s"));
			return 0;
		}
#endif
		if (ssh_get_publickey_hash(server_pubkey, SSH_PUBLICKEY_HASH_MD5, &pubkey, &len) != 0) {
			ssh_key_free(server_pubkey);
			remmina_ssh_set_error(ssh, _("ssh_get_publickey_hash() has failed: %s"));
			return 0;
		}
		ssh_key_free(server_pubkey);
		keyname = ssh_get_hexa(pubkey, len);

		if (ret == SSH_SERVER_NOT_KNOWN || ret == SSH_SERVER_FILE_NOT_FOUND) {
			message = g_strdup_printf("%s\n%s\n\n%s",
						  _("The server is unknown. The public key fingerprint is:"),
						  keyname,
						  _("Do you trust the new public key?"));
		} else {
			message = g_strdup_printf("%s\n%s\n\n%s",
						  _("WARNING: The server has changed its public key. This means either you are under attack,\n"
						    "or the administrator has changed the key. The new public key fingerprint is:"),
						  keyname,
						  _("Do you trust the new public key?"));
		}

		ret = remmina_protocol_widget_panel_question_yesno(gp, message);
		g_free(message);

		ssh_string_free_char(keyname);
		ssh_clean_pubkey_hash(&pubkey);
		if (ret != GTK_RESPONSE_YES) return -1;
		ssh_write_knownhost(ssh->session);
		break;
	case SSH_SERVER_ERROR:
	default:
		remmina_ssh_set_error(ssh, _("SSH known host checking failed: %s"));
		return 0;
	}

	switch (ssh->auth) {
	case SSH_AUTH_PASSWORD:
		keyname = _("SSH password");
		pwdtype = "ssh_password";
		break;
	case SSH_AUTH_PUBLICKEY:
	case SSH_AUTH_AGENT:
	case SSH_AUTH_AUTO_PUBLICKEY:
		keyname = _("SSH private key passphrase");
		pwdtype = "ssh_passphrase";
		break;
	case SSH_AUTH_GSSAPI:
		keyname = _("SSH Kerberos/GSSAPI");
		pwdtype = "kerberos_token";
		break;
	default:
		return FALSE;
	}
	/* Try empty password or existing password/passphrase first */
	ret = remmina_ssh_auth(ssh, remmina_file_get_string(remminafile, pwdtype), gp, remminafile);
	if (ret > 0) return 1;

	/* Requested for a non-empty password */
	if (ret < 0) {
		gchar *pwd;

		disablepasswordstoring = remmina_file_get_int(remminafile, "disablepasswordstoring", FALSE);

		if (g_strcmp0(pwdtype, "ssh_passphrase") == 0)
			ret = remmina_protocol_widget_panel_authpwd(gp, REMMINA_AUTHPWD_TYPE_SSH_PRIVKEY, !disablepasswordstoring);
		else if (g_strcmp0(pwdtype, "ssh_password") == 0)
			ret = remmina_protocol_widget_panel_authuserpwd_ssh_tunnel(gp, FALSE, !disablepasswordstoring);
		else
			ret = remmina_protocol_widget_panel_authuserpwd(gp, FALSE, !disablepasswordstoring);
		save_password = remmina_protocol_widget_get_savepassword(gp);

		if (ret == GTK_RESPONSE_OK) {
			if (save_password) {
				pwd = remmina_protocol_widget_get_password(gp);
				remmina_file_set_string(remminafile, pwdtype, pwd);
				g_free(pwd);
			}
		} else {
			return -1;
		}
		pwd = remmina_protocol_widget_get_password(gp);
		ret = remmina_ssh_auth(ssh, pwd, gp, remminafile);
		g_free(pwd);
	}

	if (ret <= 0)
		return 0;

	return 1;
}

void
remmina_ssh_log_callback(ssh_session session, int priority, const char *message, void *userdata)
{
	TRACE_CALL(__func__);
	remmina_log_printf("[SSH] %s\n", message);
}

gboolean
remmina_ssh_init_session(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint verbosity;
	gint rc;
#ifdef HAVE_NETINET_TCP_H
	socket_t sshsock;
	gint optval;
#endif

	ssh->callback = g_new0(struct ssh_callbacks_struct, 1);

	/* Init & startup the SSH session */
	ssh->session = ssh_new();
	ssh_options_set(ssh->session, SSH_OPTIONS_HOST, ssh->server);
	ssh_options_set(ssh->session, SSH_OPTIONS_PORT, &ssh->port);
	if (*ssh->user != 0)
		ssh_options_set(ssh->session, SSH_OPTIONS_USER, ssh->user);
	if (ssh->privkeyfile && *ssh->privkeyfile != 0) {
		rc = ssh_options_set(ssh->session, SSH_OPTIONS_IDENTITY, ssh->privkeyfile);
		if (rc == 0)
			remmina_log_printf("[SSH] SSH_OPTIONS_IDENTITY has been set to: %s\n", ssh->privkeyfile);
		else
			remmina_log_printf("[SSH] SSH_OPTIONS_IDENTITY is not set, by default identity, id_dsa and id_rsa are checked.\n");
	}

#ifdef SNAP_BUILD
	ssh_options_set(ssh->session, SSH_OPTIONS_SSH_DIR, g_strdup_printf("%s/.ssh", g_getenv("SNAP_USER_COMMON")));
#endif
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_KEY_EXCHANGE, ssh->kex_algorithms);
	if (rc == 0)
		remmina_log_printf("[SSH] SSH_OPTIONS_KEY_EXCHANGE has been set to: %s\n", ssh->kex_algorithms);
	else
		remmina_log_printf("[SSH] SSH_OPTIONS_KEY_EXCHANGE does not have a valid value: %s\n", ssh->kex_algorithms);
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_CIPHERS_C_S, ssh->ciphers);
	if (rc == 0)
		remmina_log_printf("[SSH] SSH_OPTIONS_CIPHERS_C_S has been set to: %s\n", ssh->ciphers);
	else
		remmina_log_printf("[SSH] SSH_OPTIONS_CIPHERS_C_S does not have a valid value: %s\n", ssh->ciphers);
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_HOSTKEYS, ssh->hostkeytypes);
	if (rc == 0)
		remmina_log_printf("[SSH] SSH_OPTIONS_HOSTKEYS has been set to: %s\n", ssh->hostkeytypes);
	else
		remmina_log_printf("[SSH] SSH_OPTIONS_HOSTKEYS does not have a valid value: %s\n", ssh->hostkeytypes);
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_PROXYCOMMAND, ssh->proxycommand);
	if (rc == 0)
		remmina_log_printf("[SSH] SSH_OPTIONS_PROXYCOMMAND has been set to: %s\n", ssh->proxycommand);
	else
		remmina_log_printf("[SSH] SSH_OPTIONS_PROXYCOMMAND does not have a valid value: %s\n", ssh->proxycommand);
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_STRICTHOSTKEYCHECK, &ssh->stricthostkeycheck);
	if (rc == 0)
		remmina_log_printf("[SSH] SSH_OPTIONS_STRICTHOSTKEYCHECK has been set to: %d\n", ssh->stricthostkeycheck);
	else
		remmina_log_printf("[SSH] SSH_OPTIONS_STRICTHOSTKEYCHECK does not have a valid value: %d\n", ssh->stricthostkeycheck);
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_COMPRESSION, ssh->compression);
	if (rc == 0)
		remmina_log_printf("[SSH] SSH_OPTIONS_COMPRESSION has been set to: %s\n", ssh->compression);
	else
		remmina_log_printf("[SSH] SSH_OPTIONS_COMPRESSION does not have a valid value: %s\n", ssh->compression);

	ssh_callbacks_init(ssh->callback);
	if (remmina_log_running()) {
		verbosity = remmina_pref.ssh_loglevel;
		ssh_options_set(ssh->session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
		ssh->callback->log_function = remmina_ssh_log_callback;
		/* Reset libssh legacy userdata. This is a workaround for a libssh bug */
		ssh_set_log_userdata(ssh->session);
	}
	ssh->callback->userdata = ssh;
	ssh_set_callbacks(ssh->session, ssh->callback);

	/* As the latest parse the ~/.ssh/config file */
	if (remmina_pref.ssh_parseconfig)
		ssh_options_parse_config(ssh->session, NULL);

	if (ssh_connect(ssh->session)) {
		remmina_ssh_set_error(ssh, _("Failed to startup SSH session: %s"));
		return FALSE;
	}

#ifdef HAVE_NETINET_TCP_H
	/* Set keepalive on SSH socket, so we can keep firewalls awaken and detect
	 * when we loss the tunnel */
	sshsock = ssh_get_fd(ssh->session);
	if (sshsock >= 0) {
		optval = 1;
		if (setsockopt(sshsock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
			remmina_log_printf("[SSH] TCP KeepAlive not set\n");
		else
			remmina_log_printf("[SSH] TCP KeepAlive enabled\n");

#ifdef TCP_KEEPIDLE
		optval = remmina_pref.ssh_tcp_keepidle;
		if (setsockopt(sshsock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, sizeof(optval)) < 0)
			remmina_log_printf("[SSH] TCP_KEEPIDLE not set\n");
		else
			remmina_log_printf("[SSH] TCP_KEEPIDLE set to %i\n", optval);

#endif
#ifdef TCP_KEEPCNT
		optval = remmina_pref.ssh_tcp_keepcnt;
		if (setsockopt(sshsock, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof(optval)) < 0)
			remmina_log_printf("[SSH] TCP_KEEPCNT not set\n");
		else
			remmina_log_printf("[SSH] TCP_KEEPCNT set to %i\n", optval);

#endif
#ifdef TCP_KEEPINTVL
		optval = remmina_pref.ssh_tcp_keepintvl;
		if (setsockopt(sshsock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof(optval)) < 0)
			remmina_log_printf("[SSH] TCP_KEEPINTVL not set\n");
		else
			remmina_log_printf("[SSH] TCP_KEEPINTVL set to %i\n", optval);

#endif
#ifdef TCP_USER_TIMEOUT
		optval = remmina_pref.ssh_tcp_usrtimeout;
		if (setsockopt(sshsock, IPPROTO_TCP, TCP_USER_TIMEOUT, &optval, sizeof(optval)) < 0)
			remmina_log_printf("[SSH] TCP_USER_TIMEOUT not set\n");
		else
			remmina_log_printf("[SSH] TCP_USER_TIMEOUT set to %i\n", optval);

#endif
	}
#endif

	/* Try the "none" authentication */
	if (ssh_userauth_none(ssh->session, NULL) == SSH_AUTH_SUCCESS)
		ssh->authenticated = TRUE;
	return TRUE;
}

gboolean
remmina_ssh_init_from_file(RemminaSSH *ssh, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	const gchar *ssh_server;
	const gchar *ssh_username;
	const gchar *ssh_privatekey;
	const gchar *server;
	gchar *s;

	ssh->session = NULL;
	ssh->callback = NULL;
	ssh->authenticated = FALSE;
	ssh->error = NULL;
	ssh->passphrase = NULL;
	pthread_mutex_init(&ssh->ssh_mutex, NULL);

	/* Parse the address and port */
	ssh_server = remmina_file_get_string(remminafile, "ssh_server");
	ssh_username = remmina_file_get_string(remminafile, "ssh_username");
	ssh_privatekey = remmina_file_get_string(remminafile, "ssh_privatekey");
	server = remmina_file_get_string(remminafile, "server");
	if (ssh_server) {
		remmina_public_get_server_port(ssh_server, 22, &ssh->server, &ssh->port);
		if (ssh->server[0] == '\0') {
			g_free(ssh->server);
			remmina_public_get_server_port(server, 0, &ssh->server, NULL);
		}
	} else if (server == NULL) {
		ssh->server = g_strdup("localhost");
		ssh->port = 22;
	} else {
		remmina_public_get_server_port(server, 0, &ssh->server, NULL);
		ssh->port = 22;
	}

	ssh->user = g_strdup(ssh_username ? ssh_username : g_get_user_name());
	ssh->password = NULL;
	ssh->auth = remmina_file_get_int(remminafile, "ssh_auth", 0);
	ssh->charset = g_strdup(remmina_file_get_string(remminafile, "ssh_charset"));
	ssh->kex_algorithms = g_strdup(remmina_file_get_string(remminafile, "ssh_kex_algorithms"));
	ssh->ciphers = g_strdup(remmina_file_get_string(remminafile, "ssh_ciphers"));
	ssh->hostkeytypes = g_strdup(remmina_file_get_string(remminafile, "ssh_hostkeytypes"));
	ssh->proxycommand = g_strdup(remmina_file_get_string(remminafile, "ssh_proxycommand"));
	ssh->stricthostkeycheck = remmina_file_get_int(remminafile, "ssh_stricthostkeycheck", 0);
	gint c = remmina_file_get_int(remminafile, "ssh_compression", 0);
	ssh->compression = (c == 1) ? "yes" : "no";

	/* Public/Private keys */
	s = (ssh_privatekey ? g_strdup(ssh_privatekey) : remmina_ssh_find_identity());
	if (s) {
		ssh->privkeyfile = remmina_ssh_identity_path(s);
		g_free(s);
	} else {
		ssh->privkeyfile = NULL;
	}

	return TRUE;
}

static gboolean
remmina_ssh_init_from_ssh(RemminaSSH *ssh, const RemminaSSH *ssh_src)
{
	TRACE_CALL(__func__);
	ssh->session = NULL;
	ssh->authenticated = FALSE;
	ssh->error = NULL;
	pthread_mutex_init(&ssh->ssh_mutex, NULL);

	ssh->server = g_strdup(ssh_src->server);
	ssh->port = ssh_src->port;
	ssh->user = g_strdup(ssh_src->user);
	ssh->auth = ssh_src->auth;
	ssh->password = g_strdup(ssh_src->password);
	ssh->privkeyfile = g_strdup(ssh_src->privkeyfile);
	ssh->charset = g_strdup(ssh_src->charset);
	ssh->proxycommand = g_strdup(ssh_src->proxycommand);
	ssh->kex_algorithms = g_strdup(ssh_src->kex_algorithms);
	ssh->ciphers = g_strdup(ssh_src->ciphers);
	ssh->hostkeytypes = g_strdup(ssh_src->hostkeytypes);
	ssh->compression = ssh_src->compression;

	return TRUE;
}

gchar *
remmina_ssh_convert(RemminaSSH *ssh, const gchar *from)
{
	TRACE_CALL(__func__);
	gchar *to = NULL;

	if (ssh->charset && from)
		to = g_convert(from, -1, "UTF-8", ssh->charset, NULL, NULL, NULL);
	if (!to) to = g_strdup(from);
	return to;
}

gchar *
remmina_ssh_unconvert(RemminaSSH *ssh, const gchar *from)
{
	TRACE_CALL(__func__);
	gchar *to = NULL;

	if (ssh->charset && from)
		to = g_convert(from, -1, ssh->charset, "UTF-8", NULL, NULL, NULL);
	if (!to) to = g_strdup(from);
	return to;
}

void
remmina_ssh_free(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	if (ssh->session) {
		ssh_disconnect(ssh->session);
		ssh_free(ssh->session);
		ssh->session = NULL;
	}
	g_free(ssh->callback);
	g_free(ssh->server);
	g_free(ssh->user);
	g_free(ssh->password);
	g_free(ssh->privkeyfile);
	g_free(ssh->charset);
	g_free(ssh->error);
	pthread_mutex_destroy(&ssh->ssh_mutex);
	g_free(ssh);
}

/*-----------------------------------------------------------------------------*
*                           SSH Tunnel                                        *
*-----------------------------------------------------------------------------*/
struct _RemminaSSHTunnelBuffer {
	gchar * data;
	gchar * ptr;
	ssize_t len;
};

static RemminaSSHTunnelBuffer *
remmina_ssh_tunnel_buffer_new(ssize_t len)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnelBuffer *buffer;

	buffer = g_new(RemminaSSHTunnelBuffer, 1);
	buffer->data = (gchar *)g_malloc(len);
	buffer->ptr = buffer->data;
	buffer->len = len;
	return buffer;
}

static void
remmina_ssh_tunnel_buffer_free(RemminaSSHTunnelBuffer *buffer)
{
	TRACE_CALL(__func__);
	if (buffer) {
		g_free(buffer->data);
		g_free(buffer);
	}
}

RemminaSSHTunnel *
remmina_ssh_tunnel_new_from_file(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnel *tunnel;

	tunnel = g_new(RemminaSSHTunnel, 1);

	remmina_ssh_init_from_file(REMMINA_SSH(tunnel), remminafile);

	tunnel->tunnel_type = -1;
	tunnel->channels = NULL;
	tunnel->sockets = NULL;
	tunnel->socketbuffers = NULL;
	tunnel->num_channels = 0;
	tunnel->max_channels = 0;
	tunnel->x11_channel = NULL;
	tunnel->thread = 0;
	tunnel->running = FALSE;
	tunnel->server_sock = -1;
	tunnel->dest = NULL;
	tunnel->port = 0;
	tunnel->buffer = NULL;
	tunnel->buffer_len = 0;
	tunnel->channels_out = NULL;
	tunnel->remotedisplay = 0;
	tunnel->localdisplay = NULL;
	tunnel->init_func = NULL;
	tunnel->connect_func = NULL;
	tunnel->disconnect_func = NULL;
	tunnel->callback_data = NULL;

	return tunnel;
}

static void
remmina_ssh_tunnel_close_all_channels(RemminaSSHTunnel *tunnel)
{
	TRACE_CALL(__func__);
	int i;

	for (i = 0; i < tunnel->num_channels; i++) {
		close(tunnel->sockets[i]);
		remmina_ssh_tunnel_buffer_free(tunnel->socketbuffers[i]);
		ssh_channel_close(tunnel->channels[i]);
		ssh_channel_send_eof(tunnel->channels[i]);
		ssh_channel_free(tunnel->channels[i]);
	}

	g_free(tunnel->channels);
	tunnel->channels = NULL;
	g_free(tunnel->sockets);
	tunnel->sockets = NULL;
	g_free(tunnel->socketbuffers);
	tunnel->socketbuffers = NULL;

	tunnel->num_channels = 0;
	tunnel->max_channels = 0;

	if (tunnel->x11_channel) {
		ssh_channel_close(tunnel->x11_channel);
		ssh_channel_send_eof(tunnel->x11_channel);
		ssh_channel_free(tunnel->x11_channel);
		tunnel->x11_channel = NULL;
	}
}

static void
remmina_ssh_tunnel_remove_channel(RemminaSSHTunnel *tunnel, gint n)
{
	TRACE_CALL(__func__);
	ssh_channel_close(tunnel->channels[n]);
	ssh_channel_send_eof(tunnel->channels[n]);
	ssh_channel_free(tunnel->channels[n]);
	close(tunnel->sockets[n]);
	remmina_ssh_tunnel_buffer_free(tunnel->socketbuffers[n]);
	tunnel->num_channels--;
	tunnel->channels[n] = tunnel->channels[tunnel->num_channels];
	tunnel->channels[tunnel->num_channels] = NULL;
	tunnel->sockets[n] = tunnel->sockets[tunnel->num_channels];
	tunnel->socketbuffers[n] = tunnel->socketbuffers[tunnel->num_channels];
}

/* Register the new channel/socket pair */
static void
remmina_ssh_tunnel_add_channel(RemminaSSHTunnel *tunnel, ssh_channel channel, gint sock)
{
	TRACE_CALL(__func__);
	gint flags;
	gint i;

	i = tunnel->num_channels++;
	if (tunnel->num_channels > tunnel->max_channels) {
		/* Allocate an extra NULL pointer in channels for ssh_select */
		tunnel->channels = (ssh_channel *)g_realloc(tunnel->channels,
							    sizeof(ssh_channel) * (tunnel->num_channels + 1));
		tunnel->sockets = (gint *)g_realloc(tunnel->sockets,
						    sizeof(gint) * tunnel->num_channels);
		tunnel->socketbuffers = (RemminaSSHTunnelBuffer **)g_realloc(tunnel->socketbuffers,
									     sizeof(RemminaSSHTunnelBuffer *) * tunnel->num_channels);
		tunnel->max_channels = tunnel->num_channels;

		tunnel->channels_out = (ssh_channel *)g_realloc(tunnel->channels_out,
								sizeof(ssh_channel) * (tunnel->num_channels + 1));
	}
	tunnel->channels[i] = channel;
	tunnel->channels[i + 1] = NULL;
	tunnel->sockets[i] = sock;
	tunnel->socketbuffers[i] = NULL;

	flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

static int
remmina_ssh_tunnel_accept_local_connection(RemminaSSHTunnel *tunnel, gboolean blocking)
{
	gint sock, sock_flags;

	sock_flags = fcntl(tunnel->server_sock, F_GETFL, 0);
	if (blocking)
		sock_flags &= ~O_NONBLOCK;
	else
		sock_flags |= O_NONBLOCK;
	fcntl(tunnel->server_sock, F_SETFL, sock_flags);

	/* Accept a local connection */
	sock = accept(tunnel->server_sock, NULL, NULL);
	if (sock < 0)
		REMMINA_SSH(tunnel)->error = g_strdup("Failed to accept local socket");

	return sock;
}

static ssh_channel
remmina_ssh_tunnel_create_forward_channel(RemminaSSHTunnel *tunnel)
{
	ssh_channel channel = NULL;

	channel = ssh_channel_new(tunnel->ssh.session);
	if (!channel) {
		remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Failed to create channel: %s"));
		return NULL;
	}

	/* Request the SSH server to connect to the destination */
	g_debug("SSH tunnel destination is %s", tunnel->dest);
	if (ssh_channel_open_forward(channel, tunnel->dest, tunnel->port, "127.0.0.1", 0) != SSH_OK) {
		ssh_channel_close(channel);
		ssh_channel_send_eof(channel);
		ssh_channel_free(channel);
		remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Failed to connect to the SSH tunnel destination: %s"));
		return NULL;
	}

	return channel;
}

static gpointer
remmina_ssh_tunnel_main_thread_proc(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnel *tunnel = (RemminaSSHTunnel *)data;
	gchar *ptr;
	ssize_t len = 0, lenw = 0;
	fd_set set;
	struct timeval timeout;
	GTimeVal t1, t2;
	glong diff;
	ssh_channel channel = NULL;
	gboolean first = TRUE;
	gboolean disconnected;
	gint sock;
	gint maxfd;
	gint i;
	gint ret;
	struct sockaddr_in sin;

	g_get_current_time(&t1);
	t2 = t1;

	switch (tunnel->tunnel_type) {
	case REMMINA_SSH_TUNNEL_OPEN:
		sock = remmina_ssh_tunnel_accept_local_connection(tunnel, TRUE);
		if (sock < 0) {
			tunnel->thread = 0;
			return NULL;
		}

		channel = remmina_ssh_tunnel_create_forward_channel(tunnel);
		if (!tunnel) {
			close(sock);
			tunnel->thread = 0;
			return NULL;
		}

		remmina_ssh_tunnel_add_channel(tunnel, channel, sock);
		break;

	case REMMINA_SSH_TUNNEL_X11:
		if ((tunnel->x11_channel = ssh_channel_new(tunnel->ssh.session)) == NULL) {
			remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Failed to create channel: %s"));
			tunnel->thread = 0;
			return NULL;
		}
		if (!remmina_public_get_xauth_cookie(tunnel->localdisplay, &ptr)) {
			remmina_ssh_set_application_error(REMMINA_SSH(tunnel), "%s", ptr);
			g_free(ptr);
			tunnel->thread = 0;
			return NULL;
		}
		if (ssh_channel_open_session(tunnel->x11_channel) ||
		    ssh_channel_request_x11(tunnel->x11_channel, TRUE, NULL, ptr,
					    gdk_x11_screen_get_screen_number(gdk_screen_get_default()))) {
			g_free(ptr);
			remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Failed to open channel: %s"));
			tunnel->thread = 0;
			return NULL;
		}
		g_free(ptr);
		if (ssh_channel_request_exec(tunnel->x11_channel, tunnel->dest)) {
			ptr = g_strdup_printf(_("Failed to execute %s on SSH server: %%s"), tunnel->dest);
			remmina_ssh_set_error(REMMINA_SSH(tunnel), ptr);
			g_free(ptr);
			tunnel->thread = 0;
			return NULL;
		}

		if (tunnel->init_func &&
		    !(*tunnel->init_func)(tunnel, tunnel->callback_data)) {
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}

		break;

	case REMMINA_SSH_TUNNEL_XPORT:
		/* Detect the next available port starting from 6010 on the server */
		for (i = 10; i <= MAX_X_DISPLAY_NUMBER; i++) {
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 7, 0)
			if (ssh_channel_listen_forward(REMMINA_SSH(tunnel)->session, (tunnel->bindlocalhost ? "localhost" : NULL), 6000 + i, NULL)) {
				continue;
			} else {
				tunnel->remotedisplay = i;
				break;
			}
#else
			if (ssh_forward_listen(REMMINA_SSH(tunnel)->session, (tunnel->bindlocalhost ? "localhost" : NULL), 6000 + i, NULL)) {
				continue;
			} else {
				tunnel->remotedisplay = i;
				break;
			}
#endif
		}
		if (tunnel->remotedisplay < 1) {
			remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Failed to request port forwarding: %s"));
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}

		if (tunnel->init_func &&
		    !(*tunnel->init_func)(tunnel, tunnel->callback_data)) {
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}

		break;

	case REMMINA_SSH_TUNNEL_REVERSE:
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 7, 0)
		if (ssh_channel_listen_forward(REMMINA_SSH(tunnel)->session, NULL, tunnel->port, NULL)) {
			remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Failed to request port forwarding: %s"));
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}
#else
		if (ssh_forward_listen(REMMINA_SSH(tunnel)->session, NULL, tunnel->port, NULL)) {
			remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Failed to request port forwarding: %s"));
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}
#endif

		if (tunnel->init_func &&
		    !(*tunnel->init_func)(tunnel, tunnel->callback_data)) {
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}

		break;
	}

	tunnel->buffer_len = 10240;
	tunnel->buffer = g_malloc(tunnel->buffer_len);

	/* Start the tunnel data transmission */
	while (tunnel->running) {
		if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_XPORT ||
		    tunnel->tunnel_type == REMMINA_SSH_TUNNEL_X11 ||
		    tunnel->tunnel_type == REMMINA_SSH_TUNNEL_REVERSE) {
			if (first) {
				first = FALSE;
				/* Wait for a period of time for the first incoming connection */
				if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_X11)
					channel = ssh_channel_accept_x11(tunnel->x11_channel, 15000);
				else
					channel = ssh_channel_accept_forward(REMMINA_SSH(tunnel)->session, 15000, &tunnel->port);
				if (!channel) {
					remmina_ssh_set_application_error(REMMINA_SSH(tunnel), _("No response from the server."));
					if (tunnel->disconnect_func)
						(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
					tunnel->thread = 0;
					return NULL;
				}
				if (tunnel->connect_func)
					(*tunnel->connect_func)(tunnel, tunnel->callback_data);
				if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_REVERSE) {
					/* For reverse tunnel, we only need one connection. */
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 7, 0)
					ssh_channel_cancel_forward(REMMINA_SSH(tunnel)->session, NULL, tunnel->port);
#else
					ssh_forward_cancel(REMMINA_SSH(tunnel)->session, NULL, tunnel->port);
#endif
				}
			} else if (tunnel->tunnel_type != REMMINA_SSH_TUNNEL_REVERSE) {
				/* Poll once per some period of time if no incoming connections.
				 * Don’t try to poll continuously as it will significantly slow down the loop */
				g_get_current_time(&t1);
				diff = (t1.tv_sec - t2.tv_sec) * 10 + (t1.tv_usec - t2.tv_usec) / 100000;
				if (diff > 1) {
					if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_X11)
						channel = ssh_channel_accept_x11(tunnel->x11_channel, 0);
					else
						channel = ssh_channel_accept_forward(REMMINA_SSH(tunnel)->session, 0, &tunnel->port);
					if (channel == NULL)
						t2 = t1;
				}
			}

			if (channel) {
				if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_REVERSE) {
					sin.sin_family = AF_INET;
					sin.sin_port = htons(tunnel->localport);
					sin.sin_addr.s_addr = inet_addr("127.0.0.1");
					sock = socket(AF_INET, SOCK_STREAM, 0);
					if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
						remmina_ssh_set_application_error(REMMINA_SSH(tunnel),
										  _("Cannot connect to local port %i."), tunnel->localport);
						close(sock);
						sock = -1;
					}
				} else {
					sock = remmina_public_open_xdisplay(tunnel->localdisplay);
				}
				if (sock >= 0) {
					remmina_ssh_tunnel_add_channel(tunnel, channel, sock);
				} else {
					/* Failed to create unix socket. Will this happen? */
					ssh_channel_close(channel);
					ssh_channel_send_eof(channel);
					ssh_channel_free(channel);
				}
				channel = NULL;
			}
		}

		if (tunnel->num_channels <= 0)
			/* No more connections. We should quit */
			break;

		timeout.tv_sec = 0;
		timeout.tv_usec = 200000;

		FD_ZERO(&set);
		maxfd = 0;
		for (i = 0; i < tunnel->num_channels; i++) {
			if (tunnel->sockets[i] > maxfd)
				maxfd = tunnel->sockets[i];
			FD_SET(tunnel->sockets[i], &set);
		}

		ret = ssh_select(tunnel->channels, tunnel->channels_out, maxfd + 1, &set, &timeout);
		if (!tunnel->running) break;
		if (ret == SSH_EINTR) continue;
		if (ret == -1) break;

		i = 0;
		while (tunnel->running && i < tunnel->num_channels) {
			disconnected = FALSE;
			if (FD_ISSET(tunnel->sockets[i], &set)) {
				while (!disconnected &&
				       (len = read(tunnel->sockets[i], tunnel->buffer, tunnel->buffer_len)) > 0) {
					for (ptr = tunnel->buffer, lenw = 0; len > 0; len -= lenw, ptr += lenw) {
						lenw = ssh_channel_write(tunnel->channels[i], (char *)ptr, len);
						if (lenw <= 0) {
							disconnected = TRUE;
							remmina_ssh_set_error(REMMINA_SSH(tunnel), _("ssh_channel_write() returned an error: %s"));
							break;
						}
					}
				}
				if (len == 0) {
					remmina_ssh_set_error(REMMINA_SSH(tunnel), _("read on tunnel listening socket returned an error: %s"));
					disconnected = TRUE;
				}
			}
			if (disconnected) {
				remmina_log_printf("[SSH] tunnel has been disconnected. Reason: %s\n", REMMINA_SSH(tunnel)->error);
				remmina_ssh_tunnel_remove_channel(tunnel, i);
				continue;
			}
			i++;
		}
		if (!tunnel->running) break;

		i = 0;
		while (tunnel->running && i < tunnel->num_channels) {
			disconnected = FALSE;
			if (!tunnel->socketbuffers[i]) {
				len = ssh_channel_poll(tunnel->channels[i], 0);
				if (len == SSH_ERROR || len == SSH_EOF) {
					remmina_ssh_set_error(REMMINA_SSH(tunnel), _("ssh_channel_poll() returned an error: %s"));
					disconnected = TRUE;
				} else if (len > 0) {
					tunnel->socketbuffers[i] = remmina_ssh_tunnel_buffer_new(len);
					len = ssh_channel_read_nonblocking(tunnel->channels[i], tunnel->socketbuffers[i]->data, len, 0);
					if (len <= 0) {
						remmina_ssh_set_error(REMMINA_SSH(tunnel), _("ssh_channel_read_nonblocking() returned an error: %s"));
						disconnected = TRUE;
					} else {
						tunnel->socketbuffers[i]->len = len;
					}
				}
			}

			if (!disconnected && tunnel->socketbuffers[i]) {
				for (lenw = 0; tunnel->socketbuffers[i]->len > 0;
				     tunnel->socketbuffers[i]->len -= lenw, tunnel->socketbuffers[i]->ptr += lenw) {
					lenw = write(tunnel->sockets[i], tunnel->socketbuffers[i]->ptr, tunnel->socketbuffers[i]->len);
					if (lenw == -1 && errno == EAGAIN && tunnel->running)
						/* Sometimes we cannot write to a socket (always EAGAIN), probably because it’s internal
						 * buffer is full. We need read the pending bytes from the socket first. so here we simply
						 * break, leave the buffer there, and continue with other data */
						break;
					if (lenw <= 0) {
						remmina_ssh_set_error(REMMINA_SSH(tunnel), _("write on tunnel listening socket returned an error: %s"));
						disconnected = TRUE;
						break;
					}
				}
				if (tunnel->socketbuffers[i]->len <= 0) {
					remmina_ssh_tunnel_buffer_free(tunnel->socketbuffers[i]);
					tunnel->socketbuffers[i] = NULL;
				}
			}

			if (disconnected) {
				remmina_log_printf("[SSH] tunnel has been disconnected. Reason: %s\n", REMMINA_SSH(tunnel)->error);
				remmina_ssh_tunnel_remove_channel(tunnel, i);
				continue;
			}
			i++;
		}
		/**
		 * Some protocols may open new connections during the session.
		 * e.g: SPICE opens a new connection for some channels.
		 */
		sock = remmina_ssh_tunnel_accept_local_connection(tunnel, FALSE);
		if (sock > 0) {
			channel = remmina_ssh_tunnel_create_forward_channel(tunnel);
			if (!channel) {
				remmina_log_printf("[SSH] Failed to open new connection: %s\n", REMMINA_SSH(tunnel)->error);
				close(sock);
				/* Leave thread loop */
				tunnel->running = FALSE;
			} else {
				remmina_ssh_tunnel_add_channel(tunnel, channel, sock);
			}
		}
	}

	remmina_ssh_tunnel_close_all_channels(tunnel);

	return NULL;
}

static gpointer
remmina_ssh_tunnel_main_thread(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnel *tunnel = (RemminaSSHTunnel *)data;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	while (TRUE) {
		remmina_ssh_tunnel_main_thread_proc(data);
		if (tunnel->server_sock < 0 || tunnel->thread == 0 || !tunnel->running) break;
	}
	tunnel->thread = 0;
	return NULL;
}

void
remmina_ssh_tunnel_cancel_accept(RemminaSSHTunnel *tunnel)
{
	TRACE_CALL(__func__);
	if (tunnel->server_sock >= 0) {
		close(tunnel->server_sock);
		tunnel->server_sock = -1;
	}
}

gboolean
remmina_ssh_tunnel_open(RemminaSSHTunnel *tunnel, const gchar *host, gint port, gint local_port)
{
	TRACE_CALL(__func__);
	gint sock;
	gint sockopt = 1;
	struct sockaddr_in sin;

	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_OPEN;
	tunnel->dest = g_strdup(host);
	tunnel->port = port;
	if (tunnel->port == 0) {
		REMMINA_SSH(tunnel)->error = g_strdup(_("Destination port has not been assigned."));
		return FALSE;
	}

	/* Create the server socket that listens on the local port */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		REMMINA_SSH(tunnel)->error = g_strdup(_("Failed to create socket."));
		return FALSE;
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));

	sin.sin_family = AF_INET;
	sin.sin_port = htons(local_port);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin))) {
		REMMINA_SSH(tunnel)->error = g_strdup(_("Failed to bind on local port."));
		close(sock);
		return FALSE;
	}

	if (listen(sock, 1)) {
		REMMINA_SSH(tunnel)->error = g_strdup(_("Failed to listen on local port."));
		close(sock);
		return FALSE;
	}

	tunnel->server_sock = sock;
	tunnel->running = TRUE;

	if (pthread_create(&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel)) {
		remmina_ssh_set_application_error(REMMINA_SSH(tunnel), _("Failed to initialize pthread."));
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_x11(RemminaSSHTunnel *tunnel, const gchar *cmd)
{
	TRACE_CALL(__func__);
	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_X11;
	tunnel->dest = g_strdup(cmd);
	tunnel->running = TRUE;

	if (pthread_create(&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel)) {
		remmina_ssh_set_application_error(REMMINA_SSH(tunnel), _("Failed to initialize pthread."));
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_xport(RemminaSSHTunnel *tunnel, gboolean bindlocalhost)
{
	TRACE_CALL(__func__);
	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_XPORT;
	tunnel->bindlocalhost = bindlocalhost;
	tunnel->running = TRUE;

	if (pthread_create(&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel)) {
		remmina_ssh_set_application_error(REMMINA_SSH(tunnel), _("Failed to initialize pthread."));
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_reverse(RemminaSSHTunnel *tunnel, gint port, gint local_port)
{
	TRACE_CALL(__func__);
	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_REVERSE;
	tunnel->port = port;
	tunnel->localport = local_port;
	tunnel->running = TRUE;

	if (pthread_create(&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel)) {
		remmina_ssh_set_application_error(REMMINA_SSH(tunnel), _("Failed to initialize pthread."));
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_terminated(RemminaSSHTunnel *tunnel)
{
	TRACE_CALL(__func__);
	return tunnel->thread == 0;
}

void
remmina_ssh_tunnel_free(RemminaSSHTunnel *tunnel)
{
	TRACE_CALL(__func__);
	pthread_t thread;

	thread = tunnel->thread;
	if (thread != 0) {
		tunnel->running = FALSE;
		pthread_cancel(thread);
		pthread_join(thread, NULL);
		tunnel->thread = 0;
	}

	if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_XPORT && tunnel->remotedisplay > 0) {
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 7, 0)
		ssh_channel_cancel_forward(REMMINA_SSH(tunnel)->session, NULL, 6000 + tunnel->remotedisplay);
#else
		ssh_forward_cancel(REMMINA_SSH(tunnel)->session, NULL, 6000 + tunnel->remotedisplay);
#endif
	}
	if (tunnel->server_sock >= 0) {
		close(tunnel->server_sock);
		tunnel->server_sock = -1;
	}
	remmina_ssh_tunnel_close_all_channels(tunnel);

	g_free(tunnel->buffer);
	g_free(tunnel->channels_out);
	g_free(tunnel->dest);
	g_free(tunnel->localdisplay);

	remmina_ssh_free(REMMINA_SSH(tunnel));
}

/*-----------------------------------------------------------------------------*
*                           SSH sFTP                                          *
*-----------------------------------------------------------------------------*/
RemminaSFTP *
remmina_sftp_new_from_file(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaSFTP *sftp;

	sftp = g_new(RemminaSFTP, 1);

	remmina_ssh_init_from_file(REMMINA_SSH(sftp), remminafile);

	sftp->sftp_sess = NULL;

	return sftp;
}

RemminaSFTP *
remmina_sftp_new_from_ssh(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	RemminaSFTP *sftp;

	sftp = g_new(RemminaSFTP, 1);

	remmina_ssh_init_from_ssh(REMMINA_SSH(sftp), ssh);

	sftp->sftp_sess = NULL;

	return sftp;
}

gboolean
remmina_sftp_open(RemminaSFTP *sftp)
{
	TRACE_CALL(__func__);
	sftp->sftp_sess = sftp_new(sftp->ssh.session);
	if (!sftp->sftp_sess) {
		remmina_ssh_set_error(REMMINA_SSH(sftp), _("Failed to create SFTP session: %s"));
		return FALSE;
	}
	if (sftp_init(sftp->sftp_sess)) {
		remmina_ssh_set_error(REMMINA_SSH(sftp), _("Failed to initialize SFTP session: %s"));
		return FALSE;
	}
	return TRUE;
}

void
remmina_sftp_free(RemminaSFTP *sftp)
{
	TRACE_CALL(__func__);
	if (sftp->sftp_sess) {
		sftp_free(sftp->sftp_sess);
		sftp->sftp_sess = NULL;
	}
	remmina_ssh_free(REMMINA_SSH(sftp));
}

/*-----------------------------------------------------------------------------*
*                           SSH Shell                                         *
*-----------------------------------------------------------------------------*/
RemminaSSHShell *
remmina_ssh_shell_new_from_file(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaSSHShell *shell;

	shell = g_new0(RemminaSSHShell, 1);

	remmina_ssh_init_from_file(REMMINA_SSH(shell), remminafile);

	shell->master = -1;
	shell->slave = -1;
	shell->exec = g_strdup(remmina_file_get_string(remminafile, "exec"));

	return shell;
}

RemminaSSHShell *
remmina_ssh_shell_new_from_ssh(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	RemminaSSHShell *shell;

	shell = g_new0(RemminaSSHShell, 1);

	remmina_ssh_init_from_ssh(REMMINA_SSH(shell), ssh);

	shell->master = -1;
	shell->slave = -1;

	return shell;
}

static gboolean
remmina_ssh_call_exit_callback_on_main_thread(gpointer data)
{
	TRACE_CALL(__func__);

	RemminaSSHShell *shell = (RemminaSSHShell *)data;
	if (shell->exit_callback)
		shell->exit_callback(shell->user_data);
	return FALSE;
}

static gpointer
remmina_ssh_shell_thread(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaSSHShell *shell = (RemminaSSHShell *)data;
	fd_set fds;
	struct timeval timeout;
	ssh_channel channel = NULL;
	ssh_channel ch[2], chout[2];
	gchar *buf = NULL;
	gint buf_len;
	gint len;
	gint i, ret;
	//gint screen;

	LOCK_SSH(shell)

	if ((channel = ssh_channel_new(REMMINA_SSH(shell)->session)) == NULL ||
	    ssh_channel_open_session(channel)) {
		UNLOCK_SSH(shell)
		remmina_ssh_set_error(REMMINA_SSH(shell), _("Failed to open channel: %s"));
		if (channel) ssh_channel_free(channel);
		shell->thread = 0;
		return NULL;
	}

	ssh_channel_request_pty(channel);

	/* ssh_channel_request_x11 works but I do not know yet how to use the
	 * channel. At the momonet I leave the code here, commented, for future
	 * investigations.
	 */
	//screen = gdk_x11_screen_get_screen_number(gdk_screen_get_default());
	//ret = ssh_channel_request_x11 (channel, FALSE, NULL, NULL, screen);
	/* TODO: We should have a callback that intercept an x11 request to use
	 * ssh_channel_accept_x11 */
	//if ( ret != SSH_OK ) {
	//g_print ("[SSH] X11 channel error: %d\n", ret);
	//}else {
	//ssh_channel_accept_x11 ( channel, 50);
	//}


	if (shell->exec && shell->exec[0])
		ret = ssh_channel_request_exec(channel, shell->exec);
	else
		ret = ssh_channel_request_shell(channel);
	if (ret) {
		UNLOCK_SSH(shell)
		remmina_ssh_set_error(REMMINA_SSH(shell), _("Failed to request shell: %s"));
		ssh_channel_close(channel);
		ssh_channel_send_eof(channel);
		ssh_channel_free(channel);
		shell->thread = 0;
		return NULL;
	}

	shell->channel = channel;

	UNLOCK_SSH(shell)

	buf_len = 1000;
	buf = g_malloc(buf_len + 1);

	ch[0] = channel;
	ch[1] = NULL;

	while (!shell->closed) {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET(shell->slave, &fds);

		ret = ssh_select(ch, chout, shell->slave + 1, &fds, &timeout);
		if (ret == SSH_EINTR) continue;
		if (ret == -1) break;

		if (FD_ISSET(shell->slave, &fds)) {
			len = read(shell->slave, buf, buf_len);
			if (len <= 0) break;
			LOCK_SSH(shell)
			ssh_channel_write(channel, buf, len);
			UNLOCK_SSH(shell)
		}
		for (i = 0; i < 2; i++) {
			LOCK_SSH(shell)
			len = ssh_channel_poll(channel, i);
			UNLOCK_SSH(shell)
			if (len == SSH_ERROR || len == SSH_EOF) {
				shell->closed = TRUE;
				break;
			}
			if (len <= 0) continue;
			if (len > buf_len) {
				buf_len = len;
				buf = (gchar *)g_realloc(buf, buf_len + 1);
			}
			LOCK_SSH(shell)
			len = ssh_channel_read_nonblocking(channel, buf, len, i);
			UNLOCK_SSH(shell)
			if (len <= 0) {
				shell->closed = TRUE;
				break;
			}
			while (len > 0) {
				ret = write(shell->slave, buf, len);
				if (ret <= 0) break;
				len -= ret;
			}
		}
	}

	LOCK_SSH(shell)
	shell->channel = NULL;
	ssh_channel_close(channel);
	ssh_channel_send_eof(channel);
	ssh_channel_free(channel);
	UNLOCK_SSH(shell)

	g_free(buf);
	shell->thread = 0;

	if (shell->exit_callback)
		IDLE_ADD((GSourceFunc)remmina_ssh_call_exit_callback_on_main_thread, (gpointer)shell);
	return NULL;
}


gboolean
remmina_ssh_shell_open(RemminaSSHShell *shell, RemminaSSHExitFunc exit_callback, gpointer data)
{
	TRACE_CALL(__func__);
	gchar *slavedevice;
	struct termios stermios;

	shell->master = posix_openpt(O_RDWR | O_NOCTTY);
	if (shell->master == -1 ||
	    grantpt(shell->master) == -1 ||
	    unlockpt(shell->master) == -1 ||
	    (slavedevice = ptsname(shell->master)) == NULL ||
	    (shell->slave = open(slavedevice, O_RDWR | O_NOCTTY)) < 0) {
		REMMINA_SSH(shell)->error = g_strdup(_("Failed to create pty device."));
		return FALSE;
	}

	/* As per libssh documentation */
	tcgetattr(shell->slave, &stermios);
	stermios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	stermios.c_oflag &= ~OPOST;
	stermios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	stermios.c_cflag &= ~(CSIZE | PARENB);
	stermios.c_cflag |= CS8;
	tcsetattr(shell->slave, TCSANOW, &stermios);

	shell->exit_callback = exit_callback;
	shell->user_data = data;

	/* Once the process started, we should always TRUE and assume the pthread will be created always */
	pthread_create(&shell->thread, NULL, remmina_ssh_shell_thread, shell);

	return TRUE;
}

void
remmina_ssh_shell_set_size(RemminaSSHShell *shell, gint columns, gint rows)
{
	TRACE_CALL(__func__);
	LOCK_SSH(shell)
	if (shell->channel)
		ssh_channel_change_pty_size(shell->channel, columns, rows);
	UNLOCK_SSH(shell)
}

void
remmina_ssh_shell_free(RemminaSSHShell *shell)
{
	TRACE_CALL(__func__);
	pthread_t thread = shell->thread;

	shell->exit_callback = NULL;
	if (thread) {
		shell->closed = TRUE;
		pthread_join(thread, NULL);
	}
	close(shell->slave);
	if (shell->exec) {
		g_free(shell->exec);
		shell->exec = NULL;
	}
	/* It’s not necessary to close shell->slave since the other end (vte) will close it */;
	remmina_ssh_free(REMMINA_SSH(shell));
}

#endif /* HAVE_LIBSSH */
