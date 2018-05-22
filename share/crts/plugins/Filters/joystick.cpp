#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/joystick.h>

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp"

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

#define NUM_STRUCTS  (16) // For buffer size


static void usage(void)
{
    char name[64];
    fprintf(crtsOut, "Usage: %s [ --device DEVICE_PATH ]\n"
            "\n"
            "\n  The default device PATH is " DEFAULT_DEVICE_PATH
            "\n"
            , CRTS_BASENAME(name, 64));

    throw "usage"; // This is how we return an error from a C++ constructor
    // the module loader will catch what we throw.
}


class Joystick : public CRTSFilter
{
    public:

        Joystick(int argc, const char **argv);
        ~Joystick(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        bool stop(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        int fd;
        const char *devicePath;
};


Joystick::Joystick(int argc, const char **argv): fd(-1)
{
    CRTSModuleOptions opt(argc, argv, usage);
    devicePath = opt.get("--device", DEFAULT_DEVICE_PATH);
 }

Joystick::~Joystick(void)
{
    DSPEW();
}

bool Joystick::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(!isSource())
    {
        WARN("Should be a source filter");
        return true; // fail
    }

    if(!numOutChannels)
    {
        WARN("Should have at least one output channel, got %"
                PRIu32, numInChannels);
        return true; // fail
    }

    // We use one buffer for the source of each output Channel
    // that all share the same ring buffer.
    createOutputBuffer(NUM_STRUCTS*sizeof(struct js_event), ALL_CHANNELS);

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


bool Joystick::stop(uint32_t numInChannels, uint32_t numOutChannels)
{
    if(fd >= 0)
    {
        close(fd);
        fd = -1;
    }
    DSPEW();
    return false; // success
}


void Joystick::input(void *buffer, size_t len, uint32_t channelNum)
{
    len = NUM_STRUCTS*sizeof(struct js_event);
    struct js_event *js = (struct js_event *) getOutputBuffer(0);
    buffer = (void *) js;

    errno = 0;

    ssize_t ret = read(fd, buffer, len);

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

    len = ret;

    output(len, ALL_CHANNELS);
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Joystick)
