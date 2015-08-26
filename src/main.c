#include <libircclient.h>
#include <libirc_rfcnumeric.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "EventHistory.h"

typedef struct
{
        const char* channel;
        const char* nick;

        EventHistory_t* history;
} irc_ctx_t;

void writeLog(const char* fmt, ...)
{
    char buf[1024];
    va_list va_alist;

    va_start(va_alist, fmt);
#if defined (_WIN32)
    _vsnprintf(buf, sizeof(buf), fmt, va_alist);
#else
    vsnprintf(buf, sizeof(buf), fmt, va_alist);
#endif
    va_end(va_alist);

    printf("%s\n", buf);

    FILE* fp = NULL;
    if ((fp = fopen ("bot.log", "ab")) != 0)
    {
        fprintf(fp, "%s\n", buf);
        fclose(fp);
    }
}

void dump_event(irc_session_t* session,
                const char* event, const char* origin,
                const char** params, unsigned int paramCount)
{
    (void) session;

    char buf[512];
    buf[0] = '\0';

    for (unsigned int i = 0; i < paramCount; i++)
    {
        if (i)
            strcat (buf, "|");

        strcat(buf, params[i]);
    }

    printf("Event \"%s\", origin: \"%s\", params: %d [%s]\n", event, origin ? origin : "NULL", paramCount, buf);
}

void event_log(irc_session_t* session,
               const char* event, const char* origin,
               const char** params, unsigned int paramCount)
{
    irc_ctx_t* ctx = (irc_ctx_t*)irc_get_ctx(session);
    pushEventToHistory(ctx->history, time(NULL), event, origin, params, paramCount);
}

void event_join(irc_session_t* session,
                const char* event, const char* origin,
                const char** params, unsigned int paramCount)
{
    dump_event(session, event, origin, params, paramCount);
    irc_cmd_user_mode(session, "+i");
}

void event_connect(irc_session_t* session,
                   const char* event, const char* origin,
                   const char** params, unsigned int paramCount)
{
    irc_ctx_t* ctx = (irc_ctx_t *)irc_get_ctx(session);
    dump_event(session, event, origin, params, paramCount);

    writeLog("Joining %s!", ctx->channel);

    // Auto-join
    irc_cmd_join(session, ctx->channel, NULL);
}

void event_numeric(irc_session_t* session,
                   unsigned int event, const char* origin,
                   const char** params, unsigned int paramCount)
{
    char buf[24];
    sprintf (buf, "%d", event);

    dump_event(session, buf, origin, params, paramCount);
}

void event_channel(irc_session_t* session,
                   const char* event, const char* origin,
                   const char** params, unsigned int paramCount)
{
    dump_event(session, event, origin, params, paramCount);

    if (paramCount != 2)
        return;

    printf("'%s' said in channel %s: %s\n",
           origin ? origin : "someone",
           params[0], params[1]);

    // Log this message.
    event_log(session, event, origin, params, paramCount);
}

void event_privmsg(irc_session_t* session,
                   const char* event, const char* origin,
                   const char** params, unsigned int paramCount)
{
    dump_event(session, event, origin, params, paramCount);

    printf("'%s' quietly told me (%s): %s\n",
           origin ? origin : "someone",
           params[0], params[1]);

    // "A buffer of size 128 should be enough for most nicks."
    char nickbuf[128];
    irc_target_get_nick(origin, nickbuf, sizeof(nickbuf));

    if (!strcmp(nickbuf, "MHL") && !strcmp(params[1], "quit"))
    {
        irc_cmd_quit(session, "Shutting down.");
    }
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    // The IRC callbacks structure
    irc_callbacks_t callbacks;

    // Init it
    memset(&callbacks, 0, sizeof(callbacks));

    // Set up the mandatory events
    callbacks.event_connect = event_connect;
    callbacks.event_numeric = event_numeric;

    // Set up the rest of events
    callbacks.event_join = event_join;
    callbacks.event_nick = event_log;
    callbacks.event_quit = event_log;
    callbacks.event_part = event_log;
    callbacks.event_mode = dump_event;
    callbacks.event_topic = event_log;
    callbacks.event_kick = event_log;
    callbacks.event_channel = event_channel;
    callbacks.event_privmsg = event_privmsg;
    /* callbacks.event_notice = dump_event; */
    /* callbacks.event_invite = dump_event; */
    /* callbacks.event_umode = dump_event; */
    /* callbacks.event_ctcp_rep = dump_event; */
    /* callbacks.event_ctcp_action = dump_event; */
    callbacks.event_unknown = dump_event;

    writeLog("Callbacks Set!");

    // Now create the session
    irc_session_t* session = irc_create_session(&callbacks);
    if (!session)
    {
        writeLog("Could not create irc session.");
        return 1;
    }

    writeLog("Session Created!");

    irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);

    // Bot / Server details
    const char* server   = "irc.quakenet.org";
    unsigned short port  = 6667;
    const char* password = NULL;
    const char* nick     = "maryb_otrev2";
    const char* username = "otrev";
    const char* realname = "maryb otrev";

    // Userdata to pass between callback functions.
    irc_ctx_t ctx =
        {
            .channel = "#MaryBotTest",
            .nick=(char*)nick,
            .history = createEventHistory(128)
        };
    irc_set_ctx(session, &ctx);

    // Initiate the IRC server connection
    if (irc_connect(session, server, port, password, nick, username, realname))
    {
        writeLog("Could not connect: %s", irc_strerror(irc_errno(session)));
        return 1;
    }

    writeLog("Connected!");

    // Loop and handle events when they come.
    if (irc_run(session))
    {
        writeLog("Could not connect or I/O error: %s", irc_strerror(irc_errno(session)));
        return 1;
    }

    destroyEventHistory(ctx.history);

    return 0;
}
