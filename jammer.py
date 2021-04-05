# !!! Important !!! - Code in this file is indented using 4-spaces.

import numpy as np
import adi


#_________________________________ SDR Configuration ____________________________________
sdr = adi.Pluto("ip:192.168.2.1")
#SDR_Fc = 1575200000     # GPS center frequency is 1575.2 MHz
SDR_Fc = 2440000000		# Unlicense Band (India) frequency range 2.4-2.4835 GHz (for indoor low-power equipment)
						# So we are currently using 2.44 GHz center frequency as of now
sdr.tx_lo = SDR_Fc
sdr.rx_lo = SDR_Fc
# figure out what the following mean:
sdr.tx_cyclic_buffer = True
sdr.tx_hardwaregain = -30
sdr.gain_control_mode = "slow_attack"


#______________________________ Noise signal generation _________________________________
noise_signal_length = 10000
noise_mean = 0
noise_power_w = 1
noise_std_dev = np.sqrt(noise_power_w)

# Generate a random noise signal
noise = np.random.normal(noise_mean, noise_std_dev, noise_signal_length)

# Create BPSK modulated bitstream for the random signal
noise_bpsk = []
real_part = 0
for i in noise:
    if(i > 0):
        imag_part = 1
    else:
        imag_part = -1
    noise_bpsk.append(real_part)
    noise_bpsk.append(imag_part)

# Transmit the signal using SDR
rxdata = sdr.rx()
print(rxdata)
sdr.tx(noise_bpsk)
