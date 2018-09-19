#ifndef __Filter_h__
#define __Filter_h__

#include <inttypes.h>
#include <pthread.h>
#include <list>
#include <map>
#include <atomic>
#include <string>

#include <crts/MakeModule.hpp>

// Due to the interdependent nature of all the C++ classes in CRTS (and
// because C++ sucks) we are forced to put most of the CRTS interface
// definitions in this one file.  We are always required to define all
// these classes in order to use any one of them, because they are all
// connected.
//
// TODO: We could break this into more files that include each other in
// a series of includes.

// FilterModule is a opaque module thingy that the user need not worry
// about much.  We just needed it to add stuff to the CRTSFilter that we
// wanted to do some lifting without being seen by the user interface.
// CRTSFilter is the particular users filter implementation that has a
// standard CRTSFilter interface.  FilterModule just adds more genetic
// code to the CRTSFilter making it a software plugin filter module which
// you string together to build a CRTS filter "Stream".
//
//
// This is what we mean by these classes are all interdependent, we
// require pre-declarations of the classes, so we can have inline
// functions and templates, hence fast running code.
//
class FilterModule;
class CRTSController;
class CRTSControl;
class CRTSFilter;
class Parameter;
class Stream;
template <class T> class CRTSParameter;


// CRTSStream is a user interface to set and get attributes of the Stream
// which is the related to the group of Filters that are connected (input
// and output) to each other.  There is a pointer to a CRTSStream in the
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

    // The CRTSFilter/FilterModule needs to be able to set up access to the
    // isRunning flag.
    //
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
// This is a stream like thing using only the exposed input() methods
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

        static const uint32_t 
            
        /** A channel number used to refer to all channels by they input
         * or output.
         *
         * It may be used as the channel parameter to the functions:
         * input(), output(), createOutputBuffer(),
         * createPassThroughBuffer(), setInputThreshold(),
         * setMaxUnreadLength(), setChokeLength(), getOutputBuffer(),
         * totalBytesIn(), and totalBytesOut().
         */
            ALL_CHANNELS,

         /** A special channel number used to refer to no channel at all.
          *
          * The value of \c NULL_CHANNEL is not 0, since 0 is a valid
          * channel number.
          */
            NULL_CHANNEL;

        /** The function for receiving input into your filter.
         *
         * The CRTSFilter super class (filter) must provide this function
         * to receive input.  This function will be called by the running
         * crts_radio program as input data becomes available from the
         * calling of output() in the filters that connect to this
         * filter.
         *
         * When this input() function is called the stream may be running
         * in one of two modes:
         *
         *      1. Normal running when stream->isRunning is true.
         *
         *      2. Shutdown mode when stream->isRunning is false.  In
         *      shutdown mode all the source filters will no longer get
         *      their input() functions called and all the other
         *      non-source filters will get their input() called until all
         *      the flowing data runs out.  There may be one of more
         *      filter input() calls with the amount of input data that is
         *      less than the requested threshold amount.
         *
         * \param buffer is a pointer to the start of the input data or 0
         * if \c len is 0.
         *
         * \param bufferLen is the number of bytes available to access in
         * \c buffer.
         *
         * \param inputChannelNum is the designated input channel number
         * for the the given input.  This channel number is valid for this
         * receiving filter.  Both input and output channel numbers start
         * at 0 and increase by one based on the order of when they are
         * connected (kind of like file descriptors).  There are never
         * gaps in the channel number sequences.
         */
        virtual
        void input(
                void *buffer,
                size_t bufferLen,
                uint32_t inputChannelNum) = 0;

        /** This is called before the flow starts, or restarts.  The flow
         * will stop and restart any time the stream topology changes.
         * The stream filter topology should be considered fixed until
         * stop() is called.
         *
         * The CRTSFilter writer may override this to take actions that
         * dependent on what input channels and output channels are
         * present, like how buffers are shared between input channels
         * and output channels.
         *
         * The flow graph structure is not known when the CRTSFilter Super
         * class constructor is called, so we must have this start
         * interface to be called when that structure is known.
         *
         * This may be used to start a piece of physical hardware.
         *
         * \param 
         *
         * \return true for failure.
         */
        virtual bool start(uint32_t numInputChannels,
                uint32_t numOutputChannels) = 0;

        /** This is called after the flow stops and the stream topology has
         * not changed yet.  This may be due to the program heading toward
         * exiting, or it may be due to a restart, in which case start()
         * will be called later.
         *
         * The engineer on the star ship Enterprise may write this to
         * shutdown the reactor core so the channels connections can be
         * changed without having to handle live reactor conduits
         * (channels).
         *
         * This may or may not be needed for all CRTSFilter modules.  It's
         * so the Filter may stop a piece of physical hardware for a
         * restart or shutdown like events.
         *
         * \return true for failure.
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

        // The stream that this filter object is in.
        CRTSStream *stream;

    protected:

        // A channel is just an count index (starting at 0) for calls to
        // CRTSFilter::input() from calls to CRTSFilter::output().  In a
        // given CRTSFilter module the indexes are from 0 to N-1 calling
        // CRTSFilter::input() and 0 to M-1 to CRTSFilter::output(). The
        // channel number is just defined in a given CRTSFilter.  So a
        // output from one module at output channel 2 is not necessarily
        // received as channel 2 as seen in the next filters input()
        // call.

        // **************************************************************
        // ************************* NOTICE *****************************
        // **************************************************************
        // For buffers passing to CRTSFilter::input() the maximum length
        // input from the feeding filter must be greater than or equal to
        // the feed filters threshold length.
        // **************************************************************
        // **************************************************************


        /** Lets a CRTSFilter know if it is the source in a stream graph.
         * 
         * A source filter will continuously have its' input() called
         * with zero bytes of input until stream->isRunning is not true.
         *
         * \return true if the filter is a source or false otherwise
         */
        bool isSource(void);

        /** Mark len bytes as read from the input buffer for the current
            input channel.

            It is not required to call this, but if it is not called
            in the filter modules input() function the input buffer
            will automatically be advanced by the total number of
            bytes called with the input() function.
         */
        void advanceInput(size_t len);


        /** Create a buffer that will have data with origin from this
         * CRTSFilter.
         *
         * This function should only be called in the filters' start()
         * function.
         *
         * \param maxLength is the largest amount of data that will be
         * written to this buffer.  This filter must not output() more
         * than \c maxLength bytes for the listed output channels.
         *
         * \param outputChannelNum is an output channel number.
         * outputChannelNum must be less than numOutputChannels
         * that was passed into the filters \c CRTSFilter::start()
         * function.
         */
        void createOutputBuffer(size_t maxLength,
                uint32_t outputChannelNum = ALL_CHANNELS);


        /** Create a buffer that will have data with origin from this
         * CRTSFilter.  This creates one buffer that may have more than
         * one output channel.  The buffer will be shared between the
         * channels.
         *
         * This function should only be called in the filters' start()
         * function.
         *
         * \param maxLength is the largest amount of data that will be
         * written to this buffer.  This filter must not output() more
         * than \c maxLength bytes for the listed output channels, in a
         * single input() call; otherwise the ring buffer may be
         * overrun.
         *
         * \param outputChannelNums a \c NULL_CHANNEL terminated array of
         * channels. For example:
         * \code
         * uint32_t outputChannelsNums[] = { 0, 2, 3, NULL_CHANNEL };
         * \endcode
         */
        void createOutputBuffer(size_t maxLength,
                const uint32_t *outputChannelNums);


        /** Instead of allocating a ring buffer, we reuse the ring buffer
         * from the channel associated with \c inputChannelNum to the
         * output channel with channel number \c outputChannelNum.
         *
         * This function should only be called in the filters' start()
         * function.
         *
         * \param inputChannelNum an input channel number that we will
         * get the buffer from.
         *
         * \param outputChannelNum an output channel number that we will
         * use to output() with.
         *
         * \param maxLength the maximum number of bytes that we will leave
         * unconsumed by the filter.  Not consuming the input data on
         * an input channel may lead to buffer overrun.  You are promising
         * to clean plate to this amount.
         *
         * \param thresholdLength is the input threshold that is needed to
         * be archived before the filters input() is called.  During
         * stream shutdown this threshold may not be archived before the
         * filters input() is called, otherwise the input would be lost.
         */
        void createPassThroughBuffer(
                uint32_t inputChannelNum,
                uint32_t outputChannelNum,
                size_t maxLength,
                size_t thresholdLength = 0);


        /** Returns a pointer to the current writing position
         * of the buffer so the filter may write to the memory.
         *
         * This buffer must have been created by the filter with
         * createOutputBuffer() in the start() function.
         *
         * \param outputChannelNum the output channel number.
         */
        void *getOutputBuffer(uint32_t outputChannelNum);


        /** Set the minimum length, in bytes, that this CRTSFilter must
         * accumulate before input() can and will be called, for the given
         * channel.  This can only be called in the start() function.
         *
         * \param len the threshold length in bytes.
         *
         * \param inputChannelNum is the input channel number.  Set \c
         * inputChannelNum to \c ALL_CHANNELS to apply this to all channels in
         * the filter.
         */
        void setInputThreshold(size_t len,
                uint32_t inputChannelNum = ALL_CHANNELS);


        /** Set the maximum length, in bytes, beyond which this
         * filter promises to not leave unconsumed.
         *
         * There may be input lengths to input() larger than this due to
         * adjcent filters having higher limits.  This is just so the
         * needed ring buffer size may be computed.
         *
         * This can not be called when the stream is running, that is
         * outside the start() function.
         *
         * \param len the maximum length in bytes.
         *
         * \param inputChannelNum is the input channel number.
         * Set \c inputChannelNum to \c ALL_CHANNELS to apply this to all
         * channels in the filter.
         */
        void setMaxUnreadLength(size_t len,
                uint32_t inputChannelNum = ALL_CHANNELS);


        /** Set the maximum length, in bytes, which this
         * filter will accept in its' input() call on the given channel.
         *
         * This can not be called when the stream is running, that is
         * outside the start() function.
         *
         * \param len the maximum length in bytes.
         *
         * \param inputChannelNum is the input channel number.
         * Set \c inputChannelNum to \c ALL_CHANNELS to apply this to all
         * channels in the filter.
         */
        void setChokeLength(size_t len,
                uint32_t inputChannelNum = ALL_CHANNELS);


        /** Write output to a given output channel. 
         *
         * Trigger a connected filter input() call from the current
         * filter, whereby writing \c len bytes to the filter connected to
         * output channel \c outputChannelNum and advancing the write
         * pointer in the ring buffer.
         *
         * The amount of data consumed by the connected receiving filter
         * can be different than, \c len, the length requested, because
         * the consuming filter has the option of letting data accumulate
         * before acting on it.
         *
         * \param len length in bytes.
         *
         * \param inputChannelNum is the input channel number.
         * Set \c inputChannelNum to \c ALL_CHANNELS to apply this to all
         * channels in the filter.
         */
        void output(size_t len, uint32_t outputChannelNum = ALL_CHANNELS);

        /**
         * \param inputChannelNum is the input channel number. If \c
         * inputChannelNum is to \c ALL_CHANNELS get the total for all
         * input channels to this filter.
         *
         * \return the total number of bytes that the filters
         * corresponding input channel has consumed.  This does
         * not include any concurrently running input() calls, only
         * calls to input() that have returned.
         */
        uint64_t totalBytesIn(
                uint32_t inputChannelNum = CRTSFilter::ALL_CHANNELS) const;

        /**
         * \param outputChannelNum is the output channel number.  If \c
         * outputChannelNum is to \c ALL_CHANNELS get the total for all
         * output channels for this filter.
         *
         * \return the total number of bytes that has been written to the
         * corresponding output channel for this filter.
         */
        uint64_t totalBytesOut(
                uint32_t outputChannelNum = CRTSFilter::ALL_CHANNELS) const;


        CRTSControl *makeControl(const char *controlName)
        {
            return makeControl(controlName, false);
        };

 
    friend FilterModule; // The rest of the filter code and data.
    friend CRTSControl;
    friend CRTSController;
    friend Stream;

    private:

        // TODO: It'd be nice to hide this list in filterModule but
        // it's a major pain, and I give up; time is money.

        // This is the list of controls that this filter as created.
        std::map<std::string, CRTSControl *>controls;


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
        // time structures do.  Good enough to now (September 2018).
        //
        uint64_t _totalBytesIn, _totalBytesOut;
};


