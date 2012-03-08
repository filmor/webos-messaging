#ifndef MESSAGING_PROTOCOL_HPP
#define MESSAGING_PROTOCOL_HPP

#include <core/MojObject.h>

#include <purple.h>

namespace Messaging
{

    MojObject getProtocolOptions(MojString prpl);

    PurpleAccount* createPurpleAccount(MojString prpl, MojString username,
                                       MojObject config);

/*    namespace
    {
        Protocol protocols[] =
        {
            {"com.palm.aol", "prpl-aim", "type_aim"},
            {"org.webosinternals.icq", "prpl-icq", "type_icq"},
            {"org.webosinternals.facebook", "prpl-jabber", "type_jabber"},
            {"com.palm.google.talk", "prpl-jabber", "type_gtalk"},
            {"org.webosinternals.msn", "prpl-msn", "type_msn"}
        };
    }*/


}

#endif
