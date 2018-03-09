#include <map>
#include <list>

class CRTSControl;

class Controller
{
    public:

        std::map<const char *, CRTSControl *> controls;

        static std::list<CRTSController *> controllers;

    friend CRTSControl;
};
