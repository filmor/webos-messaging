/*
 * IMAccountValidatorApp.cpp
 *
 * Copyright 2010 Palm, Inc. All rights reserved.
 *
 * This program is free software and licensed under the terms of the GNU
 * General Public License Version 2 as published by the Free
 * Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License,
 * Version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-
 * 1301, USA
 *
 * IMAccountValidator uses libpurple.so to implement the checking of credentials when logging into an IM account
 */

/*
 * This file includes code from pidgin  (nullclient.c)
 *
 * pidgin
 *
 * Pidgin is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 *
 */

#include "IMAccountValidatorApp.h"
#include "luna/MojLunaService.h"
#include "core.h"
#include "debug.h"

const char* const IMAccountValidatorApp::ServiceName = _T("com.palm.imaccountvalidator");
MojLogger IMAccountValidatorApp::s_log(_T("imaccountvalidator"));

/**
 * The following eventloop functions are used in both pidgin and purple-text. If your
 * application uses glib mainloop, you can safely use this verbatim.
 * copied from http://pidgin.sourcearchive.com/documentation/2.5.2/nullclient_8c-source.html
 */
#define PURPLE_GLIB_READ_COND  (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define PURPLE_GLIB_WRITE_COND (G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL)

typedef struct _PurpleGLibIOClosure {
      PurpleInputFunction function;
      guint result;
      gpointer data;
} PurpleGLibIOClosure;

static void purple_glib_io_destroy(gpointer data)
{
      g_free(data);
}

static gboolean purple_glib_io_invoke(GIOChannel *source, GIOCondition condition, gpointer data)
{
      PurpleGLibIOClosure *closure = (PurpleGLibIOClosure *)data;
      int purple_cond = 0;

      if (condition & PURPLE_GLIB_READ_COND)
            purple_cond |= PURPLE_INPUT_READ;
      if (condition & PURPLE_GLIB_WRITE_COND)
            purple_cond |= PURPLE_INPUT_WRITE;

      closure->function(closure->data, g_io_channel_unix_get_fd(source), (PurpleInputCondition)purple_cond);

      return TRUE;
}

static guint glib_input_add(gint fd, PurpleInputCondition condition, PurpleInputFunction function, gpointer data)
{
      PurpleGLibIOClosure *closure = g_new0(PurpleGLibIOClosure, 1);
      GIOChannel *channel;
      int cond = 0;

      closure->function = function;
      closure->data = data;

      if (condition & PURPLE_INPUT_READ)
            cond |= PURPLE_GLIB_READ_COND;
      if (condition & PURPLE_INPUT_WRITE)
            cond |= PURPLE_GLIB_WRITE_COND;

      channel = g_io_channel_unix_new(fd);
      closure->result = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT, (GIOCondition) cond,
                                    purple_glib_io_invoke, closure, purple_glib_io_destroy);

      g_io_channel_unref(channel);
      return closure->result;
}


static PurpleEventLoopUiOps glib_eventloops =
{
      g_timeout_add,
      g_source_remove,
      glib_input_add,
      g_source_remove,
      NULL,
#if GLIB_CHECK_VERSION(2,14,0)
      g_timeout_add_seconds,
#else
      NULL,
#endif

      /* padding */
      NULL,
      NULL,
      NULL
};


/*** Conversation uiops ***/
static void null_write_conv(PurpleConversation *conv, const char *who, const char *alias,
                  const char *message, PurpleMessageFlags flags, time_t mtime)
{
      const char *name;
      if (alias && *alias)
            name = alias;
      else if (who && *who)
            name = who;
      else
            name = NULL;

      MojLogInfo(IMAccountValidatorApp::s_log, "(%s) %s %s: %s\n", purple_conversation_get_name(conv),
                  purple_utf8_strftime("(%H:%M:%S)", localtime(&mtime)),
                  name, message);
}

