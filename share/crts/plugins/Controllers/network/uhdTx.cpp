#include <stdio.h>

#include <values.h>
#include <jansson.h>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>

#include "../../Filters/txControl.hpp"


class UhdTx: public CRTSController
{
    public:

        UhdTx(int argc, const char **argv);
        ~UhdTx(void);

        void start(CRTSControl *c);
        void stop(CRTSControl *c);

        void execute(CRTSControl *c, const void *buffer, size_t len,
                uint32_t channelNum);

    private:

        CRTSTcpConnection *connection;

        TxControl *tx;

        int socketFd;

        double maxFreq, minFreq, txFreq, lastTxFreq;
};


// in mega-herz (M Hz)
#define DEFAULT_MAXFREQ (915.5)
#define DEFAULT_MINFREQ (914.5)
#define DEFAULT_PORT ((uint32_t) 6523)
#define DEFAULT_ADDRESS "localhost"


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"    OPTIONS are optional.  UHD Transmitter parameter Controller module.\n"
"\n"
"    This CRTS Controller will use one control get a TxControl from it and\n"
"  apply actions given from the network to that control.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"  --firstMessage MSG  send MSG as the first message to the socket just after\n"
"                      connecting.\n"
"\n"
"\n"
"   --address ADDR     set the TCP/IP4 address to ADDR.  The default port is\n"
"                      \"" DEFAULT_ADDRESS "\"\n"
"\n"
"\n"
"   --controlTx NAME   connect to TX control named NAME.  The default TX\n"
"                      control name is \"" DEFAULT_TXCONTROL_NAME "\".\n"
"\n"
"\n"
"   --maxFreq MAX      set the maximum carrier frequency to MAX Mega Hz.  The\n"
"                      default MAX is %lg.  libuhd may add additional\n"
"                      restriction.\n"
"\n"
"\n"
"   --minFreq MIN      set the minimum carrier frequency to MIN Mega Hz. The\n"
"                      default MIN is %lg.  libuhd may add additional\n"
"                      restriction.\n"
"\n"
"\n"
"   --port PORT        set the TCP/IP port to PORT.  The default port is %d\n"
"\n",

    name, DEFAULT_MAXFREQ, DEFAULT_MINFREQ, DEFAULT_PORT);
}


UhdTx::UhdTx(int argc, const char **argv) : connection(0)
{
    CRTSModuleOptions opt(argc, argv, usage);
    const char *controlName;

    controlName = opt.get("--controlTx", DEFAULT_TXCONTROL_NAME);
    tx = getControl<TxControl *>(controlName);

    maxFreq = opt.get("--maxFreq", DEFAULT_MAXFREQ) * 1.0e6;
    minFreq = opt.get("--minFreq", DEFAULT_MINFREQ) * 1.0e6;

    txFreq = lastTxFreq = maxFreq;

    short port = opt.get("--port", DEFAULT_PORT);
    const char *address = opt.get("--port", DEFAULT_ADDRESS);
    const char *firstMessage = opt.get("--port", "");

    DSPEW("address=\"%s\" port=%hu firstMessage=\"%s\"", address, port, firstMessage);

    connection = new CRTSTcpConnection(firstMessage, address, port);
}

UhdTx::~UhdTx(void)
{
    if(connection)
    {
        delete connection;
        connection = 0;
    }

    DSPEW();
}


void UhdTx::start(CRTSControl *c)
{
    DASSERT(c->getId() == tx->getId(), "");

    DSPEW("control=\"%s\"", c->getName());
}


void UhdTx::stop(CRTSControl *c)
{
    DASSERT(c->getId() == tx->getId(), "");

    DSPEW();
}


void UhdTx::execute(CRTSControl *c, const void *buffer,
        size_t len, uint32_t channelNum)
{
    // We only use the "tx" control
    //
    DASSERT(c->getId() == tx->getId(), "");

    // Check if we have a request.


    {
        fprintf(stderr, "Setting TX carrier frequency to  %lg Hz\n", txFreq);
        tx->usrp->set_tx_freq(txFreq);
        lastTxFreq = txFreq;

        return;
    }
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(UhdTx)
