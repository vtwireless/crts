#include <stdio.h>

#include "crts/debug.h"
#include "crts/Module.hpp"


class Test : public CRTSModule
{
    public:

        Test(int argc, const char **argv);
        ~Test(void);
};

Test::Test(int argc, const char **argv) { DSPEW(); }

Test::~Test(void) { DSPEW(); }


// Define the module loader stuff to make one of these class objects.
CRTSMODULE_MAKE_MODULE(Test)
