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
    filter(filter_in), isRunning(filter->stream->isRunning)
{
    ASSERT(name_in.length(), "");
    name = strdup(name_in.c_str());
    ASSERT(name, "strdup() failed");
    DASSERT(filter, "");

    // Initialize the getNextParameterName() iterator.
    getNextParameterNameIt = filter->parameters.end();

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

    // Add control for this filter.
    filter->control = this;

    id = createCount++;

    DSPEW("Added CRTS Control(%" PRIu32 ") named \"%s\"", id, name);
}

CRTSControl::~CRTSControl(void)
{
    DASSERT(name, "");
    DASSERT(name[0], "");

    // Remove this pointer from the filter, so the filter will not
    // think it needs to delete this control.  Very important if this
    // is a statically declared object the filter writer made.
    filter->control = 0;

    // Remove this from the list of all controls in all streams.
    CRTSControl::controls.erase(name);

    DSPEW("Removing CRTS control named \"%s\"", name);

    free(name);
}
