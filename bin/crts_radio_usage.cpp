#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <atomic>
#include <map>
#include <list>

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp" // CRTSFilter user module interface
#include "pthread_wrappers.h" 
#include "Feed.hpp"
#include "FilterModule.hpp"
#include "Thread.hpp"



int usage(const char *argv0, const char *uopt=0)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    // Keep this function consistent with the argument parsing:

    if(uopt)
        printf("\n Bad option: %s\n\n\n", uopt);

    printf(
        "\n"
        "  Usage: %s OPTIONS\n",
        argv0);

    printf(
"\n"
"    Run the Cognitive Radio Test System (CRTS) transmitter/receiver program.\n"
" Some -f options are required.  The filter stream is setup as the arguments are\n"
" parsed, so stuff happens as the command line options are parsed.\n"
"\n"
"    For you \"system engineers\" the term \"filter\" we mean software filter\n"
" [https://en.wikipedia.org/wiki/Filter_(software)], module component node, or the\n"
" loaded module code that runs and passes data to other loaded module code that\n"
" runs and so on.  Component and node were just to generic a term in a software\n"
" sense.  If you have a hard time stomaching this terminology consider that\n"
" sources and sinks are just filters with null inputs and outputs correspondingly.\n"
" The real reason maybe that the word \"component\" has more letters in it than\n"
" the word \"filter\".   Maybe we should have used the word \"node\"; no too\n"
" generic.  The most general usage the word filter implies a point in a flow, or\n"
" stream.  The words block, component, element, and node do not imply this in the\n"
" most general usage; they have no associated flow.\n"
"\n"
"\n"
"\n"
"                   OPTIONS\n"
"\n"
"\n"
"   -c | --connect LIST              how to connect the loaded filters that are\n"
"                                    in the current stream\n"
"\n"
"                                       Example:\n"
"\n"
"                                              -c \"0 1 1 2\"\n"
"\n"
"                                    connect from filter 0 to filter 1 and from\n"
"                                    filter 1 to filter 2.  This option must\n"
"                                    follow all the corresponding FILTER options.\n"
"                                    Arguments follow a connection LIST will be in\n"
"                                    a new Stream.  After this option and next\n"
"                                    filter option with be in a new different stream\n"
"                                    and the filter indexes will be reset back to 0.\n"
"                                    If a connect option is not given after an\n"
"                                    uninterrupted list of filter options than a\n"
"                                    default connectivity will be setup that connects\n"
"                                    all adjacent filters in a single line.\n"
"\n"
"\n"
"   -C | --controller CNAME          load a CRTS Controller plugin module named CNAME.\n"
"                                    CRTS Controller plugin module need to be loaded\n"
"                                    after the FILTER modules.\n"
"\n"
"\n"
"   -d | --display                   display a DOT graph via dot and imagemagick\n"
"                                    display program, before continuing to the next\n"
"                                    command line options.  This option should be\n"
"                                    after filter options in the command line.  Maybe\n"
"                                    make it the last option.\n"
"\n"
"\n"
"   -D | --Display                   same as option -d but %s will not be\n"
"                                    blocked waiting for the imagemagick display\n"
"                                    program to exit.\n"
"\n"
"\n"
"   -e | --exit                      exit the program.  Used if you just want to\n"
"                                    print the DOT graph after building the graph.\n"
"                                    Also may be useful in debugging your command\n"
"                                    line.\n"
"\n"
"\n"
"   -f | --filter FILTER [OPTS ...]  load filter module FILTER passing the OPTS ...\n"
"                                    arguments to the CRTS Filter constructor.\n"
"\n"
"\n"
"   -h | --help                      print this help and exit\n"
"\n"
"\n"
"   -l | --load G_MOD [OPTS ...]     load general module file passing the OPTS ...\n"
"                                    arguments to the objects constructor.  General\n"
"                                    modules are loaded with symbols shared so that\n"
"                                    you may shared variables across CRTS Filter\n"
"                                    modules through general modules.  General\n"
"                                    modules will only be loaded once for a given\n"
"                                    G_MOD.\n"
"\n"
"                                       TODO: general module example...\n"
"\n"
"\n"
"   -p | --print FILENAME            print a DOT graph to FILENAME.  This should be\n"
"                                    after all filter options in the command line.  If\n"
"                                    FILENAME ends with .png this will write a PNG\n"
"                                    image file to FILENAME.\n"
"\n"
"\n"
"   -t | --thread LIST              run the LIST of filters in a separate thread.\n"
"                                   Without this argument option the program will run\n"
"                                   all filters modules in a single thread for each\n"
"                                   stream.\n"
"\n"
"\n", argv0);

    return 1; // return error status
}
