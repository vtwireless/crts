#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <values.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <atomic>

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp"
#include "crts/Controller.hpp"

/* NOTES:

   # prerequisite:
   apt-get install joystick

reference code:

  https://github.com/flosse/linuxconsole/blob/master/utils/jstest.c

  https://www.kernel.org/doc/Documentation/input/joystick-api.txt

# Just plug in the USB joystick and run:

    cat /dev/input/js0 | hexdump

# now move the joystick and see the spew.

 */

#define DEFAULT_DEVICE_PATH  "/dev/input/js0"

// Number of joystick struct in the buffer
#define NUM_STRUCTS  (16) // For buffer size

// in mega-herz (M Hz)
#define DEFAULT_MAXFREQ (915.5)
#define DEFAULT_MINFREQ (914.5)

#define DEFAULT_TXCONTROL_NAME            "tx"

static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  OPTIONS are optional.  Transmitter Controller test module.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --controlTx NAME    connect to TX control named NAME.  The default TX\n"
"                       control name is \"" DEFAULT_TXCONTROL_NAME "\".\n"
"\n"
"\n"
"   --device DEVICE     set the linux device file to DEVICE.  The default\n"
"                       linux device file is \"" DEFAULT_DEVICE_PATH "\"\n"
"\n"
"\n"
"   --maxFreq MAX       set the maximum carrier frequency to MAX Mega Hz.  The\n"
"                       default MAX is %lg\n"
"\n"
"\n"
"   --minFreq MIN       set the minimum carrier frequency to MIN Mega Hz. The\n"
"                       default MIN is %lg\n"
"\n"
"\n", name,
    DEFAULT_MAXFREQ, DEFAULT_MINFREQ);
}



class JoystickReader : public CRTSFilter, public CRTSController
{
    public:

        JoystickReader(int argc, const char **argv);
        ~JoystickReader(void);

    // For the filter
    //
        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);

        // Called in this filter thread
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);


    // For the controller
    //
        void start(CRTSControl *c) { DSPEW(); };
        void stop(CRTSControl *c) { DSPEW(); };

        // Called in TX filter thread
        void execute(CRTSControl *c, const void * buffer, size_t len,
                uint32_t inChannelNum);

    private:

        int fd;
        const char *devicePath;
        struct js_event jsEvents[NUM_STRUCTS];

        double maxFreq, minFreq, lastFreq;

        // freq is shared between this filters thread and the Tx filters
        // thread.
        std::atomic<double> freq;
};


// The joystick controller controlling the TX
//
void JoystickReader::execute(CRTSControl *c, const void * buffer, size_t len,
                uint32_t inChannelNum)
{
    // This call is from the CRTS Tx filters' thread

    // atomic get freq in this thread.
    double f = freq;

    if(f != lastFreq)
    {
        fprintf(stderr, "   Setting carrier frequency to  %lg Hz\n", f);
        c->setParameter("freq", f);
        lastFreq = f;
    }
}


JoystickReader::JoystickReader(int argc, const char **argv): fd(-1)
{
    CRTSModuleOptions opt(argc, argv, usage);

    devicePath = opt.get("--device", DEFAULT_DEVICE_PATH);

    const char *controlName = opt.get("--controlTx", DEFAULT_TXCONTROL_NAME);
    CRTSControl *c = getControl<CRTSControl *>(controlName);
    if(!c)
        throw "Cannot get Tx Control";

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;

    freq = lastFreq = maxFreq;

    DSPEW();
}

JoystickReader::~JoystickReader(void)
{
    DSPEW();
}

bool JoystickReader::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(!isSource())
    {
        WARN("Should be a source filter");
        return true; // fail
    }

    if(numOutChannels)
    {
        WARN("Should not have output channels, got %"
                PRIu32, numOutChannels);
        return true; // fail
    }

    errno = 0;
    fd = open(devicePath, O_RDONLY);
    if(fd < 0)
    {
        ERROR("open(\"%s\", O_RDONLY) failed", devicePath);
        return true; // fail
    }

    DSPEW("opened device \"%s\"", devicePath);

    return false; // success
}


bool JoystickReader::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(fd >= 0)
    {
        close(fd);
        fd = -1;
    }
    DSPEW();
    return false; // success
}


void JoystickReader::input(void *buffer, size_t len, uint32_t channelNum)
{
    // This call is from this CRTS filters' thread

    DASSERT(len == 0, "");

    len = NUM_STRUCTS*sizeof(struct js_event);
    errno = 0;

    ssize_t ret = read(fd, jsEvents, len);

    if(ret < 0 ||
            // This happens ...
            ((size_t) ret)%sizeof(struct js_event))
    {
        // We'll just shutdown in this case.
        //
        stream->isRunning = false;
        NOTICE("read() returned %zd", ret);
        return;
    }

    size_t n = ret/sizeof(struct js_event);

    for(size_t i=0; i<n; ++i)
    {
        if(!(jsEvents[i].type & JS_EVENT_AXIS) || jsEvents[i].number != 0)
            continue;

        // atomic set freq in this thread.
        freq = minFreq +
            (((double) jsEvents[i].value - SHRT_MIN)/
             ((double) SHRT_MAX - SHRT_MIN))*(maxFreq - minFreq);
    }

    // Spew for the hell of it.
    //fwrite(jsEvents, 1, len, crtsOut);
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(JoystickReader)
