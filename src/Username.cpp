#include "Util.hpp"

namespace Util
{

	// TODO: Use regular expressions for this, which are to be set in
	//		 the config
	std::string getMojoUsername(std::string username, std::string const& prpl)
    {
		std::size_t at_char = username.find('@');

        if (at_char == username.length())
        {
            username.append("@");
            username.append(prpl);
            username.append(".example.com");
        }

		return username;
    }

	std::string getPurpleUsername(std::string const& serviceName, std::string const& username)
    {
		std::size_t pos = username.rfind(".example.com");

		if (pos != std::string::npos)
		{
			pos = username.rfind("@", pos);

			// This or pos +1?
			return username.substr(0, pos);
		}

        return username.data();
    }

}
