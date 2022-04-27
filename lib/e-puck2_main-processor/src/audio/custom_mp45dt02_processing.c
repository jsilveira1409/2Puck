/*******************************************************************************
* Copyright (c) 2017, Alan Barr
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <string.h>
#include "ch.h"
#include "hal.h"
#include "spi3_slave.h"
#include "custom_mp45dt02_processing.h"
#include "pdm_filter.h"

/******************************************************************************/
/* Hardware configuration */
/******************************************************************************/

/* ChibiOS I2S driver in use */
#define MP45DT02_I2S_DRIVER                 I2SD2

/* STM32F4 configuration for the I2S configuration register */
#define I2SCFG_MODE_MASTER_RECEIVE          (SPI_I2SCFGR_I2SCFG_0 | SPI_I2SCFGR_I2SCFG_1)
#define I2SCFG_STD_I2S                      (0)
#define I2SCFG_STD_LSB_JUSTIFIED            (SPI_I2SCFGR_I2SSTD_1)
#define I2SCFG_STD_MSB_JUSTIFIED            (SPI_I2SCFGR_I2SSTD_0)
#define I2SCFG_STD_PCM                      (SPI_I2SCFGR_I2SSTD_0 | SPI_I2SCFGR_I2SSTD_1)
#define I2SCFG_CKPOL_STEADY_LOW             (0)
#define I2SCFG_CKPOL_STEADY_HIGH            (SPI_I2SCFGR_CKPOL)
#define I2SCFG_DATAFORMAT_32B				(SPI_I2SCFGR_DATLEN_1)
#define I2SCFG_MODE_SLAVE_RECEIVE			(SPI_I2SCFGR_I2SCFG_0)

/* STM32F4 configuration for I2S prescalar register */
// In our case we want an audio sampling frequency Fs = 16 KHz.
// We use 32 bits frame mode, it means 32 bits for left channel and 32 bits for right channel, thus 64 bits at each sampling event.
// For the conversion from PDM to PCM we use a decimation factor = 64.
//
// Required I2S clock = Fs x decimation factor x number of channels
// Required I2S clock = 16 KHz x 64 x 2 = 2.048 MHz
//
// This means that the I2S will read data at 2 MHz, 1 MHz for each channel (interleaved).
// The input clock for the microphones of 1 MHz is generated by a timer that is placed between the I2S and the microphones and simply halves the input
// clock of 2 MHz coming from the I2S.
//
// I2S source clock is PLLI2SCLK = 32 MHz (see mcuconf.h):
// PLLI2SCLK = HSE/PLLM x PLLN/PLLR = 24/24 x 192/6 = 32 MHz.
//
// I2S divider is composed by I2SDIV (linear prescaler) and ODD (odd factor), divider = (I2SDIV x 2) + ODD.
// I2S clock = PLLI2SCLK / divider => divider = PLLI2SCLK / I2S clock
// => ODD =  (PLLI2SCLK / I2S clock) & 1 = (32 MHz / 2 MHz) & 1 = 0
// => I2SDIV = ((PLLI2SCLK / I2S clock) - ODD) / 2 = 8
//
// The number of samples required to get 1 ms of data is:
// 2.048 Mbs / 32 bits = 64000 32bit_samples/sec, 32000 32bit_samples/sec for left channel and 32000 32bit_samples/sec for right channel.
// 0.001 x 64000 = 64 32bit_samples in 1 ms, 32 32bit_samples (128 bytes) for left and 32 32bit_samples (128 bytes) for right.
// This value is needed to set the number of samples to acquire for DMA, since the PDM to PCM library works with 1 ms of data.
#define MP45DT02_I2SDIV                     8
#define CUSTOM_MP45DT02_I2SDIV              8
#define MP45DT02_I2SODD                     0
#define I2SPR_I2SODD_SHIFT                  8

/* Debugging - check for buffer overflows */
#define MEMORY_GUARD                        0xDEADBEEF

// Decode matrix: from 8 bits to 4 bits.
uint8_t Channel_Demux[128] = {
  0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
  0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
  0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
  0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
  0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
  0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03,
  0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
  0x04, 0x05, 0x04, 0x05, 0x06, 0x07, 0x06, 0x07,
  0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
  0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f,
  0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f,
  0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
  0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b,
  0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f,
  0x0c, 0x0d, 0x0c, 0x0d, 0x0e, 0x0f, 0x0e, 0x0f
};

static struct {
    uint32_t offset;
    uint32_t number;
    uint16_t buffer[MP45DT02_BUFFER_SIZE_2B];
    uint32_t guard;
} mp45dt02I2sData;

