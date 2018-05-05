#include <stdio.h>

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp"
#include "Feed.hpp"


void Feed::write(void *buffer, size_t bufferLen,
                uint32_t inputChannelNum)
{
    DASSERT(buffer == 0,"");
    DASSERT(bufferLen == 0, "");
    // We have no input.
    DASSERT(inputChannelNum == NULL_CHANNEL, "");

    // This is the start of the flow in the stream to all the filters in
    // the stream flow graph.  Feed is a special CRTSFilter that has this
    // simple loop that runs in the thread of the what was the source
    // filters threads.  Since this filter is now the source, that what
    // was source filters are now feed by this feed source filter, making
    // the original users source filters become receiving filters from
    // this Feed filter.
    //
    // The other non-Feed filters should not loop like this.  They should
    // do just one round of writePush()'s for a given input channel
    // number.
    //
    // This is the only filter that has no feed source filter.

    while(stream->isRunning)
        // Trigger all output channels with 0 byte inputs.
        //
        // This is the start of the stream flow.
        //
        // We only have one output channel for this special Feed source
        // filter.
        //
        writePush(0/* length */, 0/* output channel number */);
}
