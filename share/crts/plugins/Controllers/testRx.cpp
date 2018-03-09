#include <stdio.h>

#include "crts/debug.h"
#include "crts/Controller.hpp"

class TestRx: public CRTSController
{
    public:

        TestRx(int argc, const char **argv);
        ~TestRx(void) {  DSPEW(); };
};


TestRx::TestRx(int argc, const char **argv)
{
    CRTSControl *c = getControl("rx");

    DSPEW("c=%p", c);
};



// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(TestRx)