//CHECK -> THIS I2S pdm buffer should be the whole buffer size, not half as we do not share it with SPI anymore
//static uint16_t I2S_PDM_samples[MP45DT02_BUFFER_SIZE_2B] = {0};
static uint16_t I2S_PDM_samples[MP45DT02_BUFFER_SIZE_2B/2] = {0};
static uint16_t I2S_PCM_samples[MP45DT02_DECIMATED_BUFFER_SIZE] = {0};
static int16_t PCM_buffer1[MIC_BUFFER_LEN] = {0};
static int16_t PCM_buffer2[MIC_BUFFER_LEN] = {0};
static uint8_t PCM_buffer_index = 0;
static int16_t *PCM_buffer_ptr_curr = PCM_buffer1;
static int16_t *PCM_buffer_ptr_last = PCM_buffer2;

static thread_t *DataProcessingThd;
static THD_WORKING_AREA(DataProcessingThdWA, 1024);
static semaphore_t DataProcessingSem;

static I2SConfig mp45dt02I2SConfig;

static mp45dt02Config initConfig;

PDMFilter_InitStruct PDM_filter_I2S[MP45DT02_NUM_CHANNELS];

static void PDMDecoder_Init(void) {
	uint8_t i = 0;

	// Enable CRC peripheral to unlock the PDM library.
	rccEnableAHB1(RCC_AHB1ENR_CRCEN, FALSE);

	for(i=0; i<MP45DT02_NUM_CHANNELS; i++) {
		// Filter LP and HP Init.
		PDM_filter_I2S[i].LP_HZ = CUSTOM_I2S_AUDIOFREQ/2;					// guitar frequency for the frets between 0 and 12 are
		PDM_filter_I2S[i].HP_HZ = 10;					// in between 180 and 700 hz
		PDM_filter_I2S[i].Fs = CUSTOM_I2S_AUDIOFREQ;
		PDM_filter_I2S[i].Out_MicChannels = 2;
		PDM_filter_I2S[i].In_MicChannels = 2;
		PDM_Filter_Init((PDMFilter_InitStruct *)&PDM_filter_I2S[i]);

	}

}

static THD_FUNCTION(DataProcessing, arg)
{
    (void)arg;

    chRegSetThreadName(__FUNCTION__);

	uint32_t index = 0;
	uint16_t * DataTempI2S;
	uint8_t a,b=0;

    while (chThdShouldTerminateX() == false)
    {
    	//Wait for the I2S buffer to fill up

        chSemWait(&DataProcessingSem);
        if (chThdShouldTerminateX() == true)
        {
            break;
        }

        // Point to the last filled data, the other half of the buffer is being filled by DMA.
        // The DMA works in 32 bits mode, thus double the buffer index to get the correct offset.
    	DataTempI2S = &mp45dt02I2sData.buffer[mp45dt02I2sData.offset*2];

        // We have MP45DT02_SAMPLE_SIZE_4B = 64 samples of 32 bits each (=> mp45dt02I2sData.number = 64).
        // Each sample is 32 bits, thus  64 * 32 bits = 2048 PDM samples.
        // The samples are interleaved left and right, this means that the first bit is left, the second is right, ...
        // Extract the bits sequence and transform it in order to have 1 byte left, 1 byte right, ...
        // This is needed by the library functions that convert PDM in PCM samples.
        for(index=0; index < mp45dt02I2sData.number*2; index++) { // We take in consideration 16 bits at a time, thus we loop twice as the number of samples.
			a = ((uint8_t *)(DataTempI2S))[(index*2)]; // MSByte.
			b = ((uint8_t *)(DataTempI2S))[(index*2)+1]; // LSByte.
			((uint8_t *)I2S_PDM_samples)[(index*2)] = Channel_Demux[a & CHANNEL_DEMUX_MASK] | Channel_Demux[b & CHANNEL_DEMUX_MASK] << 4; // Extract left and swap bytes.
			((uint8_t *)I2S_PDM_samples)[(index*2)+1] = Channel_Demux[(a>>1) & CHANNEL_DEMUX_MASK] |Channel_Demux[(b>>1) & CHANNEL_DEMUX_MASK] << 4; // Extract right and swap bytes.
		}

        // Using a decimator factor of 64, we get 2048/64 = 32 PCM samples of 16 bits, 16 PCM for left channel and 16 PCM for right channel.
        for(index = 0; index < MP45DT02_NUM_CHANNELS; index++) {
        	/* PDM to PCM filter */
        	PDM_Filter_64_LSB(&((uint8_t*)(I2S_PDM_samples))[index], (uint16_t*)&(I2S_PCM_samples[index]), AUDIO_IN_VOLUME , (PDMFilter_InitStruct *)&PDM_filter_I2S[index]);
        }

        // Arrange the buffer in order to have the microphones data in sequence: mic0, mic1, mic2, mic3, mic0, mic1, mic2, mic3, ...
        //JOJO	for(index=0; index<MP45DT02_DECIMATED_BUFFER_SIZE/2; index++) {
        for(index=0; index<MP45DT02_DECIMATED_BUFFER_SIZE/2; index++) {
        	//PCM_buffer_ptr_curr[(PCM_buffer_index*2*MP45DT02_DECIMATED_BUFFER_SIZE)+(index)] = I2S_PCM_samples[index]; // Right microphone is MIC0
        	//PCM_buffer_ptr_curr[(PCM_buffer_index*2*MP45DT02_DECIMATED_BUFFER_SIZE)+(index)+1] = I2S_PCM_samples[index+1]; // Left microphone is MIC1

			PCM_buffer_ptr_curr[(PCM_buffer_index*2*MP45DT02_DECIMATED_BUFFER_SIZE)+(index*4)] = I2S_PCM_samples[index*2]; // Right microphone is MIC0
			PCM_buffer_ptr_curr[(PCM_buffer_index*2*MP45DT02_DECIMATED_BUFFER_SIZE)+(index*4)+1] = I2S_PCM_samples[index*2+1]; // Left microphone is MIC1
        }

        if(PCM_buffer_index == 9) {
        	PCM_buffer_index = 0;

        	initConfig.fullbufferCb((int16_t *)PCM_buffer_ptr_curr, MIC_BUFFER_LEN);

        	PCM_buffer_ptr_last = PCM_buffer_ptr_curr;
        	if(PCM_buffer_ptr_curr == PCM_buffer1) {
        		PCM_buffer_ptr_curr = PCM_buffer2;
        	} else {
        		PCM_buffer_ptr_curr = PCM_buffer1;
        	}
        } else {
        	PCM_buffer_index++;
        }
    }
}

