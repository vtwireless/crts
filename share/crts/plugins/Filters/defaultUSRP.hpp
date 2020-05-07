///////////////////////////////////////////////////////////////////////////
// USRP DEFAULT SETTINGS
//
// These settings are not so hard coded, these are just the default
// initial (startup) values, that are the values before command-line
// arguments are parsed.  These USPR UHD parameters may then change due to
// filter module optional arguments.
//
#define RX_FREQ  (914.5) // in MHz = 1e6 Hz
#define RX_RATE  (3.0)   // millions of samples per sec (bandWidth related)
#define RX_GAIN  (0.0)   // 0 is the minimum
#define RX_RESAMPFACTOR (1.0)
//
#define TX_FREQ  (915.5) // in MHz = 1e6 Hz
#define TX_RATE  (3.0)   // millions of samples per sec (bandWidth related)
#define TX_GAIN  (0.0)   // 0 is the minimum
#define TX_RESAMPFACTOR (1.0/RX_RESAMPFACTOR)
///////////////////////////////////////////////////////////////////////////
