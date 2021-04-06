#include "jammer_c.h"
#include <time.h>

/************** Noise Parameters **************/
#define noise_signal_length 10000
#define noise_mean          0
#define noise_power_w       1
#define noise_std_dev       sqrt(noise_power_w)

void fill_TxBuffer_with_NoiseBPSK(void);



int main(void)
{
    srand(time(0));         // Seed for random num generator. This is used to create noise.

    signal(SIGINT, handle_sig); // Listen to ctrl+c to terminate program from terminal


    // Streaming devices
    struct iio_device *tx;
    struct iio_device *rx;

    // RX and TX sample counters
    size_t nrx = 0;
    size_t ntx = 0;

    configure_pluto(tx, rx);    // Configures PlutoSDR with the parameters set in jammer_c.h file

    printf("* Starting IO streaming (press CTRL+C to cancel)\n");
    while (!stop)
    {
        ssize_t nbytes_rx, nbytes_tx;
        char *p_dat, *p_end;
        ptrdiff_t p_inc;

        // Schedule TX buffer
        //printf("Pushing buffer to pluto \n");

        nbytes_tx = iio_buffer_push(txbuf);
        if (nbytes_tx < 0)
        {
            printf("Error pushing buf %d\n", (int) nbytes_tx);
            shutdown(1);
        }
        //printf("Refilling rx buffer\n");
        // Refill RX buffer
        nbytes_rx = iio_buffer_refill(rxbuf);
        if (nbytes_rx < 0)
        {
            printf("Error refilling buf %d\n",(int) nbytes_rx);
            shutdown(1);
        }

        //printf("Reading data from Rx buffer \n");
        // READ: Get pointers to RX buf and read IQ from RX buf port 0
        p_dat = (char *)iio_buffer_first(rxbuf, rx0_i);
        p_inc = iio_buffer_step(rxbuf);
        p_end = iio_buffer_end(rxbuf);
        
        for (int rx_buf_count = 0; (p_dat < p_end) AND (rx_buf_count<1000000); rx_buf_count++, p_dat += p_inc)
        {
            int16_t i = ((int16_t*)p_dat)[0]; // Real (I)
            int16_t q = ((int16_t*)p_dat)[1]; // Imag (Q)
            //printf("reading data, iter %8d: i = %d, q = %d\n", rx_buf_count, i, q);
        }

        fill_TxBuffer_with_NoiseBPSK();

        // Sample counter increment and status output
        nrx += nbytes_rx / iio_device_get_sample_size(rx);
        ntx += nbytes_tx / iio_device_get_sample_size(tx);
        printf("\tRX %8.2f MSmp, TX %8.2f MSmp\n", nrx/1e6, ntx/1e6);
    }

    shutdown(0);

    return 0;
}


void fill_TxBuffer_with_NoiseBPSK(void)
{
    int16_t noise[noise_signal_length], random_number, i;

    //printf("Generating noise...\n");
    for(i = 0; i < noise_signal_length; i++)
    {
        random_number = rand();
        if(random_number > 0)
            noise[i] = 1;
        else
            noise[i] = -1;
        //printf("bit[%d] = %d\n", i, noise[i]);
    }


    char *p_dat, *p_end;
    ptrdiff_t p_inc;

    //printf("Writing data to tx buffer\n");
    // WRITE: Get pointers to TX buf and write IQ to TX buf port 0
    p_inc = iio_buffer_step(txbuf);
    p_end = iio_buffer_end(txbuf);
    for(i = 0, p_dat = (char *)iio_buffer_first(txbuf, tx0_i); (p_dat < p_end) AND (i < noise_signal_length); p_dat += p_inc, i++)
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
