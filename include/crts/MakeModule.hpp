#ifndef __CRTS_MakeModule_H__
#define __CRTS_MakeModule_H__


// CPP Macro Module Loader Function
//
// A module must define the two object factory functions: createObj() and
// destroyObj() and loader() by using this MACRO function.   The exposed
// symbol is loader() which we get with the dynamic shared object loading
// API functions dlopen(3), dlsym(3), and dlclose(3).  The argument
// derived_class_name is the derived class name which must publicly
// inherit the base class named base_class_name.  Call this macro outside
// all code blocks without a tailing semi-colon.

/* For Example:
 *
 * MyMod.cpp

// crts/BaseMod.hpp should include crts/MakeModule.hpp:
//#include <crts/MakeModule.hpp>

#inlcude <crts/BaseMod.hpp>


class MyModule : public BaseMod
{
    public:

        MyModule(int argc, const char **argv);

        void execute(void);
};

MyModule::MyModule(int argc, const char **argv)
{
    // Parse argv.
}

void MyModule::execute(void)
{ 
    // Do stuff.
}

CRTS_MAKE_MODULE(BaseMod, MyModule)

 */
#define CRTS_MAKE_MODULE(base_class_name, derived_class_name) \
    static base_class_name *createObj(int argc, const char **argv)\
    {\
        return new derived_class_name(argc, argv);\
    }\
    static void destroyObj(base_class_name *obj)\
    {\
        delete obj;\
    }\
    extern "C"\
    {\
        const char *crtsModuleBaseClassName = "#base_class_name";\
        \
        void loader(void **c, void **d)\
        {\
            *c = (void *) createObj;\
            *d = (void *) destroyObj;\
        }\
    }

#endif // #ifndef __CRTS_MakeModule_H__
