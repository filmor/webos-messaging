#ifndef PURPLE_UI_HPP
#define PURPLE_UI_HPP

#include <purple.h>
#include <core/MojObject.h>

namespace Purple
{
    PurpleNotifyUiOps* getNotifyOps();
    PurpleRequestUiOps* getRequestOps();

    void popEvent(MojObject& var);
    MojErr answerRequest(unsigned id, unsigned answer);
}

#endif
