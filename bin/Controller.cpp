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

CRTSControl *CRTSController::getControl(const std::string name) const
{
    DASSERT(name.length(), "");

    auto &controls = CRTSController::controller.controls;

    CRTSControl *c = 0;

    auto search = controls.find(name);
    if(search != controls.end())
    {
        c = search->second;
        DASSERT(c, "");
    }

    DSPEW("got control \"%s\"=%p", name.c_str(), c);
    return c;
}

// This is a single list of all CRTS controllers we defined here.
Controller CRTSController::controller;
