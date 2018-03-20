// A class for keeping the CRTSFilter loaded modules with other data.
// Basically a container struct, because we wanted to decrease the amount
// of data in the CRTSFilter (private or otherwise) which will tend to
// make the user interface change less.  We can add more stuff to this and
// the user interface will not change at all.  Even adding private data to
// the user interface in the class CRTSFilter will change the API
// (application programming interface) and more importantly ABI
// (application binary interface), so this cuts down on interface
// changes; and that could be a big deal.
//

class Stream;
class Thread;
class CRTSFilter;



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

        // Filter modules connections are made with Input writer
        // and Output reader.
        //
        // reader reads what this module produces triggered by
        // writer writes to this module.
        //
        // We have multiple readers and writers giving forks and merges 
        // in the stream flow.
        //
        //
        //     SINK, SOURCE, and INTERMEDIATE filters:
        //
        // If there is no reader than this is a output stream filter
        // (flow) terminator or SINK stream filter.  If there is no writer
        // than this is a in stream filter (flow) terminator or SOURCE
        // stream filter.  If there is a reader and a writer than this is
        // a continuous flow, pass through, flow restriction, general
        // stream, or in general a software INTERMEDIATE stream filter.
        //
        // We hide the private data and functions in class FilterModule
        // so as to not pollute this interface header file.

        // TODO: Consider a more rapidly nodal topology and make a general
        // node class that better handles connection changes.
        //
        // Indexing into arrays is fast and changing connections it slow,
        // so we are not planing on changing connection often.  We can
        // change the method we use to connect filters without affecting
        // the CRTSFilter user interface.

        // We assume (for now) that the connections are pretty static
        // so accessing the adjacent filters via an array is fine.
        //
        // If fast changes are needed in this connectivity we can make
        // this a std::map later without affecting the user interface.
        // readers, writers are arrays of pointers:
        FilterModule **readers, **writers;
        // This is a corresponding array of indexes that this filter
        // uses as channel numbers to read and write. 
        // i=readerIndexes[n] is the channel index that the reader filter
        // that we pushWrite(,,n) sees, when this filter calls write(,,i)
        uint32_t *readerIndexes;
        // The length of the reader arrays and the writer arrays:
        uint32_t numReaders, numWriters;


        std::string name; // name from program crts_radio command line argv[]

 
        // Buffer length needed/requested by this module.
        //uint32_t bufferQueueLength;

        // This write calls the underlying CRTSFilter::write() functions
        // or signals a thread that calls the underlying
        // CRTSFilter::write() function.
        void write(void *buffer, size_t len,
                uint32_t channelNum, bool toDifferentThread = false);


        // The buffers that this filter module is using in a given
        // CRTSFilter::write() call.
        std::stack<void *>buffers;


        // the thread that this filter module is running in.
        Thread *thread;


        // Is set from the CRTSFilter constructor and stays constant.
        // Flag to say if this object can write to the buffer
        // in CRTSFilter::write(buffer,,).  If this is not set the
        // CRTSFIlter::write() function should not write to the buffer
        // that is passed in the arguments.
        bool canWriteBufferIn;


        void removeUnusedBuffers(void);


        // Is this filter module a data source.  If it has no writers
        // than it is a source.
        inline bool isSource(void) { return (writers?false:true); };



    // TODO: could this friend mess be cleaned up?  Not easily, it would
    // require quite a bit of refactoring.  Note: this interface is not
    // exposed to the (user interface) CRTSFilter objects, so users will
    // not see this mess and it can change without changing the users
    // codes.


    friend CRTSFilter; // CRTSFilter and FilterModule are co-classes
    // (sister classes); i.e.  they share their data and methods.  We just
    // made them in two classes so that we could add code to FilterModule
    // without the users CRTSFilter inferface seeing changes to the
    // FilterModule interface.  Call it interface hiding where the
    // FilterModule is the hidden part of CRTSFilter.  FilterModule does
    // the heavy lifting for CRTSFilter.

    friend Stream;

        void runUsersActions(void *buffer, size_t len, uint32_t channelNum)
        {
            // TODO: The order of filter->write() and the
            // controller->execute() calls could be different???
            // Whatever is decided we will be stuck with it after this
            // starts getting used.

            // totalBytesIn is a std::atomic so we don't need the mutex
            // lock here.  This can be read in an users' CRTS Control.
            filter->_totalBytesIn += len;

            // If there are any users CRTS Contollers we call their
            // execute() like so:
            for(auto const &control: filter->controls)
                for(auto const &controller: control.second->controllers)
                    controller->execute(control.second);

            // All the CRTSFilter::writePush() calls will add to
            // _totalBytesOut.
            filter->write(buffer, len, channelNum);

            // If the controller needs a hook to be called after the last
            // filter->write() they can use the controller destructor
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
        }

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
        }
};
