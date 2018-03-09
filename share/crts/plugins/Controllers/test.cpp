#include <stdio.h>

#include "crts/debug.h"
#include "crts/Controller.hpp"

class Test: public CRTSController
{
    public:

        Test(int argc, const char **argv) { DSPEW(); };
        ~Test(void) {  DSPEW(); };
};


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(Test)
