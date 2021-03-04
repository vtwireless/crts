#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdlib.h>
#include <map>
#include <list>
#include <string>
#include <stack>
#include <atomic>

#include "crts/debug.h"
#include "crts/crts.hpp"

#include "get_opt.hpp"
#include "pthread_wrappers.h" // some pthread_*() wrappers

#include "crts/crts.hpp"
#include "crts/Filter.hpp"
#include "crts/Module.hpp"
#include "Feed.hpp"
#include "FilterModule.hpp"
#include "Thread.hpp"
#include "Stream.hpp"
#include "LoadModule.hpp"
#include "crts_radio_usage.hpp"


// We can add signals to this list that is 0 terminated.  Signals that we
// use to gracefully exit with, that is catch the signal and then set the
// atomic flags and wait for the threads to finish the last loop, if they
// have not already.
//
// These signal cannot include the THREAD_EXIT_SIG defined in Thread.hpp.
// The THREAD_EXIT_SIG is used by worker (CRTSFilter) threads.
//
static const int exitSignals[] =
{
    SIGINT, // from Ctrl-C in a terminal.
    SIGTERM, // default kill signal
    // We must catch SIGPIPE as an exit signal for the case when we are
    // reading or writing a UNIX bash pipe line; otherwise a broken pipe
    // will not let us cleanly exit.
    SIGPIPE,
    //
    // TODO: add more clean exit signal numbers here.
    //
    0/*0 terminator*/
};


static pthread_t exitSignalThread;


// TODO: Extend this to work for the case when there is more than one
// stream.
//
// This is a module user interface that may be called from another thread.
//
// Try to gracefully exit.
void crtsExit(void)
{
    errno = 0;
    // We signal using just the first exit signal in the list.
    INFO("Sending signal %d to exit signal thread", exitSignals[0]);
    errno = pthread_kill(exitSignalThread, exitSignals[0]);
    // All we could do is try and report.
    WARN("Signal %d sent to exit signal thread", exitSignals[0]);

    // TODO: cleanup all thread and processes

    for(auto stream : Stream::streams)
        // Let it finish the last loop:
        stream->isRunning = false;
}