static PurpleConversationUiOps null_conv_uiops =
{
      NULL,                      /* create_conversation  */
      NULL,                      /* destroy_conversation */
      NULL,                      /* write_chat           */
      NULL,                      /* write_im             */
      null_write_conv,           /* write_conv           */
      NULL,                      /* chat_add_users       */
      NULL,                      /* chat_rename_user     */
      NULL,                      /* chat_remove_users    */
      NULL,                      /* chat_update_user     */
      NULL,                      /* present              */
      NULL,                      /* has_focus            */
      NULL,                      /* custom_smiley_add    */
      NULL,                      /* custom_smiley_write  */
      NULL,                      /* custom_smiley_close  */
      NULL,                      /* send_confirm         */
      NULL,
      NULL,
      NULL,
      NULL
};

static void null_ui_init(void)
{
      /**
       * This should initialize the UI components for all the modules. Here we
       * just initialize the UI for conversations.
       */
      purple_conversations_set_ui_ops(&null_conv_uiops);
}

static PurpleCoreUiOps null_core_uiops =
{
      NULL,
      NULL,
      null_ui_init,
      NULL,

      /* padding */
      NULL,
      NULL,
      NULL,
      NULL
};

int main(int argc, char** argv)
{
	IMAccountValidatorApp app;
	app.initializeLibPurple();
	int mainResult = app.main(argc, argv);
	return mainResult;
}

IMAccountValidatorApp::IMAccountValidatorApp()
{
}

MojErr IMAccountValidatorApp::open()
{
	MojLogNotice(s_log, _T("%s starting..."), name().data());

	MojErr err = Base::open();
	MojErrCheck(err);

	// open service
	err = m_service.open(ServiceName);
	MojErrCheck(err);
	err = m_service.attach(m_reactor.impl());
	MojErrCheck(err);

	// create and attach service handler
	m_handler.reset(new IMAccountValidatorHandler(&m_service));
	MojAllocCheck(m_handler.get());

	err = m_handler->init(this);
	MojErrCheck(err);

	err = m_service.addCategory(MojLunaService::DefaultCategory, m_handler.get());
	MojErrCheck(err);

	MojLogNotice(s_log, _T("%s started."), name().data());

	return MojErrNone;
}

MojErr IMAccountValidatorApp::close()
{
	MojLogNotice(s_log, _T("%s stopping..."), name().data());

	MojErr err = MojErrNone;
	MojErr errClose = m_service.close();
	MojErrAccumulate(err, errClose);
	errClose = Base::close();
	MojErrAccumulate(err, errClose);

	MojLogNotice(s_log, _T("%s stopped."), name().data());

	return err;
}

MojErr IMAccountValidatorApp::initializeLibPurple() {

	/* libpurple's built-in DNS resolution forks processes to perform
	 * blocking lookups without blocking the main process.  It does not
	 * handle SIGCHLD itself, so if the UI does not you quickly get an army
	 * of zombie subprocesses marching around.
	 */
	signal(SIGCHLD, SIG_IGN);

	/* turn on debugging. Turn off to keep the noise to a minimum. */
	purple_debug_set_enabled(TRUE);

	  /* Set the core-uiops, which is used to
	   *    - initialize the ui specific preferences.
	   *    - initialize the debug ui.
	   *    - initialize the ui components for all the modules.
	   *    - uninitialize the ui components for all the modules when the core terminates.
	   */
	purple_core_set_ui_ops(&null_core_uiops);

	/* Set the uiops for the eventloop. If your client is glib-based, you can safely
	 * copy this verbatim. */
	purple_eventloop_set_ui_ops(&glib_eventloops);

	/* Now that all the essential stuff has been set, let's try to init the core. It's
	 * necessary to provide a non-NULL name for the current ui to the core. This name
	 * is used by stuff that depends on this ui, for example the ui-specific plugins. */
	if (!purple_core_init(UI_ID))
	{
		MojLogError(IMAccountValidatorApp::s_log, "libpurple initialization failed.");
		abort();
	}

	/* Create and load the buddylist. */
	purple_set_blist(purple_blist_new());
	purple_blist_load();

	return MojErrNone;

}
