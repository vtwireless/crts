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

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp"
#include "crts/Controller.hpp"
#include "../txControl.hpp"

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
"  --device DEVICE_PATH set the linux device file to DEVICE_PATH.  The default\n"
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
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    // For the controller
    //
        void start(CRTSControl *c) { };
        void stop(CRTSControl *c) { };
        void execute(CRTSControl *c, const void * buffer, size_t len,
                uint32_t inChannelNum)
        {
        };

    private:

        int fd;
        const char *devicePath;
        struct js_event jsEvents[NUM_STRUCTS];

        TxControl *tx;
        //CRTSControl *js;

        double maxFreq, minFreq, freq, lastFreq;
};


JoystickReader::JoystickReader(int argc, const char **argv): fd(-1)
{
    CRTSModuleOptions opt(argc, argv, usage);

    devicePath = opt.get("--device", DEFAULT_DEVICE_PATH);

    const char *controlName = opt.get("--controlTx", DEFAULT_TXCONTROL_NAME);
    tx = getControl<TxControl *>(controlName);

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
                PRIu32, numInChannels);
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

    fwrite(jsEvents, 1, len, crtsOut);
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(JoystickReader)
