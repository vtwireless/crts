// This CRTSController module connects to a TCP/IP server and relays
// commands from that server to the filter stream.
// 
#include <stdio.h>
#include <math.h>
#include <values.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <string>
#include <list>
#include <map>
#include <atomic>

#include <crts/crts.hpp>
#include <crts/debug.h>
#include <crts/Controller.hpp>
#include <crts/Filter.hpp>


#define MUTEX_LOCK(client)                                               \
    ASSERT((errno = pthread_mutex_lock(&client->mutex)) == 0,            \
            "pthread_mutex_lock() failed")

#define MUTEX_UNLOCK(client)                                             \
    ASSERT((errno = pthread_mutex_unlock(&client->mutex)) == 0,          \
            "pthread_mutex_unlock() failed")

// Default address of crts_scenarioWebServer
#define DEFAULT_ADDRESS "127.0.0.1"
// DEFAULT_PORT should be consistent with crts_scenarioWebServer
#define DEFAULT_PORT    ((uint32_t) 9383)



static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  OPTIONS are optional.  Transmitter Controller test module.\n"
"\n"
"\n"
"  ---------------------------------------------------------------------------\n"
"                           OPTIONS\n"
"  ---------------------------------------------------------------------------\n"
"\n"
"\n"
"   --help             print this help\n"
"\n"
"\n"
"   --server_address ADDRESS set the server address to connect to.  The default\n"
"                            server address is " DEFAULT_ADDRESS "\n"
"\n"
"\n"
"   --server_port PORT       set the server port to connect to.  The default port\n"
"                            port is %u\n"
"\n"
"   --help             print this help\n"
"\n",
name, DEFAULT_PORT);

}



class Client: public CRTSController
{
    public:

        Client(int argc, const char **argv);
        ~Client(void);

        void start(CRTSControl *c);
        void stop(CRTSControl *c);

        void execute(CRTSControl *c, const void *buffer, size_t len,
                uint32_t channelNum);

        CRTSControl *findControl(std::string name)
        {
            return getControl<CRTSControl *>(name, /*addController*/false);
        };

        unsigned short getPortAndAddressFromArgs(int argc, const char **argv);

        pthread_mutex_t mutex;

        std::atomic<bool> *isRunning; // pointer to the stream running flag 

        // We can't verify commands until we are in the execute() call.
        // So we just store the commands as strings as they come in.
        // Each CRTSControl has a list of commands.
        std::map<CRTSControl *, std::list<json_t *>> commands;

        bool started;

        unsigned short port;
        const char *address;

        CRTSTcpClient socket;
};



unsigned short Client::getPortAndAddressFromArgs(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    address = opt.get("server_address", DEFAULT_ADDRESS);

    return (port = opt.get("server_port", DEFAULT_PORT));
}

/* Find the path to the program "crts_shell" by assuming that it is in the
 * same directory as the current running program "crts_radio".
 *
 * Note: This code is very Linux specific, using /proc/.
 */
static inline std::string getExePath(void)
{
    ssize_t bufLen = 128;
    char *buf = (char *) malloc(bufLen);
    ASSERT(buf, "malloc() failed");
    ssize_t rl = readlink("/proc/self/exe", buf, bufLen);
    ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    while(rl >= bufLen)
    {
        DASSERT(bufLen < 1024*1024, ""); // it should not get this large.
        buf = (char *) realloc(buf, bufLen += 128);
        ASSERT(buf, "realloc() failed");
        rl = readlink("/proc/self/exe", buf, bufLen);
        ASSERT(rl > 0, "readlink(\"/proc/self/exe\",,) failed");
    }

    // Strip off the filename leaving just the directory file path.
    // Example:   /foo/bin/crts_radio  => /foo/bin
    char *s = &buf[rl];
    while(--s > buf && *s != '/') *s = '\0';
    ASSERT(s > buf, "Failed to find the path to this program");
    *s = '\0';

    std::string path = buf;
    path += "/crts_shell";
    free(buf);

    return path;
}


// Instead of polling for input in the Client::execute() method we
// just use a blocking read in another thread, here:
//
static void *receiver(Client *client)
{
    CRTSTcpClient &socket = client->socket;
    std::atomic<bool> &isRunning = *(client->isRunning);
    json_t *root;

    while((root = socket.receiveJson(isRunning)))
    {
        json_t *control = json_object_get(root, "control");

        // Server to Client protocol:
        //
        /* The JSON command request should be of the form:
         *
         *   {
         *     id: requestID_number,
         *     control: "controlName0",
         *     commands: [
         *              { set: [ "parameterName0", 1.0e-4 ]},
         *              { get: "parameterName1"},
         *              { set: [ "parameterName2", 2.0e-6 ]}
         *          ]
         *   }
         *
         * By putting the many commands in one request we can issue all
         * commands in the request in one controller execute() call.
         * Being able to issue all the commands in one controller
         * execute() call could be very important.
         *
         * We can only make commands act in the same controller execute()
         * call if they are in the same CRTSControl, hence we have just
         * one control for a given command request.
         */

        if(control && json_typeof(control) == JSON_STRING)
        {
            const char *controlName = json_string_value(control);
            if(controlName)
            {
                CRTSControl *crtsControl = client->findControl(controlName);
                if(crtsControl)
                    // Add a command to the list for this control.
                    client->commands[crtsControl].push_back(root);
            }
        }
        // else we got crap.

        DSPEW("GOT COMMAND:");
        json_dumpf(root, stderr, 0);
        fprintf(stderr, "\n");
    }


    return 0;
}



