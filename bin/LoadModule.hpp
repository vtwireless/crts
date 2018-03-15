/* The only interface that we need to expose and use in this file is C++
 * template function LoadModule<T>().  The rest of the classes and
 * functions in this file are just directly or indirectly used by function
 * LoadModule<T>().
 */

template <class Base, class Create>
class ModuleLoader
{
    public:
        ModuleLoader(void);
        virtual ~ModuleLoader(void);
        // Returns false on success, true otherwise
        bool loadFile(const char *dso_path, int addLD_flags = 0);
        // create is a function pointer.
        Create create;
        // destroy is a function pointer.
        void *(*destroy)(Base *base);

    private:
        void *dhandle;
};


template <class Base, class Create>
ModuleLoader<Base, Create>::ModuleLoader():create(0),destroy(0)
{
}


template <class Base, class Create>
bool ModuleLoader<Base, Create>::loadFile(const char *dso_path, int addLD_flags)
{
    dhandle = dlopen(dso_path, RTLD_LAZY|addLD_flags);
    if(!dhandle)
    {
        WARN("dlopen() failed: %s", dlerror());
        return true;
    }

    dlerror(); // clear the error.
    void *(*loader)(void **, void **) =
         (void *(*)(void **, void **)) dlsym(dhandle, "loader");
    char *error = dlerror();
    if(error)
    {
        WARN("dlsym() failed: %s", error);
        dlclose(dhandle);
        dhandle = 0;
        return true; // failed
    }

    loader((void **) &create, (void **) &destroy);

    if(!create)
    {
        WARN("failed to get create factory function");
        destroy = 0;
        dlclose(dhandle);
        dhandle = 0;
        return true; // failed
    }
    DSPEW();
    return false; // success
}

template <class Base, class Create>
ModuleLoader<Base, Create>::~ModuleLoader(void)
{
    dlerror(); // clear the error.
    if(dhandle && dlclose(dhandle))
        ERROR("dlclose(%p) failed: %s", dhandle, dlerror());
    DSPEW();
}


static inline char *GetPluginPathFromEnv(const char *category, const char *name)
{
    char *env = 0;

    {
        // Maybe more than one environment variable:
        const char *envs[] = { "CRTS_MODULE_PATH", 0 };
        const char **e = envs;

        for(;*e; ++e)
            if((env = getenv(*e)))
                break;
    }

    // TODO: For now CRTS_MODULE_PATH is just a directory
    // but we need this to work for a ':' separated list
    // of directories.

    if(!env) return env; // failed to find module this way.

    // len = strlen("$env" + '/' + category + '/' + name)
    const ssize_t len = strlen(env) + strlen(category) +
        strlen(name) + 6/* for '//' and ".so" and '\0' */;
    DASSERT(len > 0 && len < 1024*1024, "");
    char *buf = (char *) malloc(len);
    ASSERT(buf, "malloc() failed");

    const char *suffix = ".so";
    if(!strcmp(&name[strlen(name)-3], ".so"))
        suffix = "";
    snprintf(buf, len, "%s/%s/%s%s", env, category, name, suffix);

    if(access(buf, R_OK) == 0)
        return buf; // success, we can access this file.

    free(buf);
    return 0; // failed to access file
}



// The relative paths to crts_program and the module with name name is the
// same in the source and installation is like so:
//
//  program_path =
//  /home/lance/installed/crts-1.0b4/bin/crts_program
//
//  plugin_path =
//  /home/lance/installed/crts-1.0b4/share/crts/plugins/category/name
//
// in source tree:
//
//  /home/lance/git/crts/bin/crts_program
//  /home/lance/git/crts/share/crts/plugins/category/name
//
//
// similarly in a tarball source tree
//

// The returned buffer must be free-d via free().


#define PRE "/share/crts/plugins/"

