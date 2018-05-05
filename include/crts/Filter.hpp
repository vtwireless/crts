#ifndef __Filter_h__
#define __Filter_h__

#include <inttypes.h>
#include <pthread.h>
#include <list>
#include <map>
#include <atomic>
#include <string>

#include <crts/MakeModule.hpp>



// FilterModule is a opaque module thingy that the user need not worry
// about much.  We just needed it to add stuff to the CRTSFilter that we
// wanted to do some lifting without being seen by the user interface.
// CRTSFilter is the particular users filter implementation that has a
// standard CRTSFilter interface.  FilterModule just adds more genetic
// code to the CRTSFilter making it a software plugin filter module which
// you string together to build a CRTS filter "Stream".
//
class FilterModule;
class CRTSController;
class CRTSControl;
class CRTSFilter;
class Stream;



// CRTSStream is a user interface to set and get attributes of the Stream
// which is the related to the group of Filters that are connected (write
// and read) to each other.  There is a pointer to a CRTSStream in the
// CRTSFilter called stream.
//
class CRTSStream
{
    public:

        // A reference to the Stream isRunning flag.
        std::atomic<bool> &isRunning;

    private:

        // The CRTSFilter user does not make a CRTSStream.  It gets made
        // for them, hence this constructor is private so they can't
        // make one.
        CRTSStream(std::atomic<bool> &isRunning);


    friend FilterModule;
};


// The CRTSFilter is the user component module base class.
//
// We use the word filter to refer to general data processing component.
// Systems engineers would likely not want to refer to these
// data processing components as filters, but strictly speaking they are
// software filters, ref:  https://en.wikipedia.org/wiki/Filter_(software),
// so we call them filters which is short for "software filters".
// Some CRTSFilter objects have zero input, but the interface to these
// filters with no inputs is the same as the filters with input, so they
// are filters with null inputs.  We call them source filters.
//
// There are sink filters too. ...
//
//
// The Unix philosophy encourages combining small, discrete tools to
// accomplish larger tasks.  Reference:
//
//       https://en.wikipedia.org/wiki/Filter_(software)#Unix
//
// A stream can be thought of as items on a conveyor belt being processed
// one at a time.  In our case in CRTSFilters are placed at points along
// the conveyor belt to transform the data stream as it flows.
// This idea of "stream" seems to fit what we are doing.  We assemble
// CRTSFilters along our stream.  Reference:
//
//       https://en.wikipedia.org/wiki/Stream_(computing)
//       https://en.wikipedia.org/wiki/Filter_(software)
//

// When these modules run the coding interfaces are such that we do not
// have to know if these modules run on separate threads or not.  Wither
// or not these modules run on different threads is decided at run time.
// This is a requirement so that run/start time optimization is possible.
//
// Sometimes things are so simple that running a single thread is much
// faster than running more than one thread; and sometimes processing in a
// module takes so long that multi-threading, and multi-buffering between,
// the modules is faster.  We can test different filter/thread topologies
// on the fly and the filter writer does not have to be concerned about
// threading, which is a run time decision.  The cost for such flexibility 
// is requiring a particular interface, the CRTSFilter object.
//
// TODO:  It may be possible to make parts of this stream have CRTSFilter
// modules running in different processes, not just threads.  This should
// be able to be added seamlessly at without changing the users CRTSFilter
// code.
//
//
//
//         CRTSFilter trade-offs and considerations:
//
// If unlocked thread looping process time, for more than one thread in
// the stream, is much greater than inter thread contention time per loop,
// then we will get better performance with multi-threading.  If one
// module thread is the bottle-neck (slower than all others) than there
// may be no benefit to multi-threading.
//
// We can put any number of adjacent modules in the same thread.
//
//
// This idea of "stream" seems to fit what we are doing.
// https://en.wikipedia.org/wiki/Filter_(computing)
// 
// Base class for crts_radio IO (stream) modules or or call them stream
// modules.
//
// We are also concerned with sources and sinks of these streams.
// A source of a stream is a CRTSFilter with no data being written to it.
// A sink of a stream is a CRTSFilter with no data being written from it.
//
// This is a stream like thing using only the exposed write() methods
// for each part of the software filter stream.
//
//
// A common filter stream processing interface to seamlessly provide
// runtime optimization of threading and process topology selection.  Kind
// of like in optimization of MPI (massage passing interface) HPC (high
// performance computing) applications, we have node/thread runtime
// partitioning.
//
//
// See REDHAWK
// http://redhawksdr.github.io/Documentation/
//
//
class CRTSFilter
{
    public:

