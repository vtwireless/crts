
class CRTSTimer
{
    public:

        CRTSTimer(void);

        inline double GetDouble(void)
        {
            struct timespec t;
            ASSERT(clock_gettime(CLOCK_MONOTONIC_RAW, &t) == 0, "");
            return (t.tv_sec - _t.tv_sec + 1.0e-9 *(t.tv_nsec - _t.tv_nsec));
        }

    private:

        struct timespec _t;
};