Client::Client(int argc, const char **argv):
    mutex(PTHREAD_MUTEX_INITIALIZER), started(false),
    port(getPortAndAddressFromArgs(argc, argv)),
    socket(address, port)
{
    // We must add this CRTSController to all filter controller callbacks
    // so that Client::start(c), Client::execute(c) and Client::stop(c) get
    // called, where c is the CRTSControl for each CRTSFilter.
    //
    CRTSControl *c = getControl<CRTSControl*>("", true/*add this controller*/,
            true/*start iterating all controls*/);
    //
    while(getControl<CRTSControl*>("",true,false));

    isRunning = &(c->isRunning);

    pthread_t thread;
    ASSERT(pthread_create(&thread, 0,
                (void *(*) (void *)) receiver,
                (void *) this) == 0, "pthread_create() failed");
    ASSERT(pthread_detach(thread) == 0, "pthread_detach() failed");

    DSPEW();
}


Client::~Client(void)
{
    DSPEW();
};


void Client::start(CRTSControl *c)
{
    // This function will get called once for each filter/control
    // but we just need it to do anything once, so we use this
    // started flag to not do this stuff again.
    //
    if(started) return;
    started = true;


    // There may be controls added after the filters are connected so
    // most things happen here and not in the constructor.
    //
    // This Controller should be loaded after all Filters that you wish to
    // control.
    //

    // Make a JSON string of commands
    // that we will send to the server.
    //
    // TODO: Maybe rewrite this with a JSON parser API.
    // Stringifying JSON is easier than parsing JSON.
    //
    //
    // Client to Server Protocol send:
    //
    //   see lib/socketIO.js this is doing Emit('controlList', {json}).
    //
    //
    // controlList is the buffer we will send.
    //
    std::string controlList("I{\"name\": \"controlList\",\n"
            "  \"args\":[\n    {");

    c = getControl<CRTSControl*>("",
        false/*add controller*/,
        true/*start iteration*/);

    // We go through the list of controls twice
    // once for setters and once for getters.
    int ccount = 0;

    while(c)
    {
        bool haveSetter, haveGetter;

        // Create a command list for this control:
        //
        commands[c] = std::list<json_t *>();

        //
        //
        //  a JSON of setters looks like:
        //
        //  {
        //      "filterName0":
        //          [ "freq",  "gain",  "foo",  "bar", "baz" ]
        //      ,
        //      "filterName1":
        //          [ "freq0",  "gain2",  "foo",  "bar", "baz" ]
        //  }
        // 
        // where "freq",  "gain",  "foo",  "bar", "baz" are parameter
        // names.

        //
        // Now Setters
        //
        std::string name = c->getNextParameterName(true/*start*/,
               &haveSetter, &haveGetter);
        int pcount = 0;
        while(name.length())
        {
            if(!pcount && haveSetter)
            {
                if(ccount) controlList += ",";
                controlList += "\n      \"";
                controlList += c->getName();
                controlList += "\": [";
            }
            if(haveSetter)
            {
                if(pcount) controlList += ", ";
                ++pcount;
                controlList += "\"";
                controlList += name + "\"";
            }

            // go to next Parameter
            name = c->getNextParameterName(false, &haveSetter, &haveGetter);
        }

        if(pcount)
        {
            // We got at least one parameter in a control, now finish it off.
            controlList += "]\n";
            ++ccount; // We have another control with parameters.
#if 0
            DSPEW("added TCP/IP client command interface for control: %s with %d set parameters",
                    c->getName(), pcount);
#endif
        }

        // Goto next CRTSControl
        //
        c = getControl<CRTSControl*>("", false, false);
    }


    controlList += "\n    },\n    {";
    ccount = 0;

    c = getControl<CRTSControl*>("",
        false/*add controller*/,
        true/*start iteration*/);

    while(c)
    {
        bool haveSetter, haveGetter;

        // Now Getters
        //
        //  a JSON of getters line looks like:
        //
        //  {
        //      "filterName0":
        //          [ "freq",  "gain",  "foo",  "bar", "baz" ]
        //      ,
        //      "filterName1":
        //          [ "freq",  "gain",  "foo",  "bar", "baz" ]
        //  }
        //
        //
        //  where "freq",  "gain",  "foo",  "bar", "baz" are parameter
        //  names.
        //

        std::string name = c->getNextParameterName(true/*start*/,
               &haveSetter, &haveGetter);
        int pcount = 0;
        while(name.length())
        {
            if(!pcount && haveGetter)
            {
                if(ccount) controlList += ",";
                controlList += "\n      \"";
                controlList += c->getName();
                controlList += "\": [";
            }
            if(haveGetter)
            {
                if(pcount) controlList += ", ";
                ++pcount;
                controlList += "\"";
                controlList += name + "\"";
            }

            // go to next Parameter
            name = c->getNextParameterName(false, &haveSetter, &haveGetter);
        }

        if(pcount)
        {
            // We got at least one parameter in a control, now finish it off.
            controlList += "]\n";
            ++ccount; // We have another control with parameters.
#if 0
            DSPEW("added TCP/IP client command interface for "
                    "control: %s with %d set parameters",
                    c->getName(), pcount);
#endif
        }


        // Goto next CRTSControl
        //
        c = getControl<CRTSControl*>("", false, false);
    }

    if(commands.size() <= 0) 
        throw "No controls were found";

    controlList += "\n    }, \"";

    socket.send(controlList.c_str());

    // Now we send the base 64 encoded PNG data. 
    //
    printStreamGraphDotPNG64(socket.getFd());

    // Terminate the JSON. // We use ascii EOT as a end marker.
    socket.send("\"]}\004"); 

    DSPEW("controls JSON=\n%s", controlList.c_str());
}


