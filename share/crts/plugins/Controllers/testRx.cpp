#include <stdio.h>

#include "crts/debug.h"
#include "crts/Controller.hpp"

class TestRx: public CRTSController
{
    public:

        TestRx(int argc, const char **argv);
        ~TestRx(void) {  DSPEW(); };
};

// TODO: make this a command line option
#define RX_CONTROL_NAME  "rx"


TestRx::TestRx(int argc, const char **argv)
{
    CRTSControl *rx = getControl(RX_CONTROL_NAME);

    if(rx == 0) throw "could not find control named \"rx\"";

    DSPEW("rx=%p", rx);
};



// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(TestRx)
