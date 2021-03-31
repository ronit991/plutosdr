import numpy as np
import adi

SDR_Fc = 1575200000     # GPS center frequency is 1575.2 MHz

sdr = adi.Pluto('ip:xx.xx.xx.xx')

noise_signal_length = 10000
noise_mean = 0
noise_power_w = 10
noise_std_dev = np.sqrt(noise_power_w)

noise = np.random.normal(noise_mean, noise_std_dev, noise_signal_length)
noise = ( noise/np.mean(abs(noise)) )

noise_bpsk = []
for i in noise:
    if(i > 0):
        bit = 1
    else:
        bit = -1
    noise_bpsk.append(bit)

print(noise_bpsk)
sdr.tx(noise_bpsk)