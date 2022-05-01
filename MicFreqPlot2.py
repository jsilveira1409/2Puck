import matplotlib.pyplot as plt
from matplotlib.ticker import EngFormatter
import numpy as np

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
plot_name = ['FFT_SIZE 1024','FFT_SIZE 2048','FFT_SIZE 4096']

y = 0
name = 0

fig, plot = plt.subplots(2, 3, figsize =(8,8))

fig.suptitle("Frequency resolution as a function of the Time to fill the FFT buffer ")

for x in FFT_SIZE:
    TimeToFill = x /BitSamplesRate * 1000        #time in ms
    Resolution = (MicSamplingFreq/2)/(x/2)       #we divide the FFT by 2 because real and negative indexes are symmetrical
    plot[0, y].plot(TimeToFill,Resolution)
    plot[0, y].set_title(plot_name[name])
    
    plot[1, y].plot(MicSamplingFreq, Resolution)
    
    y +=1
    name +=1

plt.subplots_adjust(left=0.1,
                    bottom=0.1, 
                    right=0.9,
                    top=0.9, 
                    wspace=0.6, 
                    hspace=0.4)

formatter0 = EngFormatter()

plot[0,0].set(ylabel='Resolution [Hz/index]',xlabel = 'Time to fill the buffer [ms]')
plot[0,0].xaxis.set_major_formatter(formatter0)
# plot[0,0].yaxis.set_major_formatter(formatter0)
plot[0,1].set(xlabel='Time to fill the buffer [ms]')
plot[0,0].xaxis.set_major_formatter(formatter0)
# plot[0,0].yaxis.set_major_formatter(formatter0)
plot[0,2].set(xlabel='Time to fill the buffer [ms]')
plot[0,0].xaxis.set_major_formatter(formatter0)
# plot[0,0].yaxis.set_major_formatter(formatter0)

plot[1,1].set_title("Frequency resolution as a function of the sampling frequency")
plot[1,0].set(ylabel='Resolution [Hz/index]',xlabel = 'Sampling Frequency [Hz]')
plot[1,0].xaxis.set_major_formatter(formatter0)
# plot[1,0].yaxis.set_major_formatter(formatter0)
plot[1,1].set(xlabel = 'Sampling Frequency [Hz]')
plot[1,1].xaxis.set_major_formatter(formatter0)
# plot[1,1].yaxis.set_major_formatter(formatter0)
plot[1,2].set(xlabel = 'Sampling Frequency [Hz]')
plot[1,2].xaxis.set_major_formatter(formatter0)
# plot[1,2].yaxis.set_major_formatter(formatter0)



for pl in plot:
    for p in pl:
        p.grid()


plt.savefig("FFTgraph.png")





