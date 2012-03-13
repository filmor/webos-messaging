#include "Util.hpp"

namespace Util
{

    MojString get(MojObject const& obj, const char* key)
    {
        bool found = false;
        MojString result;

        MojErr err = obj.get(key, result, found);

        if (!found)
        {
            std::string error_text = "Missing '";
            error_text += key;
            error_text += "' in payload.";
            throw MojoException(error_text);
        }

        return result;
    }

}
