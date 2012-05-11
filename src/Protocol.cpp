#include "Util.hpp"

namespace Util
{

    namespace 
    {

        PurplePluginProtocolInfo* getProtocolInfo(const char* prpl_id)
        {
            // Find prpl name
            PurplePlugin* prpl = purple_find_prpl(prpl_id);

            if (prpl == NULL)
                throw MojoException("Couldn't find prpl");

            PurplePluginProtocolInfo* result = PURPLE_PLUGIN_PROTOCOL_INFO(prpl);

            if (result == NULL)
                throw MojoException("Couldn't find prpl");

            return result;
        }
    }

    MojObject getProtocolOptions(MojString prpl)
    {
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
                node.putBool("default_value",
                    purple_account_option_get_default_bool(option)
                    );
                break;

            case PURPLE_PREF_INT:
                node.putString("type", "int");
                node.putInt("default_value",
                    purple_account_option_get_default_int(option)
                    );
                break;

            case PURPLE_PREF_STRING:
                node.putString("type", "string");
                {
                    const char* def
                        = purple_account_option_get_default_string(option);
                    node.putString("default_value", def ? def : "");
                }
                break;

            case PURPLE_PREF_STRING_LIST:
                node.putString("type", "list");
                {
                    MojObject choices;

                    for (GList* list = purple_account_option_get_list(option);
                         list != NULL; list = list->next)
                    {
                        PurpleKeyValuePair* kvp = (PurpleKeyValuePair*)list->data;
                        // XXX: Dangerous!
                        if (kvp->key && kvp->value)
                            choices.putString((const char*)kvp->value,
                                              (const char*)kvp->key);
                    }
                    node.put("choices", choices);

                    const char* def
                        = purple_account_option_get_default_list_value(option);
                    node.putString("default_value", def ? def : "");
                }
                break;

            default:
                continue;
            };

            result.put(purple_account_option_get_setting(option), node);
        }

        return result;
    }

    PurpleAccount* createPurpleAccount(MojString username, MojString prpl, MojObject prefs)
    {
        PurplePluginProtocolInfo* info = getProtocolInfo(prpl.data());

        // TODO: Strip off possible junk here!
        //       The Username Split API might be useful, as soon as I have
        //       understood it ...
        PurpleAccount* account = purple_account_new(username.data(), prpl.data());

        for(GList* l = info->protocol_options; l != NULL; l = l->next)
        {
            PurpleAccountOption* option = (PurpleAccountOption*)l->data;

            const char* name = purple_account_option_get_setting(option);

            if (!prefs.contains(name))
                continue;

            switch(purple_account_option_get_type(option))
            {
            case PURPLE_PREF_BOOLEAN:
                {
                    bool value;
                    prefs.get(name, value);
                    purple_account_set_bool(account, name, value);
                }
                break;

            case PURPLE_PREF_INT:
                {
                    bool found;
                    int value;
                    prefs.get(name, value, found);
                    purple_account_set_int(account, name, value);
                }
                break;

            case PURPLE_PREF_STRING:
            case PURPLE_PREF_STRING_LIST:
                {
                    bool found;
                    MojString value;
                    prefs.get(name, value, found);
                    purple_account_set_string(account, name, value.data());
                }
                break;

            default:
                continue;
            }
        }

        return account;
    }
}


