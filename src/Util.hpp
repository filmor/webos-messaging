#ifndef MESSAGING_UTIL_HPP
#define MESSAGING_UTIL_HPP

#include <stdexcept>
#include <core/MojObject.h>
#include <core/MojString.h>

namespace Util
{

    MojString get (MojObject const& obj, const char* key);
    MojObject get_object (MojObject const& obj, const char* key);

    struct MojoException
    {
        MojoException (std::string const& err) : err_(err)
        {}

        virtual ~MojoException () {}

        std::string const& what() const { return err_; }

        std::string err_;
    };

}

#endif
