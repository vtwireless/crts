// A class for keeping the CRTSFilter loaded modules with other data.
// Basically a container struct, because we wanted to decrease the amount
// of data in the CRTSFilter (private or otherwise) which will tend to
// make the user interface change less.  We can add more stuff to this and
// the user interface will not change at all.  Even adding private data to
// the user interface in the class CRTSFilter will change the API
// (application programming interface) and more importantly ABI
// (application binary interface), so this cuts down on interface changes;
// and that could be a big deal.
//

// You cannot have BUFFER_DEBUG without DEBUG
#ifdef BUFFER_DEBUG
#  ifndef DEBUG
#    undef BUFFER_DEBUG
#  endif
#endif



class Stream;
class Thread;
class CRTSFilter;
class FilterModule;
class Output;
class Input;


// We copied this ring buffer idea from GNUradio.
//
// https://www.gnuradio.org/blog/buffers/
//
// See makeRingBuffer.cpp for more references and details.
//


// All the values in a RingBuffer object never change at RUNTIME, except
// writePoint.
//
class RingBuffer
{
    public:

        RingBuffer(Output *ownerOutput_in):
            length(0), overhang(0), maxLength(0),
            ownerOutput(ownerOutput_in),
            writePoint(0),
            start(0) { };

        // un-maps the memory and frees object for a Filter stop.
        //
        ~RingBuffer(void);

        // makeRingBuffer() parameters.  See makeRingBuffer.cpp.
        size_t 
            length, overhang, // in bytes, is multiple of pagesize..
            // maxLength is the maximum length read/write request length
            // that all filters promised.
            maxLength; // in bytes.

        // The Output Channel that manages this ring buffer though it may
        // be shared between other Output Channels in other filters.
        //
        // The owner filter is ownerOutput->filterModule;
        //
        Output *ownerOutput;

        // writePoint is the next byte address that will be written to.
        //
        // The writePoint value that changes at RUNTIME
        //
        uint8_t *writePoint;

        // start is the address of the first byte in the ring buffer
        // memory.
        //
        uint8_t *start;

        void
        advancePointer(uint8_t * &ptr, size_t len) const
        {
            DASSERT(len <= maxLength, "");
            // This is the order of things:
            DASSERT(overhang <= length, "");
            DASSERT(ptr >= start, "");
            DASSERT(ptr < start + length, "");
 
            if(ptr + len < start + length)
                ptr += len;
            else
                ptr += len - length;

            DASSERT(overhang <= length, "");
            DASSERT(ptr >= start, "");
            DASSERT(ptr < start + length, "");
        };
};


// All data in an Output is constant at stream run-time.
//
class Output
{
    public:

        Output(FilterModule *fM, FilterModule *toFilterModule);

        void reset(void)
        {
            if(ringBuffer && ringBuffer->ownerOutput == this)
                delete ringBuffer;
            ringBuffer = 0;
            maxLength = 0;
            isPassThrough = false;
            totalBytesOut = 0;
        }; 

        // See comments about writePush() below.
        //
        void writePush(size_t len, FilterModule *filterModule);
 
        // input that this Output Channel is connected to.
        //
        // input points to data in a input in a connecting filter
        // which is not the filter that has this as an output.
        //
        Input *input;

        // toFilterModule is the filter object that has input in it.
        //
        FilterModule *toFilterModule;

        // CASE 1: adjacent:
        //                   The originating filter that is up stream
        //                   filter just one filter up.
        //
        // CASE 2: pass through:
        //                   The originating filter that is up stream
        //                   more than one filter.
        //
        RingBuffer *ringBuffer; // data store.

        // The filter outputting this data promises to not write more than
        // maxLength bytes.
        //
        size_t maxLength;

        uint64_t totalBytesOut;

        // isPassThrough if this is a pass through buffer that uses a ring
        // buffer from another filter.
        //
        bool isPassThrough;
};


// All the values in a Input object never change, except
// readPoint and unreadLength.
//
// readPoint and unreadLength are changing at stream run-time, but is only
// accessed by the reading filter/thread.
//
class Input
{
    public:

        Input(Output *o, uint32_t inChannelNum):
            output(o),
            inputChannelNum(inChannelNum)
        { 
            reset();
        };

        void reset(void)
        {
            readPoint = 0;
            unreadLength = 0;
            thresholdLength = 0;
            maxUnreadLength = 0;
            chokeLength = (size_t) -1; // largest value
            totalBytesIn = 0;
        };

