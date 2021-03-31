import numpy as np
import adi


#_________________________________ SDR Configuration ____________________________________
sdr = adi.Pluto('ip:xx.xx.xx.xx')
SDR_Fc = 1575200000     # GPS center frequency is 1575.2 MHz
sdr.tx_lo = SDR_Fc
sdr.rx_lo = SDR_Fc
# figure out what the following mean:
# sdr.tx_cyclic_buffer = True
# sdr.tx_hardwaregain = -30
# sdr.gain_control_mode = "slow_attack"


#______________________________ Noise signal generation _________________________________
noise_signal_length = 10000
noise_mean = 0
noise_power_w = 10
noise_std_dev = np.sqrt(noise_power_w)

# Generate a random noise signal
noise = np.random.normal(noise_mean, noise_std_dev, noise_signal_length)

# Create BPSK modulated bitstream for the random signal
noise_bpsk = []
for i in noise:
    if(i > 0):
        bit = 1
    else:
        bit = -1
    noise_bpsk.append(bit)

# Transmit the signal using SDR
sdr.tx(noise_bpsk)