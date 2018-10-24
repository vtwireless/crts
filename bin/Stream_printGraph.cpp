#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <map>
#include <list>

#include "crts/debug.h"
#include "crts/crts.hpp"
#include "LoadModule.hpp"
#include "pthread_wrappers.h"
#include "crts/crts.hpp"
#include "crts/Filter.hpp"
#include "Feed.hpp"
#include "FilterModule.hpp"
#include "Thread.hpp"
#include "Stream.hpp"



// ref:
//   https://en.wikipedia.org/wiki/DOT_(graph_description_language)
//
// Print a DOT graph to filename or PNG image of a directed graph
// return false on success
bool Stream::printGraph(const char *filename, bool _wait)
{
    // This is the main thread.
    DASSERT(pthread_equal(Thread::mainThread, pthread_self()), "");

    DSPEW("Writing DOT graph to: \"%s\"", filename);

    FILE *f;

    if(!filename || !filename[0])
    {
        // In this case we run dot and display the images assuming
        // the program "display" from imagemagick is installed in
        // the users path.

        errno = 0;
        f = tmpfile();
        if(!f)
        {
            ERROR("tmpfile() failed");
            return true; // failure
        }

        bool ret = printGraph(f);
        if(ret)
        {
            fclose(f);
            return ret; // failure
        }

        fflush(f);
        rewind(f);

        pid_t pid = fork();
        if(pid == 0)
        {
            // I'm the child
            errno = 0;
            if(0 != dup2(fileno(f), 0))
            {
                WARN("dup2(%d, %d) failed", fileno(f), 0);
                exit(1);
            }
            DSPEW("Running dot|display");
            // Now stdin is the DOT graph file
            // If this fails there's nothing we need to do about it.
            execl("/bin/bash", "bash", "-c", "dot|display", (char *) 0);
            exit(1);
        }
        else if(pid >= 0)
        {
            // I'm the parent
            fclose(f);

            if(_wait)
            {
                int status = 0;
                INFO("waiting for child display process", status);
                errno = 0;
                // We wait for just this child.
                if(pid == waitpid(pid, &status, 0))
                    INFO("child display process return status %d",
                            status);
                else
                    WARN("child display process gave a wait error");
            }
        }
        else
        {
            ERROR("fork() failed");
        }

        return false; // success, at least we tried so many thing can fail
        // that we can't catch all failures, like X11 display was messed
        // up.
    }

    size_t flen = strlen(filename);

    if(flen > 4 && (!strcmp(&filename[flen - 4], ".png") ||
            !strcmp(&filename[flen - 4], ".PNG")))
    {
        // Run dot and generate a PNG image file.
        //
        const char *pre = "dot -o "; // command to run without filename
        char *command = (char *) malloc(strlen(pre) + flen + 1);
        sprintf(command, "%s%s", pre, filename);
        errno = 0;
        f = popen(command, "w");
        if(!f)
        {
            ERROR("popen(\"%s\", \"w\") failed", command);
            free(command);
            return true; // failure
        }
        free(command);
        bool ret = printGraph(f);
        pclose(f);
        return ret;
    }
    
    // else
    // Generate a DOT graphviz file.
    //
    f = fopen(filename, "w");
    if(!f)
    {
        ERROR("fopen(\"%s\", \"w\") failed", filename);
        return true; // failure
    }
        
    bool ret = printGraph(f);
    fclose(f);
    return ret;
}


// This just writes DOT content to the file with no other options.
bool Stream::printGraph(FILE *f)
{
    DASSERT(f, "");

    uint32_t n = 0; // stream number

    fprintf(f,
            "// This is a generated file\n"
            "\n"
            "// This is a DOT graph file.  See:\n"
            "//  https://en.wikipedia.org/wiki/DOT_"
            "(graph_description_language)\n"
            "\n"
            "// There are %zu filter streams in this graph.\n"
            "\n", Stream::streams.size()
    );

    fprintf(f, "digraph {\n");

    for(auto stream : streams)
    {
        fprintf(f,
                "\n" // The word "cluster_" is needed for dot.
                "  subgraph cluster_stream_%" PRIu32 " {\n"
                "    label=\"Stream %" PRIu32 "\";\n"
                , n, n);

        for(auto pair : stream->map)
        {
            FilterModule *filterModule = pair.second;

            if(dynamic_cast<Feed *>(filterModule->filter))
                // Skip displaying the Feed filters
                continue;

            char wNodeName[64]; // writer node name

            snprintf(wNodeName, 64, "f%" PRIu32 "_%" PRIu32, n,
                    filterModule->loadIndex);

            // example f0_1 [label="stdin(0)\n1"] for thread 1
            fprintf(f, "    %s [label=\"%s\\nthread %" PRIu32 "\"];\n",
                    wNodeName,
                    filterModule->name.c_str(),
                    (filterModule->thread)?
                        (filterModule->thread->threadNum):0
                    );

            for(uint32_t i = 0; i < filterModule->numOutputs; ++i)
            {
                char rNodeName[64]; // reader node name
                snprintf(rNodeName, 64, "f%" PRIu32 "_%" PRIu32, n,
                        filterModule->outputs[i]->toFilterModule->loadIndex);

                fprintf(f, "    %s -> %s;\n", wNodeName, rNodeName);
            }
        }

        fprintf(f, "  }\n");

        ++n;
    }

    const auto &controllers = CRTSController::controllers;

    if(controllers.size())
    {
        n = 0; // filter numbering starting again.

        fprintf(f,
                "\n"// The word "cluster_" is needed for dot.
                "  subgraph cluster_controllers {\n"
                "    label=\"Controllers\";\n"
        );

        // Loop through all the filters and the Controllers in each one.
        for(auto const stream : streams)
        {
            for(auto const pair : stream->map)
            {
                FilterModule *filterModule = pair.second;

                if(dynamic_cast<Feed *>(filterModule->filter))
                    // Skip displaying the Feed filters
                    continue;

                char filterName[64]; // writer node name

                snprintf(filterName, 64, "f%" PRIu32 "_%" PRIu32,
                        n, filterModule->loadIndex);

                for(auto const &controller: filterModule->filter->control->controllers)
                {
                    fprintf(f, "    controller_%"
                                PRIu32 " [label=\"%s\(%"
                                PRIu32 ")\"];\n",
                                controller->getId(),
                                controller->getName(),
                                controller->getId());
                    fprintf(f, "    controller_%"
                                PRIu32 " -> %s [color=\"brown1\"];\n",
                                controller->getId(), filterName);
                }
            }
            ++n; // next filter number.
        }

        fprintf(f,"  }\n"); // subgraph cluster_controllers
    }

    fprintf(f, "}\n");

    return false; // success
}