// A thread-safe path finder looks at /proc/self which is the same as
// /proc/self.  Note this is linux specific code using the linux /proc/
// file system.
//
// The returned pointer must be free()ed.
static inline char *GetPluginPath(const char *category, const char *name)
{
    DASSERT(name && strlen(name) >= 1, "");
    DASSERT(category && strlen(category) >= 1, "");

    char *buf;

    if((buf = GetPluginPathFromEnv(category, name)))
        return buf;

    // postLen = strlen("/share/crts/plugins/" + category + '/' + name)
    const ssize_t postLen =
        strlen(PRE) + strlen(category) +
        strlen(name) + 5/* for '/' and ".so" and '\0' */;
    DASSERT(postLen > 0 && postLen < 1024*1024, "");
    ssize_t bufLen = 128 + postLen;
    buf = (char *) malloc(bufLen);
    ASSERT(buf, "malloc() failed");
    ssize_t rl = readlink("/proc/self/exe", buf, bufLen);
    ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    while(rl + postLen >= bufLen)
    {
        DASSERT(bufLen < 1024*1024, ""); // it should not get this large.
        buf = (char *) realloc(buf, bufLen += 128);
        ASSERT(buf, "realloc() failed");
        rl = readlink("/proc/self/exe", buf, bufLen);
        ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    }

    buf[rl] = '\0';
    // Now buf = path to program binary.
    //
    // Now strip off to after "/bin/crts_" (Linux path separator)
    // by backing up two '/' chars.
    --rl;
    while(rl > 5 && buf[rl] != '/') --rl; // one '/'
    ASSERT(buf[rl] == '/', "");
    --rl;
    while(rl > 5 && buf[rl] != '/') --rl; // two '/'
    ASSERT(buf[rl] == '/', "");
    buf[rl] = '\0'; // null terminate string

    // Now
    //
    //     buf = "/home/lance/installed/crts-1.0b4"
    // 
    //  or
    //     
    //     buf = "/home/lance/git/crts"

    // Now just add the postfix to buf.
    //
    // If we counted chars correctly strcat() should be safe.

    DASSERT(strlen(buf) + strlen(PRE) +
            strlen(category) + strlen("/") +
            strlen(name) + 1 < (size_t) bufLen, "");

    strcat(buf, PRE);
    strcat(buf, category);
    strcat(buf, "/");
    strcat(buf, name);

    // Reuse the bufLen variable.

    bufLen = strlen(buf);
    if(strcmp(&buf[bufLen-3], ".so"))
        strcat(buf, ".so");

    return buf;
}



// This is the only interface in this file that we wish to expose,
// but C++ templates suck, so we expose lots of stuff.
/*
 *  Example usage:


    void *(*destroyIO)(CRTSRadioIO *) = 0;
    CRTSRadioIO *io =
        LoadModule<CRTSRadioIO>("stdin", "RadioIO", argc, argv, destroyIO);

   if(!io)
   {
      ERROR("Loading module \"stdin\" failed");
      return 1; // error
   }

   INFO("Loaded module: \"stdin\"");

   // Do stuff


   if(destroyIO)
     destroyIO(io);

*/
template <class T>
T *LoadModule(const char *path, const char *category, int argc,
        const char **argv, void *(*&destroyer)(T *),
        int dlflags = RTLD_NODELETE|RTLD_NOW|RTLD_DEEPBIND)
{
    DASSERT(path, "");
    DASSERT(category, "");
    DASSERT(&destroyer, "");

    destroyer = 0;
    T *ret = 0;


    path = GetPluginPath(category, path);
    // GCC bug: This DASSERT prints the wrong filename, because this
    // is a template function; __BASE_FILE__ is not module_util.hpp.
    // But __LINE__ is correct.  Not a show stopper, but makes debugging
    // harder.
    DASSERT(path, "");

    DSPEW("loading module: \"%s\"", path);

    ModuleLoader<T,T *(*)(int argc, const char **argv)> moduleLoader;

    if(moduleLoader.loadFile(path, dlflags))
                // See 'man dlopen(3)' for what these option
                // flags mean.
            //RTLD_NODELETE|RTLD_GLOBAL|RTLD_NOW|RTLD_DEEPBIND))
            // The RTLD_GLOBAL flag may be a bad idea.
            //RTLD_NODELETE|RTLD_NOW|RTLD_DEEPBIND))
    {
        ERROR("Failed to load module plugin \"%s\"", path);
        return 0; // fail
    }


    try
    {
        ret = moduleLoader.create(argc, argv);
        ASSERT(ret, "");
        destroyer = moduleLoader.destroy;
        DASSERT(destroyer, "");
    }
    catch(const char *msg)
    {
        // This is what I hate about C++.
        //
        // Do I call the destructor or not?

        ERROR("Failed to load module: \"%s\"\n"
                "exception thrown: \"%s\"", path, msg);

        if(ret && destroyer)
            destroyer(ret);
        ret = 0; // fail
        destroyer = 0;
    }
    catch(std::string msg)
    {
        ERROR("Failed to load module: \"%s\"\n"
                "exception thrown: \"%s\"", path, msg.c_str());
    }
    catch(...)
    {
        ERROR("Failed to load module \"%s\""
                " an exception was thrown", path);

        if(ret && destroyer)
            destroyer(ret);
        ret = 0; // fail
        destroyer = 0;
    }

    free((void *) path);

    return ret;
}

