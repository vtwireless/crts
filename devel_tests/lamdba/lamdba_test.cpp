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


        void addParameter(std::string name, std::function<bool (const double &)> set, std::function<double (void)> get)
        {
            Parameter p;
            p.set = set;
            p.get = get;
            // We store a copy of this parameter.
            Parameters[name] = p;
        };

        bool set(const std::string name, const double &val)
        {
            SPEW();
            bool ret = Parameters[name].set(val);
            SPEW();
            return ret;
        };

        double get(const std::string name)
        {
            SPEW();
            double x = Parameters[name].get();
            SPEW();
            return x;
        };

    private:

        struct Parameter
        {
            public:

                std::function<bool (double)> set;
                std::function<double (void)> get;
        };


        std::map<std::string, Parameter> Parameters;


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


// This is the best interface I've come up with so that a CRTSFilter
// (Filter) super class (Speed) can add parameters and the controller can
// than set and get these parameter values without having anything but the
// string name of the parameter.  The particular filter knows how to set
// and get it's parameter and the controller (filter user) only knows to
// set and get using the string name of the parameter.

class Speed : public Filter
{

    public:

        Speed()
        {
            s = this;
            addParameter("speed", [&](double x) { return setMySpeed(x); }, [&]() { return getMySpeed(); });
            addParameter("double_speed", [&](double x) { return setMySpeed(x); }, [&]() { return 2.0*getMySpeed(); });
            addParameter("bind_speed",  std::bind(&Speed::setMySpeed, this, std::placeholders::_1), std::bind(&Speed::getMySpeed, this));
            addParameter("global_speed",  setSpeed, getSpeed);
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


    const char *names[] = { "speed", "double_speed", "bind_speed", "global_speed", 0 };
    double val = 3.0;

    for(const char **name = names; *name; ++name)
    {
        f.set(*name, val);
        SPEW("%s=%g", *name, f.get(*name));
        val += 1.01;
    }

    return 0;
}
