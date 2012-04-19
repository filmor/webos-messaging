#ifndef MESSAGING_UTIL_HPP
#define MESSAGING_UTIL_HPP

#include <stdexcept>
#include <core/MojObject.h>
#include <core/MojString.h>
#include <core/MojJson.h>

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
    // Expects Mojo Username (i.e. user@example.com)
    PurpleAccount* createPurpleAccount(MojString username, MojObject config);

    // If there is 
	std::string getMojoUsername(std::string username, std::string const& prpl);
	std::string getPurpleUsername(std::string const& serviceName, std::string const& username);

}

#endif
