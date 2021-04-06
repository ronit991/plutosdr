#include "jammer_c.h"
#include <time.h>

#define noise_signal_length 10000
#define noise_mean          0
#define noise_power_w       1
#define noise_std_dev       sqrt(noise_power_w)

void fill_TxBuffer_With_NoiseBPSK(void);

int main(void)
{
    srand(time(0));         // Seed for random num generator. This is used to create noise.

    // Streaming devices
    struct iio_device *tx;
    struct iio_device *rx;

    // RX and TX sample counters
    size_t nrx = 0;
    size_t ntx = 0;

    // Stream configurations
    struct stream_cfg rxcfg;
    struct stream_cfg txcfg;

    // Listen to ctrl+c and IIO_ENSURE
    signal(SIGINT, handle_sig);

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

    printf("* Creating non-cyclic IIO buffers with 1 MiS\n");
    rxbuf = iio_device_create_buffer(rx, 1024*1024, false);
    if (!rxbuf) {
        perror("Could not create RX buffer");
        shutdown();
    }
    txbuf = iio_device_create_buffer(tx, 1024*1024, false);
    if (!txbuf) {
        perror("Could not create TX buffer");
        shutdown();
    }

    printf("* Starting IO streaming (press CTRL+C to cancel)\n");
    while (!stop)
    {
        ssize_t nbytes_rx, nbytes_tx;
        char *p_dat, *p_end;
        ptrdiff_t p_inc;

        // Schedule TX buffer
        nbytes_tx = iio_buffer_push(txbuf);
        if (nbytes_tx < 0) { printf("Error pushing buf %d\n", (int) nbytes_tx); shutdown(); }

        // Refill RX buffer
        nbytes_rx = iio_buffer_refill(rxbuf);
        if (nbytes_rx < 0) { printf("Error refilling buf %d\n",(int) nbytes_rx); shutdown(); }

        // READ: Get pointers to RX buf and read IQ from RX buf port 0
        p_inc = iio_buffer_step(rxbuf);
        p_end = iio_buffer_end(rxbuf);
        for (p_dat = (char *)iio_buffer_first(rxbuf, rx0_i); p_dat < p_end; p_dat += p_inc) {
            // Example: swap I and Q
            const int16_t i = ((int16_t*)p_dat)[0]; // Real (I)
            const int16_t q = ((int16_t*)p_dat)[1]; // Imag (Q)
            ((int16_t*)p_dat)[0] = q;
            ((int16_t*)p_dat)[1] = i;
        }

        fill_TxBuffer_With_NoiseBPSK();
        



        // Sample counter increment and status output
        nrx += nbytes_rx / iio_device_get_sample_size(rx);
        ntx += nbytes_tx / iio_device_get_sample_size(tx);
        printf("\tRX %8.2f MSmp, TX %8.2f MSmp\n", nrx/1e6, ntx/1e6);
    }

    shutdown();

    return 0;
}


void fill_TxBuffer_With_NoiseBPSK(void)
{
    int noise[noise_signal_length];
    int random_number, i;

    for(i = 0; i < noise_signal_length; i++)
    {
        random_number = rand();
        if(random_number > 0)
            noise[i] = 1;
        else
            noise[i] = -1;
    }


    char *p_dat, *p_end;
    ptrdiff_t p_inc;

    // WRITE: Get pointers to TX buf and write IQ to TX buf port 0
    p_inc = iio_buffer_step(txbuf);
    p_end = iio_buffer_end(txbuf);
    for(i = 0, p_dat = (char *)iio_buffer_first(txbuf, tx0_i); p_dat < p_end; p_dat += p_inc, i++)
    {
        // 12-bit sample needs to be MSB aligned so shift by 4
        // https://wiki.analog.com/resources/eval/user-guides/ad-fmcomms2-ebz/software/basic_iq_datafiles#binary_format
        ((int16_t*)p_dat)[0] = 0 << 4; // Real (I)
        ((int16_t*)p_dat)[1] = noise[i] << 4; // Imag (Q)
        /* Since we are BPSK modulating our noise, the real part is 0, and the imaginary part
         * is either iota(1) or -iota(-1). This is generated in the previous loop randomly &
         * is stored in the noise array.    */
    }
}