        static const uint32_t ALL_CHANNELS, NULL_CHANNEL;

        // Function to write data to this filter.
        //
        // This stream gets data when this is called by the "writer" and
        // in response may call this may call the reader->write().  This
        // is how data flows in this group of connected Filter objects.
        //
        // write() must call reader->write() when it wishes to push data
        // along the "stream" because the particular instance is the only
        // thing that knows what data is available to be pushed along.  A
        // CRTSFilter is a software filter with it's own ideas of what it
        // is doing.
        //
        // If this is a source (writer=0, below) this write() will be
        // called with buffer=0.
        //
        // In a sense this write() executes the stream.
        //
        // Clearly the writer (caller of this) dictates the buffer size.
        //
        // channelNum is set to non-zero values in order to merge filter
        // streams.  Most CRTSFilters will not care about channelNum.
        //
        // 0 <= channelNum < N   It's up to the CRTSFilter to decide what
        // to do with channelNum.  A CRTSFilter code that looks at
        // channelNum may be a stream merging filter, or a general stream
        // switching filter.
        virtual
        void write(
                void *buffer,
                size_t bufferLen,
                uint32_t inputChannelNum=0) = 0;

        /** This is called before the flow starts, or restarts.  The flow
          will stop and restart any time the stream topology changes.
          The stream filter topology should be considered fixed until
          stop() is called.

          The CRTSFilter writer may override this to take actions that
          dependent on what input channels and output channels are
          present, like how buffers are shared between input channels
          and output channels.

          The flow graph structure is not known when the CRTSFilter Super
          class constructor is called, so we must have this start
          interface to be called when that structure is known.

          This may be used to start a piece of physical hardware.

          Return true for failure.
         */
        virtual bool start(uint32_t numInputChannels,
                uint32_t numOutputChannels) = 0;

        /** This is called after the flow stops and the stream topology has
          not changed yet.  This may be due to the program heading toward
          exiting, or it may be due to a restart, in which case start()
          will be called later.

          The engineer on the star ship Enterprise may write this to
          shutdown the reactor core so the channels connections can be
          changed without having to handle live reactor conduits
          (channels).

          This may or may not be needed for all CRTSFilter modules.  It's
          so the Filter may stop a piece of physical hardware for a
          restart or shutdown like events.

          Return true for failure.
         */
        virtual bool stop(uint32_t numInputChannels,
                uint32_t numOutputChannels) = 0;


        virtual ~CRTSFilter(void);


        /** When the Super class constructor is called the filter
         * connection topology is not yet known.  The connection topology
         * stays the same between the call of CRTSFilter start() and
         * stop().  After the constructor is called and after stop(), but
         * before start() the topology may change.
         */
        CRTSFilter(void);

        CRTSStream *stream;


        /////// --- DEPRECATED ----
        // releaseBuffer() is not required to be called in
        // CRTSFilter::write().  It is used to free up the buffer that may
        // be being accessed in another thread where by freeing up
        // contention between threads that is associated with the sharing
        // of buffers between threads.  You can call this in your modules
        // CRTSFilter::write() when you know that CRTSFilter::write() is
        // in a part of your code that may take a while and it will not
        // access (read or write) to the buffer again in that call to
        // CRTSFilter::write().
        //
        // What releaseBuffers() does is depends on filter module
        // connection topology and thread grouping.  Using it may or may
        // not make the stream run faster.  It just depends.
        //
        // TODO: This may not be needed given the source filters no longer
        // loop, and return without looping in CRTSFilter::write().
        //--- DEPRECATED ---- void releaseBuffers(void);

        // Releases a buffer lock if this module has a different thread
        // than the module that wrote to this module.  The module may
        // hold more than one lock, so that adjacent buffers may be
        // compared without memory copies.
        //
        // This is automatically done for after CRTSFilter::write().
        //
        // --- DEPRECATED ---- static void releaseBuffer(void *buffer);

    protected:

        // A channel is just an count index (starting at 0) for calls to
        // CRTSFilter::write() and calls from CRTSFilter::writePush().
        // In a given CRTSFilter module the indexes are from 0 to N-1
        // calling CRTSFilter::write() and 0 to M-1 to
        // CRTSFilter::writePush(). The channel number is just defined in
        // a given CRTSFilter.  So a writePush from one module
        // at channel 2 is not necessarily received as channel 2 as
        // seen in the write() call.

