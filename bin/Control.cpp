#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stack>
#include <string>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/Control.hpp"
#include "crts/Controller.hpp"
#include "Controller.hpp"
#include "FilterModule.hpp"



CRTSControl::CRTSControl(CRTSFilter *filter_in, const char *name_in):
    filter(filter_in)
{
    ASSERT(name_in, "");
    ASSERT(name_in[0], "");
    name = strdup(name_in);
    ASSERT(name, "strdup() failed");
    DASSERT(filter, "");

    auto &controls = CRTSController::controller.controls;

    if(controls.find(name) != controls.end())
    {
        WARN("A control named \"%s\" is already loaded", name);
        std::string str = "A control named \"";
        str += name;
        str += "\" is already loaded";
        free(name);
        throw str;
    }

    // Add this to the list of all controls.
    controls[name] = this;

    // Add to the list of controls for this filter.
    filter->controls[name] = this;

    DSPEW("Added CRTS Control named \"%s\"", name);
}

CRTSControl::~CRTSControl(void)
{
    DASSERT(name, "");
    DASSERT(name[0], "");

    // Remove this from the list of controls for this filter.
    filter->controls.erase(name);
    
    // Remove this from the list of all controls.
    CRTSController::controller.controls.erase(name);

    free(name);

    DSPEW();
}