class CRTSController
{

    public:

        CRTSController(void);
        virtual ~CRTSController(void);

        /** start is called after all the filters start and
         * before the stream is running.
         *
         * /param c the filters CRTS control
         */
        virtual void start(CRTSControl *c) = 0;

        /** stop is called just before the filters stop.
         *
         * /param c the filters CRTS control
         */
        virtual void stop(CRTSControl *c) = 0;

        /** execute is called before each CRTSFilter that owns the
         * CRTSControl calls output() for the connected CRTSFilters.
         *
         * When this is called, we are running in the CRTSFilter that owns
         * the CRTSControl that is passed in.  We can't used controls from
         * other CRTSFilter objects, because they may be accessed in other
         * threads.
         *
         * This is the only method that that is not called from the main
         * thread.  All other methods are called in the main thread.
         *
         * /param c the CRTSControl that is owned by the CRTSFilter that 
         * called this.
         *
         * /param buffer pointer to the next list of bytes that will come
         * into the CRTSFilter that receive this data.
         *
         *  /param len length of data that could be read, but may not
         *  necessarily be read after this call.
         *
         * /param channelNum the CRTSFilters channel number that it uses
         * to refer to the connection in the filter stream.
         */
        virtual void execute(CRTSControl *c, const void * buffer,
                size_t len, uint32_t channelNum) = 0;