void Client::stop(CRTSControl *c)
{
    // TODO: more code here.
    started = false;
    DSPEW("control %s", c->getName());
}


void Client::execute(CRTSControl *c, const void *buffer,
        size_t len, uint32_t channelNum)
{
    //DSPEW("control=\"%s\"", c->getName());

    std::list<json_t *> &list = commands[c];

    MUTEX_LOCK(this);

    while(!list.empty())
    {
        const size_t BUFLEN = 1024;
        char replyBuff[BUFLEN];
        size_t replyLen = 0;
        // We only reply if there is one or more get commands

        json_t *root = list.front();

#if 1 // debugging spew
        DSPEW("Got COMMAND:");
        json_dumpf(root, stderr, 0);
        fprintf(stderr, "\n");
#endif

        // jansson json_ documentation:
        //
        // https://jansson.readthedocs.io/en/2.6/apiref.htm
        //
        json_t *commands = json_object_get(root, "commands");
        size_t numCommands = json_array_size(commands);
        json_t *idObj = json_object_get(root, "id");
        json_int_t id = json_integer_value(idObj);
        if(numCommands < 1 || !idObj || json_is_integer(idObj))
        {
            WARN("bad request");
            json_decref(root);
            list.pop_front();
            continue;
        }
        size_t i;
        for(i=0; i<numCommands; ++i)
        {
            json_t *command = json_array_get(commands, i);
            json_t *obj;

            if((obj = json_object_get(command, "set")) // set a parameter
                    && json_typeof(obj) == JSON_ARRAY && json_array_size(obj) == 2)
            {
                const char *parameterName = json_string_value(json_array_get(obj, 0));
                obj = json_array_get(obj, 1);
                if(parameterName && json_typeof(obj) == JSON_REAL)
                {
                    double value = json_real_value(obj);
                    c->setParameter(parameterName, value);
                }
            }
            else if((obj = json_object_get(command, "get"))) // get a parameter
            {
                const char *parameterName = json_string_value(obj);
                if(parameterName)
                {
                    if(replyLen == 0)
                    {
                        replyLen = snprintf(replyBuff, BUFLEN,
                        //
                        // Client to Server protocol:
                        //
                        //  reply to get request with:  G{JSON}EOT
                        //
                        "G{\"id\":%" JSON_INTEGER_FORMAT
                        ",\"control\":\"%s\""
                        ",\"replies\":[",
                        id, c->getName());
                        ASSERT(replyLen > 0, "snprintf() failed");
                    }
                    else if(replyLen + 1 < BUFLEN)
                    {
                        replyBuff[replyLen++] = ',';
                        replyBuff[replyLen] = '\0';
                    }

                    if(replyLen + 1 < BUFLEN)
                        replyLen += snprintf(&replyBuff[replyLen], BUFLEN-replyLen,
                            "{\"get\":[\"%s\",%lg]}",
                            parameterName,
                            c->getParameter(parameterName));
                }
            }
        }

        if(replyLen && replyLen + 3 < BUFLEN)
        {
            replyLen += snprintf(&replyBuff[replyLen], BUFLEN-replyLen, "]}\004");
            socket.send(replyBuff);
            DSPEW("sent reply: %s", replyBuff);
        }

        // Free the json object.
        //
        json_decref(root);

        list.pop_front();
    }

    MUTEX_UNLOCK(this);
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(Client)
