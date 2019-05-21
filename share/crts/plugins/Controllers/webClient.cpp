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


#define MUTEX_LOCK(mutexPtr)                                               \
    ASSERT((errno = pthread_mutex_lock(mutexPtr)) == 0,                    \
            "pthread_mutex_lock() failed")

#define MUTEX_UNLOCK(mutexPtr)                                             \
    ASSERT((errno = pthread_mutex_unlock(mutexPtr)) == 0,                  \
            "pthread_mutex_unlock() failed")


#define DEFAULT_PERIOD  (0.5) // in seconds

// Default address of crts_contestWebServer that this code connects to:
#define DEFAULT_ADDRESS "127.0.0.1"
// DEFAULT_PORT should be consistent with crts_contestWebServer.
#define DEFAULT_PORT    ((uint32_t) 9383)


/*************************************************************************
 *
 *    BIG TODO:  How this module stops needs to be thought through.  It
 *    may not have a robust stopping that catches all TCP requests.  It
 *    may need a shutdown sequence of commands to and from the server to
 *    let both this client and the server shutdown in a synchronous fashion.
 *
 *    and
 *
 *    There's also the BIGGER issue of stream restart.
 *
 *************************************************************************/


static void usage(void)
{
    char nameBuf[64], *name;
    name = CRTS_BASENAME(nameBuf, 64);
    fprintf(stderr,
"\n"
"\n"
"Usage: %s [ OPTIONS ]\n"
"\n"
"  This controller module connects to the CRTS web server letting a browser\n"
"  control and monitor the stream.  It can sends and receive all filter control\n"
"  parameter values for all filters that it finds in all streams that it finds.\n"
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
"   --period PERIOD    set the period in seconds that his writes to the web.  The\n"
"                      default PERIOD is %g\n"
"\n"
"\n"
"   --server_address ADDRESS set the server address to connect to.  The default\n"
"                            server address is " DEFAULT_ADDRESS "\n"
"\n"
"\n"
"   --server_port PORT       set the server port to connect to.  The default port\n"
"                            port is %u\n"
"\n",
name, DEFAULT_PERIOD, DEFAULT_PORT);

}



class Client: public CRTSController
{
    public:

        Client(int argc, const char **argv);
        ~Client(void);

        void start(CRTSControl *c, uint32_t numIn, uint32_t numOut);
        void stop(CRTSControl *c);

        void execute(CRTSControl *c, const void *buffer, size_t len,
                uint32_t channelNum);

        CRTSControl *findControl(std::string name)
        {
            return getControl<CRTSControl *>(name, /*addController*/false);
        };

        unsigned short getOptions(int argc, const char **argv);

        pthread_mutex_t readerMutex;
        pthread_mutex_t writerMutex;

        std::atomic<bool> *isRunning; // pointer to the stream running flag 

        // We can't verify commands until we are in the execute() call.
        // So we just store the commands as strings as they come in.
        // Each CRTSControl has a list of commands.
        std::map<CRTSControl *, std::list<json_t *>> commands;

        bool started;

        unsigned short port;
        const char *address;

        CRTSTcpClient socket;

        double period;


        // We store a list of parameters that have not been pushed to the
        // server yet.  These are parameters that the web clients
        // subscribe to.
        std::map<const std::string, double> parameterChanges;

        std::atomic<bool> runThread;


    private:



        // This function sends a parameter changing event to
        // the web server.
        //
        // TODO: This needs major work.  This is called in the
        // flow stream every time a packet goes from one filter
        // to another.  The socket write call take an order of
        // magnitude longer than the stream inputs.
        //
        void getParameterCB(const char *controlName,
                const std::string parameterName, double value)
        {
            std::string str = "I{\"name\":\"updateParameter\""
                     ",\"args\":[\"";
            str += controlName;
            str += "\",\"";
            str += parameterName;
            str += "\",";

            MUTEX_LOCK(&writerMutex);
            parameterChanges[str] = value;
            MUTEX_UNLOCK(&writerMutex);
        };

};



// This is the thread that writes the socket to the web server for
// web clients that subscribe to "get" parameters.
//
static void *parameterSender(Client *client) {

    std::atomic<bool> &runThread = client->runThread;
    std::map<const std::string, double> &parameterChanges = (client->parameterChanges);
    useconds_t usec = client->period * 1000000;
    pthread_mutex_t *writerMutex = &client->writerMutex;
    CRTSTcpClient &socket = client->socket;

    while(runThread)
    {
        // TODO: use interval timer in place of usleep().
        // or maybe nanosleep().
        //
        usleep(usec);

        MUTEX_LOCK(writerMutex);
        auto it = parameterChanges.begin();

        while(it != parameterChanges.end())
        {
            std::string str = it->first;
            double value = it->second;
            parameterChanges.erase(it);
            MUTEX_UNLOCK(writerMutex);
            str += std::to_string(value);
            str += "]}\004";
            socket.send(str.c_str());
            //SPEW("%s", str.c_str());
            MUTEX_LOCK(writerMutex);
            it = parameterChanges.begin();
        }
        MUTEX_UNLOCK(writerMutex);
    }
    DSPEW("parameterSender thread returning");
    return 0;
}



