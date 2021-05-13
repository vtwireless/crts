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
#define DEFAULT_PORT    ((uint32_t) 5554)
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
"  This filter module connects to a TCP Server socket and writes the CRTS stream\n"
"  that it reads to a TCP socket.\n"
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


class TcpClientSink : public CRTSFilter
{
    public:

        TcpClientSink(int argc, const char **argv);
        ~TcpClientSink(void);

        bool start(uint32_t numInChannels, uint32_t numOutChannels);
        void input(void *buffer, size_t bufferLen, uint32_t inChannelNum);


    private:

        unsigned short getOptions(int argc, const char **argv);

        unsigned short port;
        const char *address;
        CRTSTcpClient socket;
        int fd;
};



unsigned short TcpClientSink::getOptions(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    address = opt.get("--server_address", DEFAULT_ADDRESS);

    return (port = opt.get("--server_port", DEFAULT_PORT));
}



TcpClientSink::TcpClientSink(int argc, const char **argv):
    port(getOptions(argc, argv)),
    socket(address, port),
    fd(socket.getFd())
{
    DSPEW();
}


bool TcpClientSink::start(uint32_t numInChannels, uint32_t numOutChannels)
{
    ASSERT(numInChannels == 1, "");
    ASSERT(numOutChannels == 0, "");

    return false; // success
}


TcpClientSink::~TcpClientSink(void)
{
    // Do nothing...

    DSPEW();
}

void TcpClientSink::input(void *buffer, size_t len, uint32_t inputChannelNum)
{
    if(len < 1) return; // WTF

    // This is a blocking read().
    ssize_t n = write(fd, buffer, len);

    if(n < 1) {
        // This whole stream may be done.
        stream->isRunning = false;
        NOTICE("write() = %zd  SO we are done", n);
        return;
    }
}

// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(TcpClientSink)
