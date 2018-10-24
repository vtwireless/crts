// This is the best interface I've come up with, so that a CRTSFilter
// (Filter) super class (Speed) can add parameters, and the controller can
// then set and get these parameter values without having anything but the
// string name of the parameter.  The particular filter knows how to set
// and get it's parameter and the controller (filter user) only knows the
// string name of the parameter and can set and get it will generic set()
// and get() functions.   These generic set() and get() will be a control
// that is set up by the filter and is accessible by the Controller.  In
// this example the Filter is the Control, but in CRTS the Control is
// managed by the Filter base class.


#include <stdio.h>

#include <string>
#include <map>
#include <functional>

#include "crts/debug.h"

class Filter
{
    public:

        Filter(void);
        virtual ~Filter(void) { SPEW(); };


        bool set(const std::string name, const double &val)
        {
            SPEW();
            bool ret = parameters[name].set(val);
            SPEW();
            return ret;
        };

        double get(const std::string name)
        {
            SPEW();
            double x = parameters[name].get();
            SPEW();
            return x;
        };

    protected:

        void addParameter(std::string name, std::function<bool (const double &)> set, std::function<double (void)> get)
        {
            Parameter p;
            p.set = set;
            p.get = get;
            // We store a copy of this parameter.
            parameters[name] = p;
        };

    private:

        struct Parameter
        {
            std::function<bool (double)> set;
            std::function<double (void)> get;
        };


        std::map<std::string, Parameter> parameters;
};


Filter::Filter(void)
{
    SPEW();
}


// Another way to do this using global functions.
class Speed;
static bool setSpeed(const double &val);
static double getSpeed(void);

Speed *s = 0;




class Speed : public Filter
{

    public:

        Speed()
        {
            // Here's two examples of using addParameter() with a lambda
            // function:
            addParameter("speed", [&](double x) { return setMySpeed(x); }, [&]() { return getMySpeed(); });
            addParameter("double_speed", [&](double x) { return setMySpeed(x); }, [&]() { return 2.0*getMySpeed(); });
            // Now try using a std::bind thingy with addParameter():
            addParameter("bind_speed",  std::bind(&Speed::setMySpeed, this, std::placeholders::_1), std::bind(&Speed::getMySpeed, this));
            // Now try using a global functions setSpeed(), getSpeed().
            s = this;
            addParameter("global_speed", setSpeed, getSpeed);
        };

        ~Speed()
        {
            SPEW();
            s = 0;
        };


    private:

        double speed;

        bool setMySpeed(const double &x)
        {
            // For a real world case this will be much more complex.
            speed = x;

            SPEW();
            return true;
        };

        double getMySpeed(void)
        {
            SPEW();
            // For a real world case this could be much more complex.
            return speed;
        };

        friend bool setSpeed(const double &val);
        friend double getSpeed(void);
};

static bool setSpeed(const double &val)
{
    return s->setMySpeed(val);
}

static double getSpeed(void)
{
    return s->getMySpeed();
}





int main(void)
{

    Speed speed;

    Filter &f = speed;


    // In this case main() is my controller code starts here:
    //
    const char *names[] = { "speed", "double_speed", "bind_speed", "global_speed", 0 };
    double val = 3.0;

    for(const char **name = names; *name; ++name)
    {
        // Here we can think of f as the control.
        //
        f.set(*name, val);
        SPEW("%s=%g", *name, f.get(*name));
        val += 1.01;
    }

    return 0;
}