/* (*i2scallback_t) */
static void mp45dt02I2SCb(I2SDriver *i2sp, size_t offset, size_t number)
{
    (void)i2sp;
    chSysLockFromISR();
    mp45dt02I2sData.offset = offset;
    mp45dt02I2sData.number = number;
    chSemSignalI(&DataProcessingSem);
    chSysUnlockFromISR();
}



void mp45dt02Init(mp45dt02Config *config)
{

    initConfig = *config;

    chSemObjectInit(&DataProcessingSem, 0);

    DataProcessingThd = chThdCreateStatic(DataProcessingThdWA,
    									sizeof(DataProcessingThdWA),
										NORMALPRIO+1,
										DataProcessing, NULL);

    PDMDecoder_Init();

    //*******************
    // I2S2 configuration
    //*******************
    memset(&mp45dt02I2sData, 0, sizeof(mp45dt02I2sData));
    mp45dt02I2sData.guard = MEMORY_GUARD;

    memset(&mp45dt02I2SConfig, 0, sizeof(mp45dt02I2SConfig));
    mp45dt02I2SConfig.tx_buffer = NULL;
    mp45dt02I2SConfig.rx_buffer = mp45dt02I2sData.buffer;
    mp45dt02I2SConfig.size      = MP45DT02_SAMPLE_SIZE_4B*2; // 2 ms of data
    mp45dt02I2SConfig.end_cb    = mp45dt02I2SCb; // Callback function called at half-fill (1 ms of data) and full-fill (1 ms of data).

    mp45dt02I2SConfig.i2scfgr   = I2SCFG_MODE_MASTER_RECEIVE    |
                                  I2SCFG_STD_LSB_JUSTIFIED      |
                                  I2SCFG_CKPOL_STEADY_HIGH		|
								  I2SCFG_DATAFORMAT_32B;

    mp45dt02I2SConfig.i2spr     = (SPI_I2SPR_I2SDIV & CUSTOM_MP45DT02_I2SDIV) |
                                  (SPI_I2SPR_ODD & (MP45DT02_I2SODD << I2SPR_I2SODD_SHIFT));

    i2sStart(&MP45DT02_I2S_DRIVER, &mp45dt02I2SConfig);
    i2sStartExchange(&MP45DT02_I2S_DRIVER);
}

void mp45dt02Shutdown(void)
{
    i2sStopExchange(&MP45DT02_I2S_DRIVER);
    i2sStop(&MP45DT02_I2S_DRIVER);

    spi3SlaveStopExchange(&SPISLAVED3);
    spi3SlaveStop(&SPISLAVED3);

    chThdTerminate(DataProcessingThd);
    chSemReset(&DataProcessingSem, 0);
    chThdWait(DataProcessingThd);
    DataProcessingThd = NULL;
}

int16_t* mp45dt02BufferPtr(void) {
	return PCM_buffer_ptr_last;
}



