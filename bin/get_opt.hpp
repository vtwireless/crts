// Yes Yes I know another stupid argument parser class.
//
// But that's just how it is, most argument parsers are of piss poor
// quality, or get in the way of the functionality that we must have.
// We cannot use getopt() or GNU getopt_long().  The quality is top,
// but they stop us from getting what we need.


// See get_opt.cpp for details.
//
extern bool get_opt(
            std::string &value,
            const char * const shorT,
	    const char * const lonG,
	    int argc, const char **argv, int &i);


// Getting options strings from main(int argc, char **argv)
//
// Example, works with bash and tcsh shells:
//
//     crts_radio -i stdin [ --buflen 100 --flush ]
//
//     calling:
//
//        get_opt(inputModule, moduleArgv, moduleArgc, "-i", "--input", i)
//
//
//         gets      moduleArgv[] = { "--buflen", "100",  "--flush", ... }
//         and       moduleArgc = 3
//
//
extern bool get_opt(
            std::string &moduleName,
            int &moduleArgc,
            const char ** &moduleArgv,
            const char * const shorT,
	    const char * const lonG,
	    int argc, const char **argv, int &i);


static inline bool get_opt_double(double &val,
            const char * const shorT,
	    const char * const lonG,
	    int argc, const char **argv, int &i)
{
    std::string str;

    if(get_opt(str, shorT, lonG, argc, argv, i))
    {
        errno = 0;
        char *endptr = 0;
        const char *s = str.c_str();
        val = strtod(s, &endptr);
        if(endptr == s || errno == ERANGE)
            ASSERT(0, "Argument: \"%s\" = \"%s\" could not be "
                "converted into a double\n", argv[i-1], s);

        return true;
    }

    return false;
}
