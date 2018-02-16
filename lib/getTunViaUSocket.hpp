
// Returned the a file descriptor to the new TUN device.
extern int getTunViaUSocket(const char *subnet/* like "10.0.0.1/24" */);

// Returns true on error.
extern bool checkSubnetAddress(const char *subnet/* like "10.0.0.1/24" */);
