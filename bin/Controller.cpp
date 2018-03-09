#include <stdio.h>


#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/Control.hpp"
#include "crts/Controller.hpp"

#include "Controller.hpp"



std::list<CRTSController *> Controller::controllers;


// CRTS Control(s) is the nob(s) that you access with CRTS Controller.
//
// A CRTS Control is associated with a CRTS Filter.
//
// A CRTS Filter may have zero, one, or more CRTS Controls.


CRTSController::CRTSController(void)
{
    DSPEW();
    Controller::controllers.push_back(this);
}


CRTSController::~CRTSController(void)
{
    Controller::controllers.remove(this);
    DSPEW();
}

CRTSControl *CRTSController::getControl(const char *name)
{
    DASSERT(name && name[0], "");

    WARN("name=\"%s\"", name);

    auto &controls = CRTSController::controller.controls;

    /// STUCK HERE LANCE
    //
    //WTF:  THis suck map::find() does not work!!!!!!!!!!!!!!!! 

    {
        for(auto c : controls)
            WARN("------ name=\"%s\"", c.first);
    }

    CRTSControl *c = 0;

    auto search = controls.find(name);
    if(search != controls.end())
    {
        WARN("FUG");

        c = search->second;
        DASSERT(c, "");
    }

    WARN("got control \"%s\"=%p", name, c);
    return c;
}


Controller CRTSController::controller;