        // **************************************************************
        // ************************* NOTICE *****************************
        // **************************************************************
        // For buffers passing to CRTSFilter::write() the maximum length
        // input from the feeding filter must be greater than or equal to
        // the feed filters threshold length.
        // **************************************************************
        // **************************************************************


        /** Lets a CRTSFilter know if it is the source in a stream graph
         */
        bool isSource(void);

        // Mark len bytes as read from the input buffer for the current
        // input channel.
        //
        void advanceInputBuffer(size_t len);

        void advanceInputBuffer(size_t len, uint32_t inputChannelNum);


        // Create a buffer that will have data with origin from this
        // CRTSFilter.  This creates one buffer that may have more than
        // one reading filter.
        //
        // Set the maximum length, in bytes, that this CRTSFilter will
        // read or write.  This must be called in the super class start()
        // function.
        //
        // This buffer is used with a given output channel.
        //
        // Any Filter that creates data for writePush() must call this.
        //
        // maxLength is the maximum length, in bytes, that this CRTS
        // Filter will and can writePush().  The CRTSFilter promises not
        // to writePush() more than this.
        //
        void createOutputBuffer(size_t maxLength,
                uint32_t outputChannelNum = ALL_CHANNELS);

        /** Create a buffer that will have data with origin from this
         * CRTSFilter.  This creates one buffer that may have more than
         * one reading filter.  In this version of this function you
         * may set many reading output channels via \p outputCannels
         * which is a null terminated list of output channel numbers.
         *
         * example:
         *   uint32_t outputChannels[] = { 0, 2, 3, NULL_CHANNEL };
         *
         * There is no threshold because this filter is the source
         * of this buffer.  This filter must not pushWrite() more than
         * maxLength bytes for the listed output channels.
         */
        void createOutputBuffer(size_t maxLength,
                const uint32_t *outputChannelNums);


        // Instead of allocating a buffer, we reuse the input buffer from
        // the channel associated with inputChannelNum to writePush() to.
        //
        // This should not be called in CRTSFilter::write().
        //
        // We are passing a buffer out so there is a maximum output
        // length.
        //
        // We are passing a buffer in and so there is an input threshold.
        //
        //
        void createPassThroughBuffer(
                uint32_t inputChannelNum,
                uint32_t outputChannelNum,
                size_t maxLength,
                size_t threshold = 1);

#if 0
        // The Filter may need to access more than just the buffer passed
        // in through the call arguments, CRTSFilter::write(buffer, len,
        // inChannelNum), so we have this so that the get access to other
        // input channel buffers.
        //
        // Returns a pointer to the start of usable memory.
        //
        void *getInputBuffer(size_t &len, uint32_t inputChannelNum);
#endif

        /** Returns a pointer to the current writing position
         * of the buffer so the filter may write to the memory.
         *
         * This buffer must have been created by the filter with
         * createOutputBuffer()
         */
        void *getOutputBuffer(uint32_t outputChannelNum);


#if 0
        void shareOutputBuffer(uint32_t outputChannelNum1,
                uint32_t outputChannelNum2);
#endif


        // Set the minimum length, in bytes, that this CRTSFilter must
        // accumulate before CRTSFilter::write() can and will be called.
        //
        // Calling this in CRTSFilter::write() is fine.  Making this limit
        // as large as practical will save on unnecessary
        // CRTSFilter::write() calls.  This length may not exceed the
        // maximum Buffer Length set with setThresholdLength().
        //
        // TODO: This can be called when the stream is running.
        void setInputThreshold(size_t len,
                uint32_t inputChannelNum = ALL_CHANNELS);


        // The CRTSFilter code know if they are pushing to more than on
        // channel.  The channel we refer to here is just an attribute of
        // this filter point (node) in this stream.  ChannelNum goes from
        // 0 to N-1 where N is the total CRTSFilters that are connected
        // to push this data to.  It's up to the writer of the CRTSFilter
        // to define how to push to each channel, and what channel M (0 <=
        // M < N) means.  Do not confuse this with other channel
        // abstractions like a frequency sub band or other software
        // signal channel.  Use channelNum=0 unless this CRTSFilter is
        // splitting the stream.
        //
        // TODO: Named channels and other channel mappings that are not
        // just a simple array index thing.
        //
        // outputChannelNum handles the splitting of the stream.  We can
        // call writePush() many times to write the same thing to many
        // channels; with the cost only being an added call on the
        // function stack.
        //
        // If outputChannel has an origin at this filter the data will be
        // copied from the buffer that was passed in to write(), else this
        // will just pass through the data by advancing pointers in the
        // buffer.  The outputChannelNum must be a created buffer in this
        // filter or the outputChannelNum must be connected to the buffer
        // via passThroughBuffer().
        // 
        //
        /** Write the current buffer that was passed into
         * CRTSFilter::write() to the output channel with channel number
         * outputChannelNum.
         * 
         *
         * This also advances the input buffer len bytes and triggers
         * a write() to output Channels.
         */
        void writePush(size_t len, uint32_t outputChannelNum = ALL_CHANNELS);


