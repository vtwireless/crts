//
// We copied this circular (ring) buffer idea from GNU radio.
//
// references:
//
// https://www.gnuradio.org/blog/buffers/
//
// https://github.com/gnuradio/gnuradio/blob/master/gnuradio-runtime/lib/vmcircbuf_mmap_shm_open.cc
//
//

extern void *makeRingBuffer(size_t &len, size_t &overhang);

extern void freeRingBuffer(void *x, size_t len, size_t overhang);

