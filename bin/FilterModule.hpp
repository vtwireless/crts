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
            readPoint(0),
            unreadLength(0),
            thresholdLength(0),
            maxLength(0),
            inputChannelNum(inChannelNum) {};


        void reset(void)
        {
            readPoint = 0;
            unreadLength = 0;
            thresholdLength = 0;
            maxLength = 0;
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
               // let the amount of unread to exceed maxLength bytes.
               maxLength;

        // The input channel number that we call
        // CRTSFilter::write(,,inputChannelNum) with.  It's the input
        // channel number that the CRTSFilter write() sees.
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
        void runUsersActions(size_t len, Input *input)
        {
            void *buf = 0;
            uint32_t inputChannelNum = CRTSFilter::NULL_CHANNEL;

#ifdef DEBUG
            if(input == 0)
            {
                DASSERT(dynamic_cast<Feed *>(filter), "");
                DASSERT(numOutputs == 1, "");
                DASSERT(numInputs == 0, "");
                DASSERT(outputs[0], "");
                DASSERT(len == 0, "");
            }
            else
            {
                DASSERT(input, "");
                DASSERT(input->output, "");
            }
#endif

            if(input)
                inputChannelNum = input->inputChannelNum;


            // input is owned by this filter so we can change it here.

            if(len)
            {
                // We do not send a pointer to data if len is 0.  This 0
                // is not just for the Feed filters.  Sometimes we have 0
                // input to trigger filters without data input.
                buf = (void *) input->readPoint;

                input->unreadLength += len;
                // Add unread/accumulated data to the len that we
                // will pass to the CRTSFilter::write().
                len = input->unreadLength;
            }

#if 0
            // If there are any users CRTS Controllers that "attached" to
            // any of the CRTSControl objects in this CRTS filter we call
            // their CRTSContollers::execute() like so:
            for(auto const &controlIt: filter->controls)
                for(auto const &controller: controlIt.second->controllers)
                {
                    // Let the CRTSController do its' thing.
                    //
                    // TODO: CHECK THIS CODE:  The controller may even
                    // change the buffer and len in the up-comming
                    // filter->write();  buffer and len are non-constant
                    // references.
                    //
                    controller->execute(controlIt.second,
                            ptr, len, inChannelNum);
                }
#endif

            // The filter is calling a writePush() to a particular output
            // channel from a CRTSFilter::write() call with a particular
            // input channel.  This input would not be know ahead of
            // now.
            //
            // What inputs go to what outputs is only known at run-time,
            // so we note what input channel is here, so we can see it
            // in the writePush() calls that the filter makes.
            //
            currentInput = input;

            // TODO: All the CRTSFilter::writePush() calls will add to
            // CRTSFilter::_totalBytesOut and the other counters.
            //
            // We are in another filter (and maybe thread) from the
            // FilerModule::write() call that spawned this
            // runUsersActions() call.
            //
            filter->write(buf, len, inputChannelNum);

#ifdef DEBUG
            currentInput = 0;
#endif


            // totalBytesIn is a std::atomic so we don't need the mutex
            // lock here.  This can be read in a users' CRTS Controller
            // via the CRTS Control that the CRTS Filter provided.
            //
            // TODO: NEED to FIX this.  The bytes consumed is not the
            // same as len.
            filter->_totalBytesIn += len;

       // if(input && len)
       //     WARN("filter \"%s\" len = %zu", name.c_str(), len);

            // If the controller needs a hook to be called after the last
            // filter->write() so they can use the controller destructor
            // which is called after the last write.
            //
            // The hook comes in all the connected CRTS Filters is called:
            // CRTSController::shutdown(CRTSControl *c)
        };


        // So we may access the map (list) of controls for this filter
        // internally.  Since CRTSFilter::controls must be private, so
        // the user does not directly access it.
        std::map<std::string, CRTSControl *> &getControls()
        {
            return filter->controls;
        };

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

        inline void advanceWriteBuffer(size_t len, uint32_t outputChannelNum);

};


inline void
FilterModule::advanceWriteBuffer(size_t len, uint32_t outputChannelNum)
{
    if(outputChannelNum == CRTSFilter::ALL_CHANNELS)
    {
        for(uint32_t i=0; i<numOutputs; ++i)
            if(outputs[i]->ringBuffer->ownerOutput == outputs[i]
                    && outputs[i]->isPassThrough == false)
                outputs[i]->ringBuffer->advancePointer(
                        outputs[i]->ringBuffer->writePoint, len);
        return;
    }

    if(outputs[outputChannelNum]->isPassThrough == false)
        outputs[outputChannelNum]->ringBuffer->advancePointer(
                    outputs[outputChannelNum]->
                    ringBuffer->writePoint, len);
}
        


// CRTSFilter::writePush() helper function.  It's called twice in
// CRTSFilter::writePush() in Filter_write.cpp.  So we write the code once
// here, instead of twice in CRTSFilter::writePush().
//
// The special Feed filter module write() function never calls this, Feed
// gets short circuited to FilterModule::write() in
// CRTSFilter::writePush().
//
// filterModule is the filter module that this object is listed in
// filterModule->outputs[].  Passing filterModule is as good as making it
// part of the Output class, but uses less data in the running program.
//
// The Feed filter will never call Output::writePush().  It gets short
// circuited to FilterModule::write() in CRTSFilter::writePush().
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
    DASSERT(input, "");


    // If len == 0 we still need to call this for triggering filters.
    //
    // This toFilterModule->write() will eventually call the
    // toFilterModules CRTSFilter::write() method.  It needs to see if it
    // will block or not and send it to another thread or not.
    //
    toFilterModule->write(len, this,
            /*is different thread*/
            filterModule->thread != toFilterModule->thread);

    // TODO: FIX THIS DAM THING:
    //filterModule->filter->_totalBytesOut += len;
}
