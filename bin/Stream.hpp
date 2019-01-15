class RingBuffer;
class Output;
class Input;

// A factory of FilterModule class objects.
// We keep a list (map) in Stream.
//
// The FilterModules numbered by load index starting at 0
// in the order given on the crts_radio command line.
// That load index is the map key to find the FilterModule.
//
class Stream
{
    public:

        // TODO: More of this needs to be private.

        // For each Stream there is at least one source but there
        // can be any number of sources.  There we keep a list
        // of all the sources for this stream.
        std::list<FilterModule*> sources;

        // The list (map) of filter modules in this Stream.
        std::map<uint32_t, FilterModule*> map;

        // We use this map just to keep the name unique for a given
        // filter.  The thing stored in this map is of no concern.  We
        // just use this map to keep the filter names unique for the whole
        // process.  This is not used with the Feed filters.
        static std::map<const std::string, uint32_t> filterNames;

        void getSources(void);

        // Finish building the default thread groupings for all streams.
        static void finishThreads(void);


        // Create a CRTSFilter plugin object without loading a file.
        FilterModule *load(CRTSFilter *crtsFilter,
                void *(*destroyFilter)(CRTSFilter *),
                const char *name);

        // Load a CRTSFilter plugin.
        bool load(const char *name, int argc, const char **argv);

        bool unload(FilterModule* filtermodule);


        bool connect(FilterModule *from,  FilterModule *to);

        bool connect(uint32_t from, uint32_t to);


        // This is accessible as a reference to it by the CRTSFilter
        // objects in CRTSFilter::stream->isRunning.  The CRTSFilter can
        // set it's value to false.  They should not set it to any other
        // value but false.  They can also read this value in
        // CRTSFilter::stream->isRunning.
        //
        // This should only be set in the users CRTSFilter code.
        //
        std::atomic<bool> isRunning;

        // Stream factory
        Stream(void);

        // list of all streams created
        static std::list<Stream*> streams;

        // Factory destructor
        static void destroyStreams(void);

        // Print a DOT graph file that represents all the streams
        // in the streams list.  Prints a PNG file if filename
        // ends in ".png".
        static bool printGraph(const char *filename = 0, bool _wait=true);

        // returns a file descriptor that can read the PNG data and than
        // the user should close it after reading it all.
        static int printGraph(bool _wait=true);


        // It removes itself from the streams list
        ~Stream(void);

        // We set this, haveConnections, flag if the --connect (or -c)
        // argument was given with the connection list (like -c 0 1 1 2)
        // then this is set.  This is so we can know to setup default
        // connections.
        bool haveConnections; // Flag standing for having any connections
        // at all.


        // A list of threads that run a list of filter modules.
        std::list<Thread *> threads;


        // So we may call wait() in the master/main thread.
        static pthread_cond_t cond;
        static pthread_mutex_t mutex;
        static bool waiting;


        // Waits until a stream and all it's threads is cleaned up.
        // Returns the number of remaining running streams.
        static size_t wait(void);


        static void signalMainThreadCleanup(void);


        // calls start for all the filters in all streams
        static bool startAll(void);

        // calls stop for all the filters in all streams
        static bool stopAll(void);

    private:

        // Called when the stream->isRunning is false.
        //
        // This returns when all the threads in this stream are in the
        // pthread_cond_wait() call in filterThreadWrite() in Thread.cpp.
        // So we can know that the main thread is the only "running"
        // thread when this returns.
        //
        void waitForCondWaitThreads(void);

        // Call filter start() for just this one stream.
        //
        // Returns true on error.
        //bool start(void);
        //
        //
        // It needed to be broken into two functions for each stream so
        // that we can call start1() for all streams and then start2() for
        // all streams, which is not the same as calling start1() and
        // start2() together on each stream.
        //
        // Returns true on error.
        bool start1(void);
        bool start2(void);

        // Call filter stop() for just this one stream.
        //
        // Returns true on error.
        bool stop(void);


        static bool printGraph(FILE *f);

        // Never decreases.  Incremented with each new FilterModule.
        uint32_t loadCount;

        uint32_t streamNum; // number starting at 0.

        static uint32_t streamCount; // always increasing.
 };
