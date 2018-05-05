
// We make a special CRTSFilter that feeds the source CRTSFilter plugins.
// We thought of just requiring that the source CRTSFilter plugins have a
// while() loop like this but then we'd loose the ability to inject code
// between CRTSFilter::write() calls for source plugins.  We run these
// Feed Filter Modules in the same thread as the source Filter Modules
// that they feed.
//
class Feed : public CRTSFilter
{
    public:

        Feed(void) {  DSPEW(); };
        ~Feed(void) { DSPEW(); };

        bool start(uint32_t numInputChannels,
                uint32_t numOutputChannels)
        {
            DASSERT(numInputChannels == 0, "");
            DASSERT(numOutputChannels == 1, "");
            DSPEW();
            return false; // success
        };

        bool stop(uint32_t numInputChannels,
                uint32_t numOutputChannels)
        {
            DSPEW();
            return false; // success
        };

        void write(void *buffer, size_t bufferLen,
                uint32_t inputChannelNum);
};
