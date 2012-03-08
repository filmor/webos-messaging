#include "Protocol.hpp"

namespace Messaging
{

    namespace 
    {

        PurplePluginProtocolInfo* getProtocolInfo(const char* prpl_id)
        {
            // Find prpl name
            PurplePlugin* prpl = purple_find_prpl(prpl_id);

            if (prpl == NULL)
                // ERROR!
                throw "Error";

            PurplePluginProtocolInfo* result = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

            if (result == NULL)
                // ERROR!
                throw "Error";

            return result;
        }
    }

    MojObject getProtocolOptions(MojString prpl)
    {
        // TODO: Defaults for Facebook and Google Talk (later, for now just set
        // the values manually in the wizard app)
        //
        // Like this: Derived defines getDefaults and returns a MojObject. The
        // options are checked in the loop, if they are already set in the
        // defaults, just continue.
        //
        MojObject result;

        PurplePluginProtocolInfo* info = getProtocolInfo(prpl.data());

        for(GList* l = info->protocol_options; l != NULL; l = l->next)
        {
            PurpleAccountOption* option = (PurpleAccountOption*)l->data;

            MojObject node;
            MojObject choices;

            node.putString("text", purple_account_option_get_text(option));

            switch(purple_account_option_get_type(option))
            {
            case PURPLE_PREF_BOOLEAN:
                node.putString("type", "bool");
                node.putBool("default",
                    purple_account_option_get_default_bool(option)
                    );
                break;

            case PURPLE_PREF_INT:
                node.putString("type", "int");
                node.putInt("default",
                    purple_account_option_get_default_int(option)
                    );
                break;

            case PURPLE_PREF_STRING:
                node.putString("type", "string");
                node.putString("default",
                    purple_account_option_get_default_string(option)
                    );
                break;

            case PURPLE_PREF_STRING_LIST:
                node.putString("type", "list");

                for (GList* list = purple_account_option_get_list(option);
                     list != NULL; list = list->next)
                {
                    PurpleKeyValuePair* kvp = (PurpleKeyValuePair*)list;
                    // XXX: Dangerous!
                    choices.putString((MojChar*)kvp->key, (MojChar*)kvp->value);
                }
                node.put("choices", choices);

                // TODO: Default value purple_account_option_get_list_default
                //       Could be the same as the first value of the list
                break;

            default:
                continue;
            };

            result.put(purple_account_option_get_setting(option), node);
        }

        return result;
    }

    PurpleAccount* createPurpleAccount(MojString prpl, MojString username,
                                       MojObject config)
    {
        PurplePluginProtocolInfo* info = getProtocolInfo(prpl.data());

        // TODO: Strip of possible junk here!
        PurpleAccount* account = purple_account_new(username.data(), prpl.data());

        for(GList* l = info->protocol_options; l != NULL; l = l->next)
        {
            PurpleAccountOption* option = (PurpleAccountOption*)l->data;

            const char* name = purple_account_option_get_setting(option);

            if (!config.contains(name))
                continue;

            switch(purple_account_option_get_type(option))
            {
            case PURPLE_PREF_BOOLEAN:
                {
                    bool value;
                    config.get(name, value);
                    purple_account_set_bool(account, name, value);
                }
                break;

            case PURPLE_PREF_INT:
                {
                    bool found;
                    int value;
                    config.get(name, value, found);
                    purple_account_set_int(account, name, value);
                }
                break;

            case PURPLE_PREF_STRING:
            case PURPLE_PREF_STRING_LIST:
                {
                    bool found;
                    MojString value;
                    config.get(name, value, found);
                    purple_account_set_string(account, name, value.data());
                }
                break;

            default:
                continue;
            };
        }
    }
}


