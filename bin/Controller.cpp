#include <stdio.h>


#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/Control.hpp"
#include "crts/Controller.hpp"


// CRTS Control(s) is the nob(s) that you access with CRTS Controller.
//
// A CRTS Control is associated with a CRTS Filter.
//
// A CRTS Filter may have zero, one, or more CRTS Controls.


// List of all controls
std::map<std::string, CRTSControl *> CRTSController::controls;

// List of all controllers
std::list<CRTSController *> CRTSController::controllers;



CRTSController::CRTSController(void)
{
    DSPEW();
    controllers.push_back(this);
}


CRTSController::~CRTSController(void)
{
    controllers.remove(this);
    DSPEW();
}
