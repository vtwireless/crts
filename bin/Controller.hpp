#include <string>
#include <map>
#include <list>

class CRTSControl;

class Controller
{
    public:

        std::map<std::string, CRTSControl *> controls;

        static std::list<CRTSController *> controllers;

    friend CRTSControl;
};
