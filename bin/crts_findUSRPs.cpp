// this is based off of the program:
//
//     uhd_find_devices
//

#include <uhd/device.hpp>
#include <string>
#include <map>
#include <string.h>
#include <stdio.h>



int main(int argc, char *argv[])
{
    uhd::device_addrs_t device_addrs = uhd::device::find(uhd::device_addr_t(""));

    if(argc > 2 || (argc == 2 && argv[1][0] == '-'))
    {
        printf(
"\n"
"  Usage: %s [host]\n"
"\n"
"    Find and print UHD USRP devices as JSON.   Prints to stdout\n"
" a JSON object that has information about the UHD USRP devices\n"
" that are found on the host this program is run on.  On successfully\n"
" finding any devices this will return a 0 status, else no devices\n"
" are found and returns a non zero (error) status.\n"
"\n"
"    Note: this links with buggy libuhd library that prints to\n"
"  stderr (not stdout).\n"
"\n"
"    If one argument is given it will add that argument as the host\n"
" in the printed JSON object, otherwise host is an empty string.\n"
"\n"
"   Example output:\n"
"\n"
"  [\n"
"      {\"type\":\"usrp2\",\"host\":\"\",\"device\":\"192.168.12.2\",\"name\":\"\",\"serial\":\"30EE238\"},\n"
"      {\"type\":\"usrp2\",\"host\":\"\",\"device\":\"192.168.10.3\",\"name\":\"\",\"serial\":\"E0R14N9UP\"}\n"
"  ]\n"
"\n"
"\n",
            argv[0]);

        return EXIT_FAILURE;
    }


    if (device_addrs.size() == 0){
        fprintf(stderr, "No UHD Devices Found\n");
        return EXIT_FAILURE;
    }

    // examples:
    // 
    // device_addrs[i].to_string() is like
    //    
    //    "type=x300,addr=192.168.12.2,fpga=HG,name=,serial=30EE438,product=X310"
    //
    // and/or
    //
    //    "type=usrp2,addr=192.168.10.2,name=,serial=F44901"
    //
    
    int count = 0;

    std::map<std::string, bool> found_devices;

    printf("[\n");

    for (size_t i = 0; i < device_addrs.size(); ++i)
    {
        if(found_devices.find(device_addrs[i]["serial"]) != found_devices.end())
            // We do not print for devices with the same serial number
            // that we already found.
            continue;

        found_devices[device_addrs[i]["serial"]] = true;

        std::string str = device_addrs[i].to_string();

        if(str.length())
        {
            if(count)
                printf(",\n");
            printf("  {");

            char *s = strdup((const char *) (str.c_str()));
            //fprintf(stderr, "%s\n", s);
            char *mem = s;
            char *saveptr;

            s = strtok_r(s, ",", &saveptr);
            int gotTok = 0;

            for(;s; s = strtok_r(0, ",", &saveptr))
            {
                char *key = s;
                char *val = s;
                while(*val && *val != '=')
                    ++val;
                if(*val)
                {
                    *val = '\0';
                    ++val;
                }
                if(gotTok)
                    printf(",");
                else
                    gotTok = 1;

                printf("\"%s\":\"%s\"", key, val);
            }

            printf("}");
            ++count;
            free(mem);
        }
    }

    printf("\n]\n");

    fprintf(stderr, "%d devices found\n", count);

    return EXIT_SUCCESS;
}
