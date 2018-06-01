#ifndef __usrp_set_parameters_hpp__
#define __usrp_set_parameters_hpp__

// This is NOT intended to be a user API.

// functions that sets and checks RX and TX USRP parameters via UHD API
//
// Very repetitive straight forward wrapper code.



#define CHECK_COMPARE(r, x, y, ret)                                     \
    do {                                                                \
        DSPEW(r " " #x ": %g", y);                                      \
        if(x > y * 1.001 || x < y * 0.999)                              \
        {                                                               \
            WARN(r " " #x " setting %g != got %g", x, y);               \
            ret = true;                                                 \
        }                                                               \
    } while(0)




static inline bool
crts_usrp_rx_set(uhd::usrp::multi_usrp::sptr usrp,
        double freq, double rate, double gain, size_t chan=0)
{
    DASSERT(freq > 0, "");
    DASSERT(rate > 0, "");
    DASSERT(gain >= 0, "");

    bool ret = false;

    uhd::tune_request_t tune;
    tune.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.rf_freq = freq;
    tune.dsp_freq = 0;

    usrp->set_rx_rate(rate, chan);
    CHECK_COMPARE("RX", rate, usrp->get_rx_rate(chan), ret);
    if(ret) return ret;

    usrp->set_rx_freq(tune, chan);
    CHECK_COMPARE("RX", freq, usrp->get_rx_freq(chan), ret);
    if(ret) return ret;

    usrp->set_rx_gain(gain, chan);
    CHECK_COMPARE("RX", gain, usrp->get_rx_gain(chan), ret);
    return ret;
}


static inline bool
crts_usrp_tx_set(uhd::usrp::multi_usrp::sptr usrp,
        double freq, double rate, double gain, size_t chan=0)
{
    DASSERT(freq > 0, "");
    DASSERT(rate > 0, "");
    DASSERT(gain >= 0, "");

    bool ret = false;

    uhd::tune_request_t tune;
    tune.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.rf_freq = freq;
    tune.dsp_freq = 0;

    usrp->set_tx_rate(rate, chan);
    CHECK_COMPARE("TX", rate, usrp->get_tx_rate(chan), ret);
    if(ret) return ret;

    usrp->set_tx_freq(tune, chan);
    CHECK_COMPARE("TX", freq, usrp->get_tx_freq(chan), ret);
    if(ret) return ret;

    usrp->set_tx_gain(gain, chan);
    CHECK_COMPARE("TX", gain, usrp->get_tx_gain(chan), ret);
    return ret;
}

#endif //#ifndef __usrp_set_parameters_hpp__
