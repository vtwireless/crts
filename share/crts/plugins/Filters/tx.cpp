#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <uhd/usrp/multi_usrp.hpp>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp" // for:  FILE *crtsOut

#include "usrp_set_parameters.hpp" // UHD usrp wrappers
#include "defaultUSRP.hpp" // defaults: TX_FREQ, TX_RATE, TX_GAIN


class Tx : public CRTSFilter
{
    public:

        Tx(int argc, const char **argv);
        ~Tx(void);

        ssize_t write(void *buffer, size_t bufferLen, uint32_t channelNum);

    private:

        uhd::usrp::multi_usrp::sptr usrp;
        uhd::device::sptr device;
        uhd::tx_metadata_t metadata;
};


// This is called if the user ran something like: 
//
//    crts_radio -f file [ --help ]
//
//
static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);

    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  OPTIONS are optional.\n"
"\n"
"  As an example you can run something like this:\n"
"\n"
"       crts_radio -f stdin -f tx [ --uhd addr=192.168.10.3 --freq 932 ]\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --uhd ARGS      set the arguments to give to the uhd::usrp constructor.\n"
"\n"
"                                 Example: %s [ --uhd addr=192.168.10.3 ]\n"
"\n"
"                   will use the USRP (Universal Software Radio Peripheral)\n"
"                   which is accessible at Ethernet IP4 address 192.168.10.3\n"
"\n"
"\n"
"   --freq FREQ     set the initial transmitter frequency to FREQ MHz.  The default\n"
"                   initial transmitter frequency is %g MHz.\n"
"\n"
"\n"
"   --gain GAIN     set the initial transmitter gain to GAIN.  The default initial\n"
"                   transmitter gain is %g.\n"
"\n"
"\n"
"   --rate RATE     set the initial transmitter sample rate to RATE million samples\n"
"                   per second.  The default initial transmitter rate is %g million\n"
"                   samples per second.\n"
"\n"
"\n"
"\n",
        name, name,
        TX_FREQ, TX_GAIN, TX_RATE);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}


static double getDouble(const char *str)
{
    char name[64];
    double ret;
    char *ptr = 0;
    errno = 0;

    ret = strtod(str, &ptr);
    if(ptr == str || errno)
    {
        fprintf(crtsOut, "\nBad module %s arg: %s\n\n",
                CRTS_BASENAME(name, 64), str);
        usage();
    }

    return ret;
}



Tx::Tx(int argc, const char **argv):
    usrp(0), device(0)
{
    std::string uhd_args = "";
    double freq = TX_FREQ, rate = TX_RATE, gain = TX_GAIN;
    int i;
#ifdef DEBUG
    DSPEW();
    if(argc>0)
        DSPEW("  GOT ARGS");
    for(i=0; i<argc; ++i)
        DSPEW("    ARG[%d]=\"%s\"", i, argv[i]);
#endif

    for(i=0; i<argc; ++i)
    {
        if(!strcmp(argv[i], "--uhd") && i<argc+1)
        {
            uhd_args = argv[++i];
            continue;
        }
        if(!strcmp(argv[i], "--freq") && i<argc+1)
        {
            freq = getDouble(argv[++i]);
            continue;
        }
        if(!strcmp(argv[i], "--rate") && i<argc+1)
        {
            rate = getDouble(argv[++i]);
            continue;
        }
        if(!strcmp(argv[i], "--gain") && i<argc+1)
        {
            gain = getDouble(argv[++i]);
            continue;
        }

        usage();
    }

    // Convert the rate and freq to Hz from MHz
    freq *= 1.0e6;
    rate *= 1.0e6;

    usrp = uhd::usrp::multi_usrp::make(uhd_args);

    crts_usrp_tx_set(usrp, freq, rate, gain);

    DSPEW("usrp->get_pp_string()=\n%s",
            usrp->get_pp_string().c_str());

    device = usrp->get_device();

    metadata.start_of_burst = true;
    metadata.end_of_burst = false;
    metadata.has_time_spec = false; // set to false to send immediately

    DSPEW("TX is initialized");
}


Tx::~Tx(void)
{
    // TODO: delete usrp device; how?

    DSPEW();
}


// len is the number of bytes not complex floats.
ssize_t Tx::write(void *buffer, size_t len, uint32_t channelNum)
{
    // TODO: check for error here, and retry?
    size_t ret = device->send(buffer, len/sizeof(std::complex<float>),
            metadata,
            uhd::io_type_t::COMPLEX_FLOAT32,
            uhd::device::SEND_MODE_FULL_BUFF);

    if(ret != len/sizeof(std::complex<float>))
    {
        WARN("wrote only %zu of complex values", ret, len/sizeof(std::complex<float>));
    }


    if(metadata.start_of_burst)
        // In all future Tx::write() calls this is false.
        metadata.start_of_burst = false;

    return 1; // TODO: what to return????
}


// Define the module loader stuff to make one of these class objects.
CRTSFILTER_MAKE_MODULE(Tx)
