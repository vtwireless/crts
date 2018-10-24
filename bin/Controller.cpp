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


static uint32_t controllerCreateCount = 0;


// List of all controllers
std::list<CRTSController *> CRTSController::controllers;


CRTSController::CRTSController(void):
    name(0), id(controllerCreateCount++)
{
    DSPEW();
    controllers.push_back(this);
    
    // initialize the getControlIt iterator so that the
    // CRTSController::superclass constructor may iterate through the map
    // list of controls using CRTSController::getControl<>()
    getControlIt = CRTSControl::controls.begin();
}


CRTSController::~CRTSController(void)
{
    controllers.remove(this);
    if(name) free(name);
    DSPEW();
}
