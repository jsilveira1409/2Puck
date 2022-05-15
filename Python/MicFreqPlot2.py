
import matplotlib.pyplot as plt
from matplotlib.ticker import EngFormatter
import numpy as np
from matplotlib import rc
import time

rc('text', usetex = True)
rc("axes", labelsize = 30)
rc("xtick", labelsizxe = 30)
rc("ytick", labelsize = 30)
rc("axes", titlesize = 30)
rc("legend", fontsize = 30)

# I2S side

MaxSignalFreq =  np.arange(1000, 8000)                  #Max frequency in the incoming signal->700Hz for the guitar

BitsPerSample = 64                      #32 bit frame mode, 32 for left and 32 for right -> 64
DecimationFactor = 64                   #PDM to PCM decimation factor
NbChannels = 2
I2SSourceClock = 32E6

MicSamplingFreq = 2*MaxSignalFreq       #Given by Shannon
RequiredI2SClock = MicSamplingFreq * DecimationFactor * NbChannels  #half for each channel because bits are interleaved
I2SClockDivider = I2SSourceClock/(2*RequiredI2SClock)

BitSamplesRate = RequiredI2SClock/32   #Number of 32bit samples per sec


NbSamples = BitSamplesRate * 0.001  #number of samples in 1ms

# FFT side
FFT_SIZE = [1024, 2048, 4096]
InputBufferSize = 2*FFT_SIZE
OutputBufferSize = FFT_SIZE

# InputBuffer is filled with 10ms of data coming from PCM_buffer1/2 in mp45dt02.c
# we want to fill it with FFT_SIZE samples (even index have the data, odd have 0, the FFT function needs the array like this)
# Here we calculate both the necessary time to fill the InputBuffer that will be used for the FFT and 
# the resolution of the resulting FFT, which is in Hz/index
plt.figure(figsize=(9, 3))
plot_name = [r"$\textrm{FFT\_SIZE 1024}$",r"$\textrm{FFT\_SIZE 2048}$",r"$\textrm{FFT\_SIZE 4096}$"]


y = 0
name = 0


fig, plot = plt.subplots(figsize=(12,8))
fig1, plot1 = plt.subplots(figsize=(12, 8))

formatter0 = EngFormatter()

plot1.xaxis.set_major_formatter(formatter0)
for x in FFT_SIZE:
    TimeToFill = x /BitSamplesRate * 1000        #time in ms
    Resolution = (MicSamplingFreq/2)/(x/2)       #we divide the FFT by 2 because real and negative indexes are symmetrical
    plot.plot(TimeToFill,Resolution, c='blue', linewidth =5)
    plot1.plot(MicSamplingFreq/1000, Resolution, label = plot_name[name],linewidth =5)
    name +=1

fig.subplots_adjust(left=0.13,
                    bottom=0.2, 
                    right=0.95,
                    top=0.95)

fig1.subplots_adjust(left=0.13,
                    bottom=0.2, 
                    right=0.95,
                    top=0.95)

plot.set(ylabel=r"$\textrm{Résolution [Hz/index]}$",xlabel = r"$\textrm{Temps de remplissage [ms]}$")
plot.xaxis.set_major_formatter(formatter0)
plot1.set(ylabel=r"$\textrm{Résolution [Hz/index]}$",xlabel = r"$\textrm{Fréquence d'échantillonage [kHz]}$")



plot1.grid()
plot.grid()
plot1.legend()

fig.savefig('histogram.png')
fig1.savefig('histog2ram.png')