        // output is the Output that is feeding this input from
        // another filter.
        //
        Output *output;

        // Where to read data from.  This points to data in the
        // output->ringBuffer.
        //
        uint8_t *readPoint;

        // How much we can read from readPoint (in bytes).
        //
        size_t unreadLength;

        // thresholdLength is the length needed for the reading filter to
        // have its' write called.
        //
        size_t thresholdLength, 
               // The filter that reads this input channel promises to not
               // let the amount of unread to exceed maxUnreadLength bytes.
               maxUnreadLength,
               // The most that the filter will get on a given input()
               // call.
               chokeLength;

        uint64_t totalBytesIn;

        // The input channel number that we call
        // CRTSFilter::write(,,inputChannelNum) with.  It's the input
        // channel number that the CRTSFilter input() sees.
        //
        uint32_t inputChannelNum;
};




// FilterModule is the hidden parts of CRTSFilter to do heavy lifting.
//
class FilterModule
{
    public:

        FilterModule(Stream *stream, CRTSFilter *filter,
                void *(*destroyFilter)(CRTSFilter *), int32_t loadIndex,
                std::string name);

        ~FilterModule(void);

        // What stream this filter belongs to:
        Stream *stream;

        CRTSFilter *filter; // users co-class

        void *(*destroyFilter)(CRTSFilter *);

        int loadIndex; // From Stream::loadCount

        Output **outputs;
        Input **inputs;

        // currentInput is set to the Input channel before calling
        // CRTSFilter::write(,,inputChannelNum) so the internals know what
        // the current input channel is.  We don't need to burden the
        // filter module writer with having to pass the current input
        // channel to function calls.
        //
        Input *currentInput;

        uint32_t numOutputs, numInputs;

        // Did we advance the input buffer pointer yet?
        //
        bool advancedInput;


        // This is not the control name.
        std::string name; // name from program crts_radio command line argv[]


        // This write calls the underlying CRTSFilter::write() functions
        // or signals a thread that calls the underlying
        // CRTSFilter::write() function.
        //
        void write(size_t len, Output *output, bool isDifferentThread);

        // the thread that this filter module is running in.
        Thread *thread;

        // If it has no inputs than it is a source.
        //
        inline bool isSource(void) { return (numInputs?false:true); };


        void initRingBuffer(RingBuffer *rb);

        void launchFeed(void);


        bool callStopForEachOutput(void);

        // Stinking flag so that we know that stop() was called for this
        // filter.  We need this because we call the stop() functions with
        // a graph traversal which can hit a filter more than once,
        // because they can have more than one input.
        bool stopped;

    // TODO: could this friend mess be cleaned up?  Not easily, it would
    // require quite a bit of refactoring.  Note: this interface is not
    // exposed to the (user interface) CRTSFilter objects, so users will
    // not see this mess and it can change without changing the users
    // codes.


    friend CRTSFilter; // CRTSFilter and FilterModule are co-classes
    friend Stream;
    friend Output;
    friend Input;
    friend RingBuffer;


        // CRTSFilter::start() functions have not been called before this
        //
        bool depthTest(int &depth/*recurse depth*/)
        {
            if(++depth > 1001)
            {
                ERROR("The filter stream directed graph appears to"
                        " have a loop, recursed %d filters to no end.",
                        depth);
                return true;
            }

            for(uint32_t i=0; i<numOutputs; --i)
            {
                Output *output = outputs[i];
                DASSERT(output, "");

#ifdef DEBUG
                if(numInputs == 0)
                {
                    DASSERT(dynamic_cast<Feed *>(filter), "");
                    DASSERT(numOutputs == 1, "");
                    DASSERT(output, "");
                }
                else
                {
                    DASSERT(dynamic_cast<Feed *>(filter) == 0, "");
                }
#endif
                // ring buffer get created in CRTSFilter::start() functions
                // which have not been called yet.
                DASSERT(output->ringBuffer == 0, "");
                DASSERT(output->input, "");
                DASSERT(output->input->output == output, "");
                DASSERT(output->toFilterModule, "");
                DASSERT(output->toFilterModule != this,
                        "You can't have filter connect to each other.");

                // Dive into the next filter module.
                if(outputs[i]->toFilterModule->depthTest(depth))
                    return true;
            }
            return false;
        }

