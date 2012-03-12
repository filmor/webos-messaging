#ifndef MESSAGING_UTIL_HPP
#define MESSAGING_UTIL_HPP

#include <stdexcept>
#include <core/MojObject.h>
#include <core/MojString.h>

#include <purple.h>

namespace Util
{

    MojString get (MojObject const& obj, const char* key);

    struct MojoException
    {
        MojoException (std::string const& err) : err_(err)
        {}

        virtual ~MojoException () {}

        std::string const& what() const { return err_; }

        std::string err_;
    };

    MojObject getProtocolOptions(MojString prpl);
    PurpleAccount* createPurpleAccount(MojString username, MojObject config);

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
