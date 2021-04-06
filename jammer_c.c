#include "jammer_c.h"



/* static scratch mem for strings */
char tmpstr[64];
bool stop;



/* IIO structs required for streaming */
struct iio_context *ctx   = NULL;
struct iio_channel *rx0_i = NULL;
struct iio_channel *rx0_q = NULL;
struct iio_channel *tx0_i = NULL;
struct iio_channel *tx0_q = NULL;
struct iio_buffer  *rxbuf = NULL;
struct iio_buffer  *txbuf = NULL;


void configure_pluto(struct iio_device *tx, struct iio_device *rx)
{
    // Stream configurations
    struct stream_cfg rxcfg;
    struct stream_cfg txcfg;

    // RX stream config
    rxcfg.bw_hz = MHZ(10);        // 2 MHz rf bandwidth
    rxcfg.fs_hz = MHZ(2.5);       // 2.5 MS/s rx sample rate
    rxcfg.lo_hz = GHZ(2.44);      // 2.5 GHz rf frequency
    rxcfg.rfport = "A_BALANCED";  // port A (select for rf freq.)

    // TX stream config
    txcfg.bw_hz = MHZ(10);     // 1.5 MHz rf bandwidth
    txcfg.fs_hz = MHZ(2.5);     // 2.5 MS/s tx sample rate
    txcfg.lo_hz = GHZ(2.44);     // 2.5 GHz rf frequency
    txcfg.rfport = "A";         // port A (select for rf freq.)

    printf("* Acquiring IIO context\n");
    ctx = iio_create_context_from_uri("ip:192.168.2.1");

    printf("* Acquiring AD9361 streaming devices\n");
    get_ad9361_stream_dev(ctx, TX, &tx);
    get_ad9361_stream_dev(ctx, RX, &rx);

    printf("* Configuring AD9361 for streaming\n");
    cfg_ad9361_streaming_ch(ctx, &rxcfg, RX, 0);
    cfg_ad9361_streaming_ch(ctx, &txcfg, TX, 0);

    printf("* Initializing AD9361 IIO streaming channels\n");
    get_ad9361_stream_ch(ctx, RX, rx, 0, &rx0_i);
    get_ad9361_stream_ch(ctx, RX, rx, 1, &rx0_q);
    get_ad9361_stream_ch(ctx, TX, tx, 0, &tx0_i);
    get_ad9361_stream_ch(ctx, TX, tx, 1, &tx0_q);

    printf("* Enabling IIO streaming channels\n");
    iio_channel_enable(rx0_i);
    iio_channel_enable(rx0_q);
    iio_channel_enable(tx0_i);
    iio_channel_enable(tx0_q);
}



void handle_sig(int sig)
{
    printf("Waiting for process to finish... Got signal %d\n", sig);
    stop = true;
}



void shutdown(int exitCode)
{
    printf("* Destroying buffers\n");
    if (rxbuf) { iio_buffer_destroy(rxbuf); }
    if (txbuf) { iio_buffer_destroy(txbuf); }

    printf("* Disabling streaming channels\n");
    if (rx0_i) { iio_channel_disable(rx0_i); }
    if (rx0_q) { iio_channel_disable(rx0_q); }
    if (tx0_i) { iio_channel_disable(tx0_i); }
    if (tx0_q) { iio_channel_disable(tx0_q); }

    printf("* Destroying context\n");
    if (ctx) { iio_context_destroy(ctx); }
    exit(exitCode);
}



/* check return value of attr_write function */
void errchk(int v, const char* what)
{
    if (v < 0)
    {
        fprintf(stderr, "Error %d writing to channel \"%s\"\nvalue may not be supported.\n", v, what);
        shutdown(1);
    }
}



/* write attribute: long long int */
void wr_ch_lli(struct iio_channel *chn, const char* what, long long val)
{
    errchk(iio_channel_attr_write_longlong(chn, what, val), what);
}



/* write attribute: string */
void wr_ch_str(struct iio_channel *chn, const char* what, const char* str)
{
    errchk(iio_channel_attr_write(chn, what, str), what);
}



/* helper function generating channel names */
char* get_ch_name(const char* type, int id)
{
    snprintf(tmpstr, sizeof(tmpstr), "%s%d", type, id);
    return tmpstr;
}



/* returns ad9361 phy device */
struct iio_device* get_ad9361_phy(struct iio_context *ctx)
{
    struct iio_device *dev =  iio_context_find_device(ctx, "ad9361-phy");
    IIO_ENSURE(dev && "No ad9361-phy found");
    return dev;
}



/* finds AD9361 streaming IIO devices */
bool get_ad9361_stream_dev(struct iio_context *ctx, enum iodev d, struct iio_device **dev)
{
    switch (d)
    {
        case TX:    *dev = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc");
                    return *dev != NULL;

        case RX:    *dev = iio_context_find_device(ctx, "cf-ad9361-lpc");
                    return *dev != NULL;

        default:    IIO_ENSURE(0);
                    return false;
    }
}



/* finds AD9361 streaming IIO channels */
bool get_ad9361_stream_ch(__notused struct iio_context *ctx, enum iodev d, struct iio_device *dev, int chid, struct iio_channel **chn)
{
    *chn = iio_device_find_channel(dev, get_ch_name("voltage", chid), d == TX);
    if (!*chn)
        *chn = iio_device_find_channel(dev, get_ch_name("altvoltage", chid), d == TX);
    return *chn != NULL;
}



/* finds AD9361 phy IIO configuration channel with id chid */
bool get_phy_chan(struct iio_context *ctx, enum iodev d, int chid, struct iio_channel **chn)
{
    switch (d)
    {
        case RX:    *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("voltage", chid), false);
                    return *chn != NULL;

        case TX:    *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("voltage", chid), true);
                    return *chn != NULL;

        default:    IIO_ENSURE(0);
                    return false;
    }
}



/* finds AD9361 local oscillator IIO configuration channels */
bool get_lo_chan(struct iio_context *ctx, enum iodev d, struct iio_channel **chn)
{
    switch (d)
    {
        // LO chan is always output, i.e. true
        case RX:    *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("altvoltage", 0), true);
                    return *chn != NULL;

        case TX:    *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("altvoltage", 1), true);
                    return *chn != NULL;

        default:    IIO_ENSURE(0);
                    return false;
    }
}



/* applies streaming configuration through IIO */
bool cfg_ad9361_streaming_ch(struct iio_context *ctx, struct stream_cfg *cfg, enum iodev type, int chid)
{
    struct iio_channel *chn = NULL;

    // Configure phy and lo channels
    printf("* Acquiring AD9361 phy channel %d\n", chid);
    if (!get_phy_chan(ctx, type, chid, &chn))
    {
        return false;
    }
    
    wr_ch_str(chn, "rf_port_select",     cfg->rfport);
    wr_ch_lli(chn, "rf_bandwidth",       cfg->bw_hz);
    wr_ch_lli(chn, "sampling_frequency", cfg->fs_hz);

    // Configure LO channel
    printf("* Acquiring AD9361 %s lo channel\n", type == TX ? "TX" : "RX");
    if (!get_lo_chan(ctx, type, &chn))
    {
        return false;
    }
    
    wr_ch_lli(chn, "frequency", cfg->lo_hz);
    return true;
}