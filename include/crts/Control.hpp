#ifndef __Control_hpp__
#define __Control_hpp__


class CRTSFilter;

class CRTSControl
{

    public:

        const char *getName(void) const { return name; };

 
    private:

        // We let only CRTSFilter objects create a CRTSControl.
        CRTSControl(CRTSFilter *filter, const char *name);
        virtual ~CRTSControl(void);

        char *name;

        // The filter associated with this control.
        CRTSFilter *filter;
        
        friend CRTSFilter;
};


#endif // #ifndef __Control_hpp__