unsigned short Client::getOptions(int argc, const char **argv)
{
    CRTSModuleOptions opt(argc, argv, usage);

    address = opt.get("--server_address", DEFAULT_ADDRESS);

    period = opt.get("--period", DEFAULT_PERIOD);

    return (port = opt.get("--server_port", DEFAULT_PORT));
}


// Instead of polling for input in the Client::execute() method we
// just use a blocking read in another thread, here:
//
static void *receiver(Client *client)
{
    CRTSTcpClient &socket = client->socket;
    std::atomic<bool> &isRunning = *(client->isRunning);
    json_t *root;
    pthread_mutex_t *readerMutex = &client->readerMutex;

    while((root = socket.receiveJson(isRunning)))
    {
        DSPEW("GOT COMMAND:");
        json_dumpf(root, stderr, 0);
        fprintf(stderr, "\n");

        const char *controlName;
        json_t *commands;

        json_object_foreach(root, controlName, commands)
        {
            DSPEW("controlName=%s", controlName);
            CRTSControl *crtsControl = client->findControl(controlName);
            if(crtsControl)
            {
                if(commands && json_typeof(commands) == JSON_ARRAY)
                {
                    MUTEX_LOCK(readerMutex);
                    client->commands[crtsControl].push_back(commands);
                    MUTEX_UNLOCK(readerMutex);

                    DSPEW("control %s commands:", controlName);
                    json_dumpf(commands, stderr, 0);
                    fprintf(stderr, "\n");
                }
            }
        }

        // else we got crap and we are ignoring it.
    }


    return 0;
}



Client::Client(int argc, const char **argv):
    readerMutex(PTHREAD_MUTEX_INITIALIZER),
    writerMutex(PTHREAD_MUTEX_INITIALIZER),
    started(false),
    port(getOptions(argc, argv)),
    socket(address, port)
{
    runThread = true;
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

    ASSERT(pthread_create(&thread, 0,
                (void *(*) (void *)) parameterSender,
                (void *) this) == 0, "pthread_create() failed");
    ASSERT(pthread_detach(thread) == 0, "pthread_detach() failed");

    DSPEW();
}


Client::~Client(void)
{
    DSPEW();
};


void Client::start(CRTSControl *c, uint32_t numIn, uint32_t numOut)
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
            DSPEW("added TCP/IP client command interface for control:"
                    "%s with %d set parameters",
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
                // Register the callback for when the parameter changes.
                c->getParameter(name, [&,c,name](double value)
                        { getParameterCB(c->getName(), name, value);});

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
    // Process any final requests.  This should get the final values of
    // totalByteIn and totalBytesOut from all the filters (CRTSControl).
    //
    execute(c, 0, 0, 0);

    started = false;
    DSPEW("control %s", c->getName());

    runThread = false;
}


void Client::execute(CRTSControl *c, const void *buffer,
        size_t len, uint32_t channelNum)
{
    //DSPEW("control=\"%s\"", c->getName());

    std::list<json_t *> &list = commands[c];

    MUTEX_LOCK(&this->readerMutex);

    while(!list.empty())
    {
        const size_t BUFLEN = 1024;
        char replyBuff[BUFLEN];
        size_t replyLen = 0;
        // We only reply if there is one or more get commands

        // Array of commands
        json_t *commands = list.front();

#if 1 // debugging spew
        DSPEW("Got control %s COMMANDs:", c->getName());
        json_dumpf(commands, stderr, 0);
        fprintf(stderr, "\n");
#endif

        // jansson json_ documentation:
        //
        // https://jansson.readthedocs.io/en/2.6/apiref.htm
        //
        size_t numCommands = json_array_size(commands);
        if(numCommands < 1)
        {
            WARN("bad request");
            json_decref(commands);
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
                if(parameterName && (
                        json_typeof(obj) == JSON_REAL ||
                        json_typeof(obj) == JSON_INTEGER))
                {
                    double value;
                    if(json_typeof(obj) == JSON_REAL)
                        value = json_real_value(obj);
                    else
                        value = json_integer_value(obj);

                    c->setParameter(parameterName, value);
                }
            }
            else if((obj = json_object_get(command, "get"))) // get a parameter
            {
                const char *parameterName = json_string_value(obj);
                if(parameterName)
                {
                    long long int id = 0;

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
        json_decref(commands);

        list.pop_front();
    }

    MUTEX_UNLOCK(&this->readerMutex);
}


// Define the module loader stuff to make one of these class objects.
CRTSCONTROLLER_MAKE_MODULE(Client)
