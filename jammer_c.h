#ifndef JAMMER_C_H
#define JAMMER_C_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <iio.h>

#define NOT     !
#define AND     &&
#define OR      ||

#define MHZ(x)  ((long long)(x*1000000.0 + .5))
#define GHZ(x)  ((long long)(x*1000000000.0 + .5))


/**************************** Pluto Configuration Parameters *****************************/
#define GPS_Fc  GHZ(1.5752)
#define ISM_Fc  GHZ(2.44)

#define PlutoTx_Fc          ISM_Fc
#define PlutoTx_Bandwidth   MHZ(10)
#define PlutoTx_SampleRate  MHZ(2.5)

#define PlutoRx_Fc          ISM_Fc
#define PlutoRx_Bandwidth   MHZ(10)
#define PlutoRx_SampleRate  MHZ(2.5)



/* ************************************************************************************* */
#define IIO_ENSURE(expr) { \
    if (!(expr)) { \
        (void) fprintf(stderr, "assertion failed (%s:%d)\n", __FILE__, __LINE__); \
        (void) abort(); \
    } \
}



/* RX is input, TX is output */
enum iodev { RX, TX };


/* common RX and TX streaming params */
extern struct stream_cfg {
    long long bw_hz; // Analog banwidth in Hz
    long long fs_hz; // Baseband sample rate in Hz
    long long lo_hz; // Local oscillator frequency in Hz
    const char* rfport; // Port name
};


/* static scratch mem for strings */
extern char tmpstr[64];

/* IIO structs required for streaming */
extern struct iio_context *ctx;
extern struct iio_channel *rx0_i;
extern struct iio_channel *rx0_q;
extern struct iio_channel *tx0_i;
extern struct iio_channel *tx0_q;
extern struct iio_buffer  *rxbuf;
extern struct iio_buffer  *txbuf;

extern bool stop;




void handle_sig(int sig);
void shutdown(int exitCode);

void errchk(int v, const char* what);                                                        /* check return value of attr_write function */
void wr_ch_lli(struct iio_channel *chn, const char* what, long long val);                    /* write attribute: long long int */
void wr_ch_str(struct iio_channel *chn, const char* what, const char* str);                  /* write attribute: string */

struct iio_device* get_ad9361_phy(struct iio_context *ctx);                                  /* returns ad9361 phy device */
bool get_ad9361_stream_dev(struct iio_context *ctx, enum iodev d, struct iio_device **dev);  /* finds AD9361 streaming IIO devices */
bool get_ad9361_stream_ch(__notused struct iio_context *ctx, enum iodev d, struct iio_device *dev, int chid, struct iio_channel **chn);  /* finds AD9361 streaming IIO channels */
char* get_ch_name(const char* type, int id);                                                         /* helper function generating channel names */
bool get_phy_chan(struct iio_context *ctx, enum iodev d, int chid, struct iio_channel **chn);        /* finds AD9361 phy IIO configuration channel with id chid */
bool get_lo_chan(struct iio_context *ctx, enum iodev d, struct iio_channel **chn);                   /* finds AD9361 local oscillator IIO configuration channels */

bool cfg_ad9361_streaming_ch(struct iio_context *ctx, struct stream_cfg *cfg, enum iodev type, int chid);   /* applies streaming configuration through IIO */

void configure_pluto(struct iio_device *tx, struct iio_device *rx);

#endif