static void badSigCatcher(int sig)
{
    ASSERT(0, "caught signal %d", sig);
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//
// We went to great lengths to make it so that crts_radio can cleanly exit
// due to the exit signals in exitSignals[].
//
// This signalExitThreadIsRunning variable is only accessed by the exit
// signal catcher thread.
//

static bool signalExitThreadIsRunning = true;

static pthread_barrier_t *startupBarrier = 0;

static void signalExitProgramCatcher(int sig)
{
    INFO("Caught signal %d exiting now", sig);

    // To keep this re-entrant we can only set the
    // signalExitThreadIsRunning value here.  By adding this extra
    // signalExitThreadIsRunning flag we keep this re-entrant.  Calling a
    // Stream method, with it's mutex locking, would not be safe in
    // general.
    signalExitThreadIsRunning = false;

    // I give up.  Signals are hanging this program.  Just exit for crying
    // out loud.  TODO: Is this success, exit(0), or failure,
    // exit(nonzero)?
    exit(0);
}

// We have yet another thread call this, because it catches the exit
// signal, and signal handlers and pthread synchronization primitives do
// not mix well.  The alternative would be to add timeouts to the pthread
// synchronization calls, but that's not a clean design; that's bad
// design, kind-of like polling.
//
static void *signalExitThreadCB(void *ptr)
{
    DASSERT(startupBarrier, "");
    sigset_t exitSigs;
    ASSERT(sigemptyset(&exitSigs) == 0, "");
    //ASSERT(sigfillset(&exitSigs) == 0, "");
    for(int i=0; exitSignals[i]; ++i)
        ASSERT(sigaddset(&exitSigs, exitSignals[i]) == 0, "");

    // This thread can handle itself.  If the main thread exits we do not
    // care, this will just terminate with it.  The Stream object will
    // stay consistent since Stream::signalMainThreadCleanup() calls a
    // mutex lock in it, which will keep the main thread from exiting if
    // this thread races to call Stream::signalMainThreadCleanup() before
    // or after the main thread shuts down.  Also having this thread
    // terminate in the middle of calling
    // Stream::signalMainThreadCleanup() is okay,
    // Stream::signalMainThreadCleanup() never makes any new system
    // resources.
    //
    // Now if this thread required a join with the main thread, then that
    // would be a problem because then we'd need a way the main thread
    // could check if there was a exit signal which would require the use
    // of a pthread thread synchronization primitive, which could dead
    // lock in the signal catcher.
    ASSERT(pthread_detach(pthread_self()) == 0, "");

    BARRIER_WAIT(startupBarrier);

    // Let this thread catch these "exit" signals now.
    ASSERT(sigprocmask(SIG_UNBLOCK, &exitSigs, 0) == 0, "");

    // We know that all threads that are created by crts_radio are now
    // running.  There may be other threads from other libraries like
    // libuhd from loaded filter modules.

    // Loop forever or until we catch a exit signal or the main thread
    // returns (exits).  If we never catch a signal that's okay, this
    // thread will just get terminated when the main thread returns.
    DSPEW("Exit signal catcher thread pausing");
    while(signalExitThreadIsRunning) pause();
    DSPEW("Exit signal catcher thread cleaning up");
    Stream::signalMainThreadCleanup();

    return 0;
}
// End stuff that the signal catcher thread uses.
//
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


static int setDefaultStreamConnections(Stream* &stream)
{
    // default connections: 0 1   1 2   2 3   3 4   4 5  ...
    // default connections: 0 -> 1   1 -> 2   2 -> 3   3 -> 4   4 -> 5  ...

    DASSERT(stream->haveConnections == false, "");

    uint32_t from = 0;

    for(auto toFilterModule : stream->map)
        if(toFilterModule.second->loadIndex > 0)
            if(stream->connect(from++, toFilterModule.second->loadIndex))
                return 1; // fail

    // We set this flag here in case there was just one filter and
    // not really any connections.
    stream->haveConnections = true;

    // It just so happens we only call this directly or indirectly from
    // parseArgs where we make a new stream after each time we set default
    // connections, so setting stream = 0 tells us that:
    stream = 0;

    return 0; // success
}


// We happened to have more than one command line option that prints a DOT
// graph. So we put common code here to keep things consistent.
static inline int doPrint(Stream* &stream, const char *filename = 0,
        bool _wait = true)
{
    if(stream)
    {
        DASSERT(!stream->haveConnections, "");
        if(setDefaultStreamConnections(stream))
            return 1; // failure
    }

    // Finish building the default thread groupings for all streams.
    Stream::finishThreads();

    if(Stream::printGraph(filename, _wait))
        return 1; // failure

    return 0; // success
}


// Container for CRTSModule objects and its destroy function.
class Module
{
    public:

        Module(CRTSModule *m, void *(*d)(CRTSModule *));

        CRTSModule *module;
        void *(*destroyModule)(CRTSModule *);
};

// List of CRTSModule objects that we loaded.
//
// This modules different from CRTSFilter modules in that they have a more
// global scope, i.e. they share symbols with the whole process, and they
// are not tied to a particular stream.
//
// Thread safety may be very important in these objects given they are
// "globally" accessible by all CRTSFilters.
//
// The CRTSModule objects are destroyed after all the streams.
//
// We keep a map of loaded modules keyed by unique name string, so we can
// prevent a module with the same name from getting loaded more than once.
static std::map<std::string, Module *> modulesMap;

// In addition to key by name we need the insertion/load order of the
// modules in a list.  std::map does not keep insertion order.
static std::list<Module *> modulesList;

CRTSModule::CRTSModule() { DSPEW(); }
CRTSModule::~CRTSModule(void) { DSPEW(); }

Module::Module(CRTSModule *m, void *(*d)(CRTSModule *)):
    module(m), destroyModule(d)
{ 
    DSPEW();
}


static void cleanupModules(void)
{
    for(auto m = modulesList.rbegin(); m != modulesList.rend(); ++m)
    {
        (*m)->destroyModule((*m)->module);
        delete (*m);
    }

    // empty the list and map
    modulesList.clear();
    modulesMap.clear();
}


// crtsRequireModule() is exported to the CRTSFilter and CRTSModule
// codes.
//
// Returns false on success.
bool crtsRequireModule(const char *name, int argc, const char **argv)
{
    DSPEW("got option load general module:  %s", name);

    std::string str = name;

    if(modulesMap.find(str) != modulesMap.end())
    {
        NOTICE("general module \"%s\" is already loaded", name);
        return false; // success
    }


    // TODO: add a requireModule() method to CRTSFilter
    // which will load these kinds of modules if the CRTSFilter
    // needs them.
    void *(*destroyModule)(CRTSModule *) = 0;

    CRTSModule *module = LoadModule<CRTSModule>(name, "General",
            argc, argv, destroyModule,
            RTLD_NODELETE|RTLD_GLOBAL|RTLD_NOW|RTLD_DEEPBIND);

    if(!module || !destroyModule)
        return true; // Fail

    Module *m = new Module(module, destroyModule);

    // We keep two lists: a std::map and a map::list
    modulesMap[str] = m;
    modulesList.push_back(m);

    return false;
}


// This number is just so the module writers can't access these two
// accidentally exposed functions removeCRTSCControllers() and
// LoadCRTSController().
#define CONTROLLER_MAGIC  0x3F237301


void removeCRTSCControllers(uint32_t magic)
{
    // Do not let module writers use this function.
    ASSERT(magic == CONTROLLER_MAGIC, "This is not a module writer interface.");

    while(CRTSController::controllers.size())
    {
        CRTSController *c = CRTSController::controllers.back();
        DASSERT(c, "");
        DASSERT(c->destroyController, "");
        // The CRTSController::~CRTSController() will remove
        // itself from this Controller::controllers list.
        c->destroyController(c);
    }
}


// TODO: This function is exposed as extern in crts/Controller.hpp and
// that's a terrible hack.  The user will not know the magic number, so if
// they try to use this it will fail as we would prefer.
//
int LoadCRTSController(const char *name, int argc, const char **argv, uint32_t magic)
{
    // Do not let module writers use this function.
    DASSERT(name && name[0], "");
    ASSERT(magic == CONTROLLER_MAGIC, "This is not a module writer interface.");

    void *(*destroyController)(CRTSController *);

    CRTSController *crtsController =
        LoadModule<CRTSController>(name, "Controllers",
                argc, argv, destroyController);

    // TODO: note that the Super class CRTSController object will not see
    // this name.
    crtsController->name = strdup(name);
    ASSERT(name, "strdup() failed");

    if(!crtsController || !destroyController)
        return 1; // fail

    // We set this now after the loader, LoadModule<>(), got it for us.
    crtsController->destroyController = destroyController;

    return 0; // Success
}


/////////////////////////////////////////////////////////////////////
// Parsing command line arguments
//
//  Butt ugly, but straight forward
/////////////////////////////////////////////////////////////////////
//
static int parseArgs(int argc, const char **argv)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    // Current stream as a pointer.  There are none yet.
    Stream *stream = 0;

    int i = 1;
    if(i >= argc)
        return usage(argv[0]);

    while(i < argc)
    {
        // These may get set if we have the necessary options:
        std::string str;
        int Argc = 0;
        const char **Argv = 0;
        double rx_freq = 0; // TODO: remove this.

        if(get_opt(str, Argc, Argv, "-f", "--filter", argc, argv, i))
        {
            if(!stream)
                // We add filters a new "current" stream until we add
                // connections.
                stream = new Stream;

            DSPEW("got optional arg filter:  %s", str.c_str());

            // Add the filter to the current stream in the command-line.
            if(stream->load(str.c_str(), Argc, Argv))
                return 1;

            continue;
        }

        if(get_opt(str, Argc, Argv, "-C", "--controller", argc, argv, i))
        {
            if(LoadCRTSController(str.c_str(), Argc, Argv, CONTROLLER_MAGIC))
                return 1;
            continue;
        }

        if(get_opt(str, Argc, Argv, "-l", "--load", argc, argv, i))
        {
            if(crtsRequireModule(str.c_str(), Argc, Argv))
                return 1; // fail
            continue;
        }

        if((!strcmp("-t", argv[i]) || !strcmp("--thread", argv[i])))
        {
            if(!stream)
            {
                ERROR("At command line argument \"%s\": No "
                        "filters have been given"
                        " yet for the current stream", argv[i]);
                return 1; // failure
            }

            ++i;

            Thread *thread = 0;

             while(i < argc && argv[i][0] >= '0' && argv[i][0] <= '9')
             {
                // the follow args are like:  0 1 2 ...
                uint32_t fi; // filter index.
                errno = 0;
                fi = strtoul(argv[i], 0, 10);
                if(errno || fi > 0xFFFFFFF0)
                    return usage(argv[0], argv[i]);

                auto it = stream->map.find(fi);
                if(it == stream->map.end())
                {
                    ERROR("Bad filter index options: %s %" PRIu32, argv[i-1], fi);
                    return usage(argv[0], argv[i-1]);
                }
                FilterModule *filterModule = it->second;

                DASSERT(filterModule, "");
                DASSERT(!filterModule->thread,
                        "filter module \"%s\" already has a thread",
                        filterModule->name.c_str());

                if(filterModule->thread)
                {
                    ERROR("filter module \"%s\" already has a thread",
                        filterModule->name.c_str());
                    return 1; // Failure
                }

                if(!thread)
                    thread = new Thread(stream);

                // This filterModule is a member of this thread group.
                thread->addFilterModule(filterModule);

                ++i;
            }

            if(!thread)
            {
                ERROR("At command line argument \"%s\": No "
                        "valid filter load indexes given", argv[i-1]);
                return usage(argv[0], argv[i-1]);
            }


            // TODO: check the order of the filters in the thread
            // and make sure that it can work in that order...
            continue;
        }


        if((!strcmp("-c", argv[i]) || !strcmp("--connect", argv[i])))
        {
            if(!stream)
            {
                ERROR("At command line argument \"%s\": No "
                        "filters have been given"
                        " yet for the current stream", argv[i]);
                return 1; // failure
            }

            ++i;

            // We read the "from" and "to" channel indexes in pairs
            while(i + 1 < argc &&
                    argv[i][0] >= '0' && argv[i][0] <= '9' &&
                    argv[i+1][0] >= '0' && argv[i+1][0] <= '9')
            {
                // the follow args are like:  0 1 1 2
                // We get them in pairs like: 0 1 and 1 2
                uint32_t from, to;
                errno = 0;
                from = strtoul(argv[i], 0, 10);
                if(errno || from > 0xFFFFFFF0)
                    return usage(argv[0], argv[i]);
                ++i;
                to = strtoul(argv[i], 0, 10);
                if(errno || to > 0xFFFFFFF0)
                    return usage(argv[0], argv[i]);
                // Connect filters in the current stream.
                if(stream->connect(from, to))
                    return usage(argv[0], argv[i]);
                ++i;
            }

            if(!stream->haveConnections)
            {
                if(setDefaultStreamConnections(stream))
                    return 1; // failure
                // setDefaultStreamConnections(stream) set stream = 0
            }
            else
            {
                // TODO: Add a check of all the connections in the stream
                // here.  This is the only place we need to make sure they
                // are connected in a way that make sense.
                //
                // We are done with this stream.  We'll make a new stream
                // if the argument options require it.
                stream = 0;
            }

            // Ready to make a another stream if more filters are added.
            // The global Stream::streams will keep the list of streams
            // for us.
            continue;
        }

        if(get_opt(str, Argc, Argv, "-p", "--print", argc, argv, i))
        {
            // This case will have a filename picked out.
            if(doPrint(stream, str.c_str()))
                return 1; // error
            continue;
        }

        if(!strcmp("-d", argv[i]) || !strcmp("--display", argv[i]))
        {
            ++i;
            // This case will not have a filename so it will
            // use a tmp file and automatically remove it.
            if(doPrint(stream, str.c_str()))
                return 1; // error
            continue;
        }

        if(!strcmp("-D", argv[i]) || !strcmp("--Display", argv[i]))
        {
            ++i;
            // This case will not have a filename so it will
            // use a tmp file and automatically remove it.
            if(doPrint(stream, str.c_str(), false/*do not wait*/))
                return 1; // error
            continue;
        }

        if(get_opt_double(rx_freq, "-f", "--rx-freq", argc, argv, i))
            // TODO: remove this if() block. Just need as example of
            // getting a double from command line argument option.
            continue;

        if(!strcmp("-e", argv[i]) || !strcmp("--exit", argv[i]))
        {
            DSPEW("Now exiting due to exit command line option");
            exit(0);
        }

        if(!strcmp("-h", argv[i]) || !strcmp("--help", argv[i]))
            return usage(argv[0]);

        return usage(argv[0], argv[i]);

        //++i;
    }

    if(stream)
    {
        DASSERT(!stream->haveConnections, "");
        setDefaultStreamConnections(stream);
    }

    // TODO: These last steps need to be made part of Stream::startAll(),
    // so that we may have restart from start() and stop().

    // Figure out which filter modules have no writers, i.e. are sources.
    for(auto stream : Stream::streams)
        stream->getSources();

    // Finish building the default thread groupings for all streams.
    Stream::finishThreads();

    // Now we add Feed source CRTSFilter objects to all the source
    // CRTSFilter objects.
    for(auto stream : Stream::streams)
        for(auto filterModule : stream->sources)
        {
            CRTSFilter *feed = new Feed;
            FilterModule *feedModule = stream->load(feed, 0, "feed");
            stream->connect(feedModule, filterModule);
            // The feed will use the thread of the source filter
            // it is feeding.
            filterModule->thread->addFilterModule(feedModule);
        }

    // So now the old sources we had are now no longer sources.  They have
    // a feed source.  We need to get/find the sources that are feeds now.
    for(auto stream : Stream::streams)
        stream->getSources();

    // See what the Feeds added to the graph.
    //Stream::printGraph(0, true);


    // TODO: Add checking of module connectivity, so that they make sense.
    // For example: a directed graph that is a ring will not run, because
    // it has no source to start it.  A ring could work if there was
    // source filter injecting into it, but in general the buffering could
    // end up infinitely long.  We need to find the rules that keep this
    // stream from failing.

    return 0; // success
}




