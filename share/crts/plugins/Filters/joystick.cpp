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

   apt-get install joystick

reference code:

  https://github.com/flosse/linuxconsole/blob/master/utils/jstest.c


# Just plug in the USB joystick and run:

    cat /dev/input/js0 | hexdump




 */

#define DEFAULT_DEVICE_PATH  "/dev/input/js0"


static void usage(void)
{
    char name[64];
    fprintf(crtsOut, "Usage: %s [ --device DEVICE_PATH ]\n"
            "\n"
            "\n  The default device PATH is " DEFAULT_DEVICE_PATH
            "\n"
            , CRTS_BASENAME(name, 64));

    throw ""; // This is how return an error from a C++ constructor
    // the module loader with catch this throw.
}



class Joystick : public CRTSFilter
{
    public:

        Joystick(int argc, const char **argv);
        ~Joystick(void);

        ssize_t write(void *buffer, size_t bufferLen,
                uint32_t channelNum);

    private:

        int fd;
};


Joystick::Joystick(int argc, const char **argv): fd(-1)
{
    CRTSModuleOptions opt(argc, argv, usage);
    const char *devicePath = opt.get("--device", DEFAULT_DEVICE_PATH);

    errno = 0;
    fd = open(devicePath, O_RDONLY);
    if(fd < 0)
    {
        ERROR("open(\"%s\", O_RDONLY) failed", devicePath);
        throw "open() failed";
    }
    DASSERT(fd>=0, "open(\"%s\", O_RDONLY) failed", devicePath);

    DSPEW("opened device \"%s\"", devicePath);
}

Joystick::~Joystick(void)
{
    if(fd >= 0)
        close(fd);

    DSPEW();
}


#define NUM  (10)


ssize_t Joystick::write(void *buffer, size_t len, uint32_t channelNum)
{
#if 1

    len = NUM*sizeof(struct js_event);
    struct js_event *js = (struct js_event *) getBuffer(len);
    buffer = (void *) js;

    ssize_t ret = read(fd, buffer, len);
    if(ret < 0 || ((size_t) ret)%sizeof(struct js_event))
    {
        stream->isRunning = false;
        NOTICE("read() %zd", ret);
        return 0;
    }
    len = ret;

    //DSPEW("read()=%zu", len);


#else
    len = 1024;
    buffer = getBuffer(len);

    ssize_t ret = read(fd, buffer, len);
    
    if(ret < 1)
    {
        stream->isRunning = false;
        NOTICE("read() %zd", ret);
        return 0;
    }

    len = ret;
#endif

    writePush(buffer, len, ALL_CHANNELS);


    return 1;
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Joystick)