        // input is a pointer to an Input in this filter
        // that is part of this filter.
        //
        void runUsersActions(size_t len, Input *input);


        // Needed by the plug-in loader to make a default CRTSControl.
        CRTSControl *makeControl(int argc, const char **argv)
        {
            CRTSModuleOptions opt(argc, argv);
            bool generateName = false;
            const char *controlName = opt.get("--control", "");
            if(controlName[0] == '\0')
            {
                // The user did not pass in a --control NAME
                // as a command line argument.
                generateName = true;
                controlName = name.c_str();
            }

            return filter->makeControl(controlName, generateName);
        };

        void createOutputBuffer(size_t maxLength,
                uint32_t outputChannelNum, Output * &ownerOutput,
                bool isPassThrough = false);

        void AdvanceWriteBuffer(size_t len, Output *o, uint32_t outputChannelNum);

        inline void advanceWriteBuffer(size_t len, uint32_t outputChannelNum);

        void InputOutputReport(FILE *file = stderr);

        void ControlActions(const void *buffer, size_t len,
                uint32_t inputChannelNum);
};


inline void FilterModule::AdvanceWriteBuffer(size_t len, Output *o,
        uint32_t outputChannelNum)
{
    if(o->ringBuffer->ownerOutput == o && o->isPassThrough == false)
        o->ringBuffer->advancePointer(o->ringBuffer->writePoint, len);

    // For this output channel
    o->totalBytesOut += len;

    // For all output channels in the filter
    filter->_totalBytesOut += len;


    if(numOutputs)
        filter->setParameter("totalBytesOut", filter->control->totalBytesOut());
    if(numOutputs > 1)
    {
        // If there was just one output there is not need of 
        std::string s = "totalBytesOut";
        s += std::to_string(outputChannelNum);
        filter->setParameter(s, filter->control->totalBytesOut(outputChannelNum));
    }
}


inline void
FilterModule::advanceWriteBuffer(size_t len, uint32_t outputChannelNum)
{
    if(outputChannelNum != CRTSFilter::ALL_CHANNELS)
        AdvanceWriteBuffer(len, outputs[outputChannelNum], outputChannelNum);
    else
    {
        // outputChannelNum == CRTSFilter::ALL_CHANNELS
        //
        for(uint32_t i=0; i<numOutputs; ++i)
            AdvanceWriteBuffer(len, outputs[i], i);
    }
}

inline void
FilterModule::ControlActions(const void *buffer, 
        size_t len, uint32_t inputChannelNum)
{
    if(!filter->control)
        // This filter has no control.
        return;

    // If there are any users CRTS Controllers that "attached" to
    // the CRTSControl object in this CRTS filter we call their
    // CRTSContollers::execute() like so:
    for(auto const &controller: filter->control->controllers)
        // Let the CRTSController do its' thing.
        //
        // TODO: CHECK THIS CODE:  The controller may even
        // change the buffer and len in the up-comming
        // filter->write();  buffer and len are non-constant
        // references.
        //
        controller->execute(filter->control, buffer, len, inputChannelNum);
}


// CRTSFilter::output() helper function.  It's called twice in
// CRTSFilter::output() in Filter_write.cpp.  So we write the code once
// here, instead of twice in CRTSFilter::output().
//
// The special Feed filter module write() function never calls this, Feed
// gets short circuited to FilterModule::write() in CRTSFilter::output().
//
// filterModule is the filter module that this object is listed in
// filterModule->outputs[].  Passing filterModule is as good as making it
// part of the Output class, but uses less data in the running program.
//
// The Feed filter will never call Output::writePush().  It gets short
// circuited to FilterModule::write() in CRTSFilter::output().
//
inline void Output::writePush(size_t len, FilterModule *filterModule)
{
    DASSERT(ringBuffer->maxLength >= len,
            "ringBuffer->maxLength=%zu < len=%zu",
            ringBuffer->maxLength, len);
    DASSERT(input, "");
    DASSERT(toFilterModule != filterModule, "");
    DASSERT(ringBuffer, "");
    DASSERT(filterModule->currentInput, "");


    // If len == 0 we still need to call this for triggering filters.
    //
    // This toFilterModule->write() will eventually call the
    // toFilterModules CRTSFilter::write() method.  It needs to see if it
    // will block or not and send it to another thread or not.
    //
    toFilterModule->write(len, this,
            /*is different thread*/
            filterModule->thread != toFilterModule->thread);
}
