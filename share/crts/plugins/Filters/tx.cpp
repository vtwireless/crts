#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <uhd/usrp/multi_usrp.hpp>

#include "crts/debug.h"
#include "crts/Filter.hpp"
#include "crts/crts.hpp"
#include "crts/usrp_set_parameters.hpp" // UHD usrp wrappers

#include "defaultUSRP.hpp" // defaults: TX_FREQ, TX_RATE, TX_GAIN

#include "txControl.hpp"


class Tx : public CRTSFilter
{
    public:

        Tx(int argc, const char **argv);
        ~Tx(void);

        ssize_t write(void *buffer, size_t bufferLen, uint32_t channelNum);

    private:

        TxControl txControl;

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
"   --control NAME  set the name of the CRTS control to NAME.  The default value of\n"
"                   NAME is \"%s\".\n"
"\n"
"\n",
        name, name,
        TX_FREQ, TX_GAIN, TX_RATE, DEFAULT_TXCONTROL_NAME);

    errno = 0;
    throw "usage help"; // This is how return an error from a C++ constructor
    // the module loader will catch this throw.
}



static const char *getControlName(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    return opt.get("--control", DEFAULT_TXCONTROL_NAME);
}



Tx::Tx(int argc, const char **argv):
    txControl(this, getControlName(argc, argv), usrp),
    usrp(0), device(0)
{
    CRTSModuleOptions opt(argc, argv, usage);

    std::string uhd_args = opt.get("--uhd", "");
    double freq = opt.get("--freq", TX_FREQ),
           rate = opt.get("--rate", TX_RATE),
           gain = opt.get("--gain", TX_GAIN);

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
