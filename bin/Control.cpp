#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stack>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp"
#include "crts/Control.hpp"
#include "crts/Controller.hpp"
#include "pthread_wrappers.h"
#include "Feed.hpp"
#include "FilterModule.hpp"


static uint32_t createCount = 0;


// Global list of all CRTSControl objects:
std::map<std::string, CRTSControl *> CRTSControl::controls;


CRTSControl::CRTSControl(CRTSFilter *filter_in, std::string name_in):
    filter(filter_in)
{
    ASSERT(name_in.length(), "");
    name = strdup(name_in.c_str());
    ASSERT(name, "strdup() failed");
    DASSERT(filter, "");

    auto &controls = CRTSControl::controls;

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

    id = createCount++;

    DSPEW("Added CRTS Control(%" PRIu32 ") named \"%s\"", id, name);
}

CRTSControl::~CRTSControl(void)
{
    DASSERT(name, "");
    DASSERT(name[0], "");

    // Remove this from the list of controls for this filter.
    filter->controls.erase(name);

    // Remove this from the list of all controls.
    CRTSControl::controls.erase(name);

    DSPEW("Removing CRTS control named \"%s\"", name);

    free(name);
}
