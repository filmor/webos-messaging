#include "PurpleUI.hpp"

#include <string>
#include <vector>
#include <purple.h>

#include <core/MojObject.h>
#include <util/concurrent_queue.hpp>
#include <unordered_map>

namespace Purple
{
    namespace
    {
        struct Action
        {
            std::vector<PurpleRequestActionCb> answers;
            void* user_data;
        };

        util::concurrent_queue<MojObject> event_queue;
        static unsigned id_counter;
        std::unordered_map<unsigned, Action> pending_actions;
    }

    extern "C"
    void* enyo_request_action(const char* title, const char* primary,
            const char* secondary, int default_value, PurpleAccount* account,
            const char* who, PurpleConversation* conv, void* user_data,
            size_t action_count, va_list actions)
    {
        MojObject request;

        if (title)
            request.putString("title", title);

        if (primary)
            request.putString("primary", primary);

        if (secondary)
            request.putString("secondary", secondary);

        request.putInt("default_action", default_value);

        Action a;
        a.user_data = user_data;

        MojObject action_array (MojObject::TypeArray);

        for (unsigned i = 0; i < action_count; ++i)
        {
            std::string name;
            const char* text = va_arg(actions, const char*);
            // Filter underscores
            for (; *text != '\0'; ++text)
                if (*text != '_')
                    name.append(text, 1);

            action_array.pushString(name.c_str());

            PurpleRequestActionCb cb = va_arg(actions, PurpleRequestActionCb);
            a.answers.push_back(cb);
        }

        request.put("actions", action_array);

        const unsigned id = id_counter++;
        request.putInt("id", id);

        event_queue.push(request);
        pending_actions[id] = a;

        return 0;
    }

    extern "C" void enyo_close_request(PurpleRequestType, void* handle) {}

    extern "C"
    void* enyo_notify_message(PurpleNotifyMsgType type, const char* title,
            const char* primary, const char* secondary)
    {
        MojObject message;

        if (title) message.putString("title", title);
        if (primary) message.putString("primary", primary);
        if (secondary) message.putString("secondary", secondary);

        const char* msg_type = 0;
        switch(type)
        {
        case PURPLE_NOTIFY_MSG_ERROR:
            msg_type = "error";
            break;
        case PURPLE_NOTIFY_MSG_WARNING:
            msg_type = "warning";
            break;
        case PURPLE_NOTIFY_MSG_INFO:
            msg_type = "info";
            break;
        default:
            msg_type = "unknown";
            break;
        };

        message.putString("type", msg_type);

        event_queue.push(message);
        return 0;
    }

    extern "C" void enyo_close_notify(PurpleNotifyType, void* handle) {}

    static PurpleRequestUiOps req_ops = 
    {
        0,                      // request_input
        0,                      // request_choice
        enyo_request_action,    // request_action
        0,                      // request_fields
        0,                      // request_file
        enyo_close_request,     // close_request
        0,                      // request_folder
        0, 0, 0, 0              // padding
    };

    PurpleRequestUiOps* getRequestOps()
    {
        return &req_ops;
    }

    void popEvent(MojObject& var)
    {
        event_queue.pop(var);
    }

    static PurpleNotifyUiOps not_ops =
    {
        enyo_notify_message,    // notify_message
        0,                      // notify_email
        0,                      // notify_emails
        0,                      // notify_formatted
        0,                      // notify_searchresults
        0,                      // notify_sr_new_rows
        0,                      // notify_userinfo
        0,                      // notify_uri
        enyo_close_notify,      // close_notify
        0, 0, 0, 0              // padding
    };

    PurpleNotifyUiOps* getNotifyOps()
    {
        return &not_ops;
    };

    MojErr answerRequest(unsigned id, unsigned answer)
    {
        if (pending_actions.count(id) == 0)
            return MojErrInvalidArg;

        if (pending_actions[id].answers.size() >= answer)
            return MojErrInvalidArg;

        PurpleRequestActionCb callback = pending_actions[id].answers[answer];
        void* user_data = pending_actions[id].user_data;

        pending_actions.erase(id);

        if (callback)
            callback(user_data, answer);

        return MojErrNone;
    }
}