        const char *getName(void) const { return (const char *) name; };
        uint32_t getId(void) const { return id; };

    protected:

        /** Returns 0 if the control with name name was not found in the
         * list of all CRTS controls.  The particular CRTSController
        * can get pointers to any CRTSControl objects.
        *
        * You do not need to (or want to) delete the CRTSControl object
        * that this points to.  It is managed by the associated CRTSFilter
        * object.
        *
        * The CRTSController implementation must include the particular
        * CRTSFilter interface header file to get access to the control
        * methods that are implemented.
        *
        * \todo add example code.
        *
        * \param name the unique (across the CRTSStream) name of this
        * CRTSControl.
        *
        * \return a CRTSControl object pointer.
        */
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




/** The CRTSControls should be accessed in the CRTSFilter thread
 * that it is associated with or in the main thread in a constructor.
 *
 * CRTSControls is a factory of any number of CRTSParameters.
 *
 * The super class of CRTSControl should public in header files all the
 * CRTSParameter classes that it creates.
 *
 */
class CRTSControl
{

    public:

        /**
         * /param inChannelNum the channel number that the CRTSFilter
         * is referring to.
         *
         * /return the total number of bytes that the associated 
         * input channel have been consumed by the associated CRTSFilter
         * since the last start.
         *
         */
        uint64_t totalBytesIn(uint32_t inChannelNum =
                CRTSFilter::ALL_CHANNELS) const
        {
            DASSERT(filter, "");
            return filter->totalBytesIn(inChannelNum);
        };