int main(int argc, const char **argv)
{
    {
        // Setup the some signal catchers.
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = badSigCatcher;
        act.sa_flags = SA_RESETHAND;
#if 1
        // block all signals when in the signal handlers.
        sigfillset(&act.sa_mask);
#endif
        errno = 0;
        // Set a signal handler for "bad" signals so we can debug it and
        // see what was wrong if we're lucky.
        ASSERT(sigaction(SIGSEGV, &act, 0) == 0, "");
        ASSERT(sigaction(SIGABRT, &act, 0) == 0, "");
        ASSERT(sigaction(SIGFPE, &act, 0) == 0, "");

        // Now setup regular (not bad) exit signals.
        act.sa_handler = signalExitProgramCatcher;
        for(int i=0; exitSignals[i]; ++i)
            ASSERT(sigaction(exitSignals[i], &act, 0) == 0, "");

        // Compose a set of signals to block in the other threads.
        sigset_t exitSigs;
        ASSERT(sigemptyset(&exitSigs) == 0, "");
        //ASSERT(sigfillset(&exitSigs) == 0, "");
        for(int i=0; exitSignals[i]; ++i)
        {
            //WARN("blocking sig=%d", exitSignals[i]);
            ASSERT(sigaddset(&exitSigs, exitSignals[i]) == 0, "");
        }
        // The other thread will inherit this mask that we set now.
        ASSERT(sigprocmask(SIG_BLOCK, &exitSigs, 0) == 0, "");
    }

#if 0
    // Testing segfault catcher by setting bad memory address
    // to an int value.
    void *tret = 0;
    *(int *) (((uintptr_t) tret) + 0xfffffffffffffff8) = 1;
#endif


#if 0 // TODO: this code may be kruft, but could be used later.
    {
        // TODO: this code may be kruft, but could be used later.

        // We must not let the threads created by the UHD API catch the
        // exit signals, so they will inherit the blocking of the exit
        // signals after we set them here in the main thread.
        sigset_t sigSet;
        sigemptyset(&sigSet);
        for(int i=0; exitSignals[i]; ++i)
            sigaddset(&sigSet, exitSignals[i]);
        errno = pthread_sigmask(SIG_BLOCK, &sigSet, NULL);
        ASSERT(errno == 0, "pthread_sigmask() failed");
    }
#endif


    if(parseArgs(argc, argv))
    {
        // We failed so cleanup
        //
        // There should be not much go'n yet just filter modules
        // loaded.  We just need to call their factory destructor's.
        for(auto stream : Stream::streams)
            // Flag the streams as not running in regular mode.
            stream->isRunning = false;

        Stream::destroyStreams();

        cleanupModules();

        DSPEW("EXITING due to startup error");
        return 1; // return failure status
    }

    if(Stream::streams.size() == 0)
    {
        cleanupModules();
        return usage(argv[0]);
    }

    ///////////////////////////////////////////////////////////////////
    // TODO: Check that connections in the stream make sense ???
    ///////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////////////
    // We make sure no two sources share the same thread.
    //
    // A dumb user can do that via the command line.
    //
    for(auto stream : Stream::streams)
        for(auto filterModule : stream->sources)
            // TODO: don't double search the lists.
            for(auto filterModule2 : stream->sources)
                if(filterModule != filterModule2 &&
                    filterModule->thread == filterModule2->thread)
                {
                    ERROR("The two source filters: \"%s\" and \"%s\""
                            " share the same thread %" PRIu32".\n\n",
                        filterModule->name.c_str(),
                        filterModule2->name.c_str(),
                        filterModule2->thread->threadNum);

                    Stream::destroyStreams();

                    cleanupModules();

                    DSPEW("EXITING due to startup error");
                    return 1; // error fail exit.
                }
    //
    ///////////////////////////////////////////////////////////////////


    // Call the CRTSFilter::start() functions.
    //
    if(Stream::startAll())
    {
        Stream::destroyStreams();
        cleanupModules();
        DSPEW("EXITING due to startup error");
        return 1; // error fail exit.
    }


    DASSERT(Thread::createCount, "");

    {
        // We just use this barrier once at the starting of the threads,
        // so we use this stack memory to create and use it.
        pthread_barrier_t barrier;
        startupBarrier = &barrier;

        DSPEW("have %zu threads", Thread::getTotalNumThreads());
        ASSERT((errno = pthread_barrier_init(&barrier, 0,
                    Thread::getTotalNumThreads() + 2)) == 0, "");


        // Start the threads.
        for(auto stream : Stream::streams)
            for(auto thread : stream->threads)
                // Launch the threads via pthread_create():
                thread->launch(&barrier);

        MUTEX_LOCK(&Stream::mutex);

        ASSERT(pthread_create(&exitSignalThread, 0,
                    signalExitThreadCB, 0) == 0, "");

        // Now we wait for all threads to be running past this
        // barrier.
        BARRIER_WAIT(&barrier);

        ASSERT((errno = pthread_barrier_destroy(&barrier)) == 0, "");
    }


    // Now all the thread in all stream are running past there barriers,
    // so they are initialized and ready to loop.

    for(auto stream : Stream::streams)
        for(auto filterModule : stream->sources)
        {
            // Above we checked that all source filters belong to a
            // particular thread.
            //
            // TODO: If the user overrode that and put two or more source
            // filters in the same thread than only the first source
            // filter in the thread will work.  We should make sure no two
            // sources share the same thread.
            //
            // Source Feed filter modules will loop until they are done.
            // These ModuleFilter::write() calls will trigger a call to
            // CRTSFilter::write() in their own threads.
            //
            // At this point this source filter that filterModule refers
            // to is of class Feed.
            DASSERT(dynamic_cast<Feed *>(filterModule->filter),"");
            // Feed filters only have one reader,
            DASSERT(filterModule->numOutputs == 1, "");
            DASSERT(filterModule->outputs[0], "");
            // and no writer filters,
            DASSERT(filterModule->numInputs == 0, "");
            DASSERT(filterModule->inputs == 0, "");
            // and write no data.
            DASSERT(filterModule->outputs[0]->ringBuffer == 0, "");

            // This will start the Feed filter threads looping.
            // 
            // This will signal a worker thread and return.
            //
            filterModule->launchFeed();

            // Now the main thread continues looping here.
        }

    // This main thread will unlock, wait, and lock the Stream::mutex
    // until there are no streams running any more.
    while(Stream::wait());


    // The first cleanup thing is removing the controller modules.
    removeCRTSCControllers(CONTROLLER_MAGIC);

    // This will try to gracefully shutdown the stream and join the rest
    // of the threads:
    //
    // TODO: But of course if any modules are using libuhd it may mess
    // that up, and may exit when you try to gracefully shutdown.
    //
    Stream::destroyStreams();

    MUTEX_UNLOCK(&Stream::mutex);

    cleanupModules();
 
    DSPEW("FINISHED");

    return 0;
}
