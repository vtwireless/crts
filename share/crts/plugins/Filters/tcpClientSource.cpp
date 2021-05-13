#include <stdio.h>
#include <math.h>
#include <values.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <string>
#include <atomic>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Filter.hpp>

#define DEFAULT_ADDRESS "localhost"
#define DEFAULT_PORT    ((uint32_t) 5555)
#define BUFLEN ((size_t) 1024)



static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  This filter module connects to a TCP Server socket and reads all the socket\n"
"  data and then writes the data to a CRTS stream.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --help             print this help\n"
"\n"
"\n"
"   --server_address ADDRESS set the server address to connect to.  The default\n"
"                            server address is " DEFAULT_ADDRESS "\n"
"\n"
"\n"
"   --server_port PORT       set the server port to connect to.  The default port\n"
"                            port is %u\n"
"\n",
name, DEFAULT_PORT);

}


class TcpClientSource : public CRTSFilter
{
    public:

        TcpClientSource(int argc, const char **argv);
        ~TcpClientSource(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);

    private:

        unsigned short getOptions(int argc, const char **argv);

        unsigned short port;
        const char *address;
        CRTSTcpClient socket;
        int fd;
};



unsigned short TcpClientSource::getOptions(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    address = opt.get("--server_address", DEFAULT_ADDRESS);

    return (port = opt.get("--server_port", DEFAULT_PORT));
}



TcpClientSource::TcpClientSource(int argc, const char **argv):
    port(getOptions(argc, argv)),
    socket(address, port),
    fd(socket.getFd())
{
    DSPEW();
}


bool TcpClientSource::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    ASSERT(numInChannels == 1, "");
    ASSERT(numOutChannels == 1, "");
    createOutputBuffer(BUFLEN, ALL_CHANNELS);
    
    return false; // success
}


TcpClientSource::~TcpClientSource(void)
{
    // Do nothing...

    DSPEW();
}

void TcpClientSource::input(void *buffer_in, size_t len, uint32_t inputChannelNum)
{
    uint8_t *buffer = (uint8_t *) getOutputBuffer(0);

    // This is a blocking read().
    ssize_t nread = read(fd, buffer, BUFLEN);

    if(nread < 1) {
        // This whole stream may be done.
        stream->isRunning = false;
        NOTICE("read() = %zd  SO we are done", nread);
        return;
    }

    output(nread, ALL_CHANNELS);
}

// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(TcpClientSource)
