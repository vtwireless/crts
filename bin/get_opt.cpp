#include <string.h>
#include <string>

#include "crts/debug.h"
#include "get_opt.hpp"

// Look for a command-line option with short and long option flags
// at index i, and advance i if it's found.
//
// Returns a pointer to the option string or 0 if it's not found,
// and i is not advanced.
//
// Example:
//
//   int i = 1;
//   std::string filename = "defaultFilename";
//
//   while(i < argc)
//   {
//      /* look for other options */
//
//      if(get_opt(filename, "-f", "--file", argc, argv, i))
//          continue;
//
//      if(bla bla)
//      {
//          ++i;
//          continue;
//      }
//
//      printf("Invalid option(s): %s\n", argv[i]);
//      return 1; // FAIL
//   }
//
//   printf("filename=%s\n", filename.c_str());
//
//

bool get_opt(std::string &value,
        const char * const shorT,
	const char * const lonG,
	int argc, const char **argv, int &i)
{
    // check for form long=VALUE
    // like  --long=VALUE
    size_t len = 0;
    if(i < argc)
    {
        char *str = (char *) argv[i];
        for(;*str && *str != '=';str++);
        if(*str == '=')
        {
            len = (size_t) str - (size_t) argv[i];
            str++;
        }
        if(len && !strncmp(argv[i],lonG,len)
	    &&  *str != '\0')
	{
            ++i;
            value = str;
            return true;
        }
    }
  
    // check for  short VALUE   or   long VALUE
    //
    // like -a VALUE  or --long VALUE
    if(((i + 1) < argc) &&
        (!strcmp(argv[i],lonG) ||
        (shorT && shorT[0] && !strcmp(argv[i],shorT))))
    {
        ++i;
        value = argv[i++];
        return true;
    }

    if(!shorT || !shorT[0]) return 0;
    // check for   shortVALUE  like -aVALUE
    len=strlen(shorT);
    if( (i < argc) && !strncmp(argv[i],shorT,len)
            && argv[i][len] != '\0')
    {
        value = &(argv[i++][len]);
        return true;
    }

    return false;
}

bool get_opt(
            std::string &moduleName,
            int &moduleArgc,
            const char ** &moduleArgv,
            const char * const shorT,
	    const char * const lonG,
	    int argc, const char **argv, int &i)
{
    if(!get_opt(moduleName, shorT, lonG, argc, argv, i))
        return false;


    // Example:  crts_radio -i stdin [ --buffer-size 100 ] ...
    //
    //     argv[i-1] points to "stdin"
    //
    //     Now point to options:
    //
    //            moduleArgv[0] points to "--buffer-size"
    //

    moduleArgc = 0;
    moduleArgv = 0;

    if(i >= argc || argv[i][0] != '[' || argv[i][1] != '\0') return true;

    // We have an open bracket as an argument "[" at i

    // Go to the next command line arg:
    ++i;

    // This may not be pointing to anything but
    // currently  moduleArgc is 0  so that's okay.
    moduleArgv = &argv[i];

    // We skipped over the "["
    //
    // Go to next argv string if it's there and not
    for(; i < argc; ++i)
    {
        // Check that the argc and argv are consistent like they should be
        // when they are passed into main().
        DASSERT(argv[i], "");

        // look for a good arg string
        if(argv[i][0] != ']' || argv[i][1] != '\0')
            // It is a good module string arg
            ++moduleArgc;
        else
        {
            ++i;
            break;
        }
    }
    return true;
}
