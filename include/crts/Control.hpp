#ifndef __Control_hpp__
#define __Control_hpp__


class CRTSFilter;

class CRTSControl
{

    public:

        virtual ~CRTSControl(void);

        CRTSControl(CRTSFilter *filter, const char *name);

        const char *getName(void) const { return name; };

    private:

        char *name;

        // The filter associated with this control.
        CRTSFilter *filter;
};


#endif // #ifndef __Control_hpp__
