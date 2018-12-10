#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sendfile.h>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/Control.hpp"
#include "crts/Controller.hpp"
#include "crts/crts.hpp"

#include "Feed.hpp"
#include "FilterModule.hpp"
#include "Thread.hpp"
#include "Stream.hpp"



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


bool CRTSController::printStreamGraphDotPNG64(int fd)
{
    int fd_fromDot = Stream::printGraph(true
        /*wait until dot finishes writing*/);

#if 1 // sendfile() is giving me a hard time.

    ssize_t rd;
    const size_t LEN = 1024*2;
    char buf[1024*2];
    size_t total = 0;
    while((rd = read(fd_fromDot, buf, LEN)) > 0)
    {
        if(write(fd, buf, rd) < 0)
        {
            WARN("write() failed");
            return true;
        }
        total += rd;
    }
    if(rd < 0)
    {
        WARN("sendfile() failed total sent=%zu bytes", total);
        return true;
    }
#else
    ssize_t ssize;
    size_t total = 0;
    errno = 0;
    while((ssize = sendfile(fd, fd_fromDot, 0,
                    (size_t) 1024*1024)) > 0)
        total += ssize;

    close(fd_fromDot);

    if(ssize < 0)
    {
        WARN("sendfile() failed total sent=%zu bytes", total);
        return true;
    }

#endif
    NOTICE("wrote %zu byte PNG base 64 file", total);

    return false; // success.
}

