#ifndef __usrp_set_parameters_hpp__
#define __usrp_set_parameters_hpp__

// This is NOT intended to be a user API.

// functions that sets and checks RX and TX USRP parameters via UHD API
//
// Very repetitive straight forward wrapper code.

#define CHECK_COMPARE(r, x, y)                                          \
    do {                                                                \
        DSPEW(r " " #x ": %g", y);                                      \
        ASSERT(x <= y * 1.001, r " " #x " setting %g != got %g", x, y); \
        ASSERT(x >= y * 0.999, r " " #x " setting %g != got %g", x, y); \
    } while(0)




static inline void crts_usrp_rx_set(uhd::usrp::multi_usrp::sptr usrp,
        double freq, double rate, double gain)
{
    DASSERT(freq > 0, "");
    DASSERT(rate > 0, "");
    DASSERT(gain >= 0, "");

    uhd::tune_request_t tune;
    tune.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.rf_freq = freq;
    tune.dsp_freq = 0;

    usrp->set_rx_rate(rate);
    CHECK_COMPARE("RX", rate, usrp->get_rx_rate());

    usrp->set_rx_freq(tune);
    CHECK_COMPARE("RX", freq, usrp->get_rx_freq());
 
    usrp->set_rx_gain(gain);
    CHECK_COMPARE("RX", gain, usrp->get_rx_gain());
}


static inline void crts_usrp_tx_set(uhd::usrp::multi_usrp::sptr usrp,
        double freq, double rate, double gain)
{
    DASSERT(freq > 0, "");
    DASSERT(rate > 0, "");
    DASSERT(gain >= 0, "");

    uhd::tune_request_t tune;
    tune.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.rf_freq = freq;
    tune.dsp_freq = 0;

    usrp->set_tx_rate(rate);
    CHECK_COMPARE("TX", rate, usrp->get_tx_rate());

    usrp->set_tx_freq(tune);
    CHECK_COMPARE("TX", freq, usrp->get_tx_freq());
 
    usrp->set_tx_gain(gain);
    CHECK_COMPARE("TX", gain, usrp->get_tx_gain());
}

#endif //#ifndef __usrp_set_parameters_hpp__