        /**
         * /param outChannelNum the channel number that the CRTSFilter
         * is referring to.
         *
         * /return the total number of bytes that the associated 
         * output channel have been pushed to the associated CRTSFilter
         * that is connected since the last start.
         *
         * This may not be the same as the number of bytes consumed,
         * totalBytesIn(), by the connected CRTSFilter, because there may
         * be data queued up and the connected CRTSFilter may be waiting
         * for a certain amount or type of data before it marks the data
         * as consumed.
         */
        uint64_t totalBytesOut(uint32_t outChannelNum =
                CRTSFilter::ALL_CHANNELS) const
        {
            DASSERT(filter, "");
            return filter->totalBytesOut(outChannelNum);
        };


        const char *getName(void) const;

        uint32_t getId(void) const { return id; };


        /** The CRTSController uses this to access CRTSParameter objects.
         *
         * Get a CRTSParameter by searching only CRTSParameters that this
         * CRTSControl made.
         *
         * The templated parameter type could be for example
         * CRTSParameter<double> which there is a parameter like a
         * floating point number like the center transmission frequency.
         * Then you could use the CRTSParameter<double>::set(freq) and
         * freq = CRTSParameter<double>::get() methods.
         *
         * /param name the unique name of this control for the associated
         * CRTSFilter.
         *
         *  /return the CRTSParameter template object pointer.  You do not
         *  want to delete the object.  It will automatically by managed
         *  by the CRTSFilter base class object.
         */
        template <class T>
        CRTSParameter<T> *getParameter(std::string name)
        {
            auto search = parameters.find(name);
            DASSERT(search, "");
            if(search == parameters.end())
            {
                DSPEW("Parameter named \"%s\" was not found", name.c_str());
                return 0;
            }

            Parameter *parameter = search->second;
            DASSERT(parameter, "");

            T t = dynamic_cast<T *>(parameter);
            DASSERT(t, "dynamic_cast<CRTSParameter super class>"
                    " failed for parameter named \"%s\"", name.c_str());
            DSPEW("got CRTSControl parameter named \"%s\"=%p", name.c_str(), t);

            // We return the type of the template which should be a
            // pointer like for example a pointer like the type:
            // CRTSParameter<float> *
            return t;
        }


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