        // Increment the buffer useCount.  Since the buffers created by
        // CRTSFilter::getBuffer() are shared between threads, we needed a
        // buffer use count thingy.  This needs to be called before the
        // buffer is passed to another thread for use.
        //--- DEPRECATED ---- void incrementBuffer(void *buffer);


        // TODO: this...
        //
        // This maximum buffer queue length may be needed in the case
        // there one of the filters in the stream is very slow compared to
        // the others, and the feeding filters make a lot of buffers
        // feeding this bottleneck.
        //
        // Think how many total packages can we handle on the conveyor
        // belts, held at the packagers (writers), and held at the
        // receivers (readers).  This is the buffer queue that is between
        // all the modules that access (read or write) this buffer
        // queue.
        //
        // Think: What is the maximum number of packages that will fit on
        // the conveyor belt.
        //void setBufferQueueLength(uint32_t n);
        //
        //static const uint32_t defaultBufferQueueLength;


    // The FilterModule has to manage the CRTSFilter adding readers and
    // writers from between separate CRTSFilter objects.  This is better
    // than exposing methods that should not be used by CRTSFilter
    // implementers. Because the user never knows what a FilterModule is,
    // the API (application programming interface) and ABI (application
    // binary interface) never changes when FilterModule changes, whereby
    // making this interface minimal and more stable.
    //
    // For example, we can add new functionality to CRTSFilter by adding
    // code to the FilterModule class, and we would not even have to
    // recompile the users CRTSFilter code, because the users CRTSFilter
    // ABI does not change.  The cost is one pointer deference at most
    // CRTSFilter methods.
    friend FilterModule; // The rest of the filter code and data.
    friend CRTSControl;
    friend CRTSController;
    friend Stream;

    private:

        // TODO: It'd be nice to hide this list in filterModule but
        // it's a major pain, and I give up; time is money.

        // This is the list of controls that this filter as created.
        std::map<std::string, CRTSControl *>controls;

        // This is the list of controllers that this filter calls
        // CRTSController::execute() for.  This is is added to from
        // CRTSController::getControl<>()
        //
        //std::list<CRTSController *> controllers;

        // Needed by the plug-in loader to make a default CRTSControl.
        CRTSControl *makeControl(const char *controlName, bool generateName);


        // Pointer to the opaque FilterModule co-object.  The two objects
        // could be one object, except that we need to hide the data and
        // methods in the FilterModule part of it.  So CRTSFilter is the
        // "public" user interface and FilterModule is the ugly opaque
        // co-class that does the heavy lifting, which keeps the exposed
        // parts of CRTSFilter smaller.
        FilterModule *filterModule;

        // These counters wrap at 2^64 ~ 1.7x10^10 GB ~ 16 exabytes. At a
        // rate of 10 GBits/s it will take 435 years, so higher rates may
        // be a problem.  A rate of 1000 GBits/s will wrap in 4 years,
        // and this would not be a good design.  In 10 years this counter
        // will need to be of type uint128_t, or it needs to be made
        // a circular counter, whatever that is, or just add an additional
        // counter to count the number of 16 exabytes chucks, like the
        // time structures do.
        std::atomic<uint64_t> _totalBytesIn, _totalBytesOut;
};


class CRTSController
{

    public:

        CRTSController(void);
        virtual ~CRTSController(void);

        virtual void execute(CRTSControl *c, void * &buffer,
                size_t &len, uint32_t channelNum) = 0;

        // This is called by each CRTS Filter as it finishes running.
        // We don't get access to the CRTSFilter, we get access to the
        // CRTSControl that the filter makes.
        virtual void shutdown(CRTSControl *c) = 0;

        const char *getName(void) const { return (const char *) name; };
        uint32_t getId(void) const { return id; };

    protected:

        // Returns 0 if the control with name name was not found in the
        // list of all CRTS controls.  The particular CRTSController
        // can get pointers to any CRTSControl objects.
        template <class C>
        C getControl(const std::string name) const;

    private:


