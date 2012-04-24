#include <string>
#include <vector>
#include <purple.h>

#include <core/MojObject.h>


static void* enyo_request_action(const char* title, const char* primary,
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

    // Util::Queue& queue = *reinterpret_cast<Util::MessageQueue*> (user_data);

    // ActionType = std::pair<std::string(strip underscores), PurpleRequestActionCb>
    typedef std::pair<std::string, PurpleRequestActionCb> action_type;
    std::vector<action_type> actions_vector;

    for (unsigned i = 0; i < action_count; ++i)
    {
        actions_vector.push_back(action_type());

        action_type& action = actions_vector.back();
        
        const char* text = va_arg(actions, const char*);
        // Filter underscores
        for (; *text != '\0'; ++text)
            if (*text != '_')
                action.first.append(text, 1);

        action.second = va_arg(actions, PurpleRequestActionCb);
    }

    // TODO: Add request id
    // queue.push(request, actions);
    return 0;
}

static void enyo_close_request(PurpleRequestType, void* handle) {}

static void* enyo_notify_message(PurpleNotifyMsgType type, const char* title,
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

    // We would need global data over here. This should come from
    // Libpurpleadapter (or it's successor)
    // TODO: Implement this
    // show(message);
    return 0;
}

static void enyo_close_notify(PurpleNotifyType, void* handle) {}

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

PurpleRequestUiOps* enyo_request_get_ui_ops()
{
    return &req_ops;
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

PurpleNotifyUiOps* enyo_notify_get_ui_ops()
{
    return &not_ops;
};