        // List CRTSParameter objects:
        //
        std::map<std::string, Parameter *> parameters;

        // The filter associated with this control.
        CRTSFilter *filter;

        uint32_t id; // unique ID from all CRTSControl objects

        friend CRTSFilter;
        friend CRTSController;
        friend FilterModule;
        friend Stream;
        friend Parameter;
};


// Parameter provides a un-templated type we can store in a std::map list of
// the templated CRTSParameter class that inherits this class.
//
// This is very much a hidden dummy storage class type so that we may put
// it in a std::map in the CRTSControl.  The super class is class
// CRTSParameter<> that has the set() and get() methods.
//
class Parameter
{

    public:

        Parameter(CRTSControl *c, const std::string name_in) : name(name_in)
        {
            DSPEW();
            control = c;
            c->parameters[name] = this;
        };

        virtual ~Parameter(void)
        {
            DASSERT(control->parameters.find(name) != control->parameters.end(), "");
            control->parameters.erase(name);
            DSPEW("removed parameter named \"%s\"", name.c_str());
        };


    private:

        CRTSControl *control; // managing control object

        const std::string name; // map key in the CRTSControl
};



/** CRTSParameter is the base class for a single parameter value that
 * can be set() and got via get().
 *
 * CRTSParameter is a template base class for providing an interface to
 * get() and set() parameters that are part of a CRTSControl.  The
 * CRTSParameter is accessed in a CRTSContorller.
 *
 * This is a base class for controlling a single parameter.  A common
 * template type would be a float or a double; for example a double which
 * is the transmitter frequency.
 */
template <class T>
class CRTSParameter : public Parameter
{

    public:

        /** /return the value of the parameter.  For example: T is a float
         * or int value.
         */
        virtual T get(void) = 0;

        /**
         * /param t the value we try to set the parameter to.
         *
         * /return true if this call effected the value of the parameter
         * and false otherwise.  What the value is may be determined from
         * get() which may not return the same value you tried to set.
         */
        virtual bool set(const T &t) = 0;

    private:

        CRTSParameter(CRTSControl *c, std::string name) : Parameter(c, name)
        {
            DSPEW();
        };


        virtual ~CRTSParameter(void) { DSPEW(); };

        friend CRTSControl;
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


#define CRTSCONTROLLER_MAKE_MODULE(derived_class_name) \
    CRTS_MAKE_MODULE(CRTSController, derived_class_name)


#define CRTSFILTER_MAKE_MODULE(derived_class_name) \
    CRTS_MAKE_MODULE(CRTSFilter, derived_class_name)


#endif // #ifndef __Filter_h__

