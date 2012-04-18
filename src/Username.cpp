#include "Util.hpp"

namespace Util
{

    MojString getMojoUsername(MojString username, MojObject config)
    {
        MojSize at_char = username.find('@');

        if (at_char == username.length())
        {
            MojString prpl;
            config.getRequired("prpl", prpl);
            username.append("@");
            username.append(prpl.data());
            username.append(".example.com");
        }
    }

    const char* getPurpleUsername(MojString username, MojObject config)
    {
#error NIY
        return username.data();    
    }

}