        // TODO: Ya, this is ugly.  It'd be nice to not expose these
        // things to the module writer; even if they are private.
        //
        friend CRTSControl;
        friend int LoadCRTSController(const char *name,
                int argc, const char **argv, uint32_t magic);
        friend void removeCRTSCControllers(uint32_t magic);
        friend Stream;


        // Global list of all loaded CRTSController plug-ins:
        static std::list<CRTSController *> controllers;

        // Used to destroy this object because CRTS Controllers are loaded
        // as C++ plugins and destroyController must not be C++ name
        // mangled so that we can call the C wrapped delete.
        //
        // TODO: We should be hiding this data from the C++ header user
        // interface; I'm too lazy to do that at this point.
        //
        void *(*destroyController)(CRTSController *);

        char *name; // From module file name.

        uint32_t id; // unique ID of a given CRTSController
};


class CRTSControl
{

    public:

        const char *getName(void) const;

        /** Total bytes from all CRTSFilter::write() calls since the
         * program started.  When you know the approximate input rate, you
         * can use this to get an approximate time without making a system
         * call.
         */
        uint64_t totalBytesIn(
                uint32_t inChannelNum = CRTSFilter::ALL_CHANNELS) const;

        /** Total bytes written out via CRTSFilter::writePush() since the
         * program started.  Note: if there is more than one output
         * channel this will include a total for all channels.  TODO: add
         * per channel totals.
         */
        uint64_t totalBytesOut(
                uint32_t outChannelNum = CRTSFilter::ALL_CHANNELS) const;

        uint32_t getId(void) const { return id; };


    protected:

        CRTSControl(CRTSFilter *filter, std::string controlName);

        virtual ~CRTSControl(void);

    private:

        char *name;

        // List of loaded CRTSController plug-ins that access
        // this CRTS Control.
        std::list<CRTSController *> controllers;

        // Global list of all CRTSControl objects:
        static std::map<std::string, CRTSControl *> controls;

        // The filter associated with this control.
        CRTSFilter *filter;

        uint32_t id; // unique ID from all CRTSControl objects

        friend CRTSFilter;
        friend CRTSController;
        friend FilterModule;
        friend Stream;
};



template <class C>
C CRTSController::getControl(const std::string name) const
{
    DASSERT(name.length(), "");
    CRTSControl *crtsControl = 0;
    C c = 0;

    auto search = CRTSControl::controls.find(name);
    if(search != CRTSControl::controls.end())
    {
        crtsControl = search->second;
        DASSERT(crtsControl, "");
        c = dynamic_cast<C>(crtsControl);
        DASSERT(c, "dynamic_cast<CRTSControl super class>"
                " failed for control named \"%s\"", name.c_str());
        DSPEW("got control \"%s\"=%p", name.c_str(), c);

        // Add this CRTS Controller to the CRTS Filter's
        // execute() callback list.
        crtsControl->controllers.push_back(
                (CRTSController *) this);
    }
    else
        WARN("Did not find CRTS control named \"%s\"", name.c_str());

    return c;
}


inline
CRTSControl *CRTSFilter::makeControl(const char *controlName, bool generateName)
{
    DASSERT(controlName && controlName[0], "");

    auto &controls = CRTSControl::controls;

    if(!generateName)
    {
        if(controls.find(controlName) != controls.end())
        {
            ERROR("A control named \"%s\" is already loaded", controlName);
            return 0;
        }
        return new CRTSControl(this, controlName);
    }

    // We generate a unique CRTS Control name using a counter.
    // Better than just failing.
    std::string genName = controlName;
    int count = 1;
    while(controls.find(genName) != controls.end())
    {
        genName = controlName;
        genName += "_";
        genName += std::to_string(count++);
        DASSERT(count < 10000, "");
    }

    return new CRTSControl(this, genName);
}


inline const char *CRTSControl::getName(void) const { return name; };


inline uint64_t CRTSControl::totalBytesIn(uint32_t inChannelNum) const
{
    DASSERT(filter, "");
    return filter->_totalBytesIn;
};


inline uint64_t CRTSControl::totalBytesOut(uint32_t outChannelNum) const
{
    DASSERT(filter, "");
    return filter->_totalBytesOut;
};



#define CRTSCONTROLLER_MAKE_MODULE(derived_class_name) \
    CRTS_MAKE_MODULE(CRTSController, derived_class_name)


#define CRTSFILTER_MAKE_MODULE(derived_class_name) \
    CRTS_MAKE_MODULE(CRTSFilter, derived_class_name)

#endif // #ifndef __Filter__
