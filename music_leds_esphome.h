#include "esphome.h"
using namespace esphome;

#include "driver/i2s.h"
#include <FastLED.h>
#include "FFT.h"
#include "VisualEffect.h"

enum PLAYMODE {MODE_SCROLL, MODE_ENERGY, MODE_SPECTRUM};

class MusicLeds{
    private:
        //N_PIXELS: The number of the LEDS on the led strip, must be even.
        static const uint16_t N_PIXELS = 60;
        //MIN_VOLUME_THRESHOLD: If the audio's volume is less than this number, the signal will not be processed.
        static constexpr float MIN_VOLUME_THRESHOLD = 0.0003;
        //Microphone(type of PDM)'s WS Pin and DATA_IN Pin, connecting to GPIO
        static const int PDM_WS_IO_PIN = 19;
        static const int PDM_DATA_IN_PIN = 22;

        static const uint16_t BUFFER_SIZE = 512; 
        static const uint8_t N_ROLLING_HISTORY = 2;
        static const uint16_t SAMPLE_RATE = 16000;
        static const uint16_t N_MEL_BIN = 18;
        static constexpr float MIN_FREQUENCY = 200;
        static constexpr float MAX_FREQUENCY = 8000;

        float y_data[BUFFER_SIZE * N_ROLLING_HISTORY];
        class FFT *fft;
        class VisualEffect * effect;

        CRGB physic_leds[N_PIXELS];

        i2s_port_t i2s_num = I2S_NUM_0; // i2s port number
        i2s_config_t i2s_config = {
          .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
          .sample_rate = SAMPLE_RATE,
          .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
          .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
          .communication_format = I2S_COMM_FORMAT_PCM,

          .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
          .dma_buf_count = 32,
          .dma_buf_len = 32 
        };

        i2s_pin_config_t pin_config = {
          //.bck_io_num = 18,
          .bck_io_num = -1,
          .ws_io_num = PDM_WS_IO_PIN,
          .data_out_num = -1,
          .data_in_num = PDM_DATA_IN_PIN
        };

        PLAYMODE CurrentMode = MODE_SCROLL;

    public:
        MusicLeds();
        ~MusicLeds();
        void ShowFrame( PLAYMODE CurrentMode, light::AddressableLight *p_it);
};

MusicLeds::MusicLeds(){
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_stop(I2S_NUM_0);
    i2s_start(I2S_NUM_0);
    
    fft = new FFT(BUFFER_SIZE*N_ROLLING_HISTORY, N_MEL_BIN, MIN_FREQUENCY, MAX_FREQUENCY, SAMPLE_RATE, MIN_VOLUME_THRESHOLD);
    effect = new VisualEffect(N_MEL_BIN,N_PIXELS);
    
}

MusicLeds::~MusicLeds(){
    i2s_stop(I2S_NUM_0);
    delete fft;
    delete effect;
}


void MusicLeds::ShowFrame( PLAYMODE CurrentMode, light::AddressableLight *p_it){
            static float mel_data[N_MEL_BIN];

            for (int i = 0; i < N_ROLLING_HISTORY - 1; i++)
                memcpy(y_data + i * BUFFER_SIZE, y_data + (i + 1)*BUFFER_SIZE, sizeof(float)*BUFFER_SIZE);

            int16_t l[BUFFER_SIZE];

            unsigned int read_num;
                i2s_read(I2S_NUM_0, l, BUFFER_SIZE * 2, &read_num, portMAX_DELAY);
          
            for(int i = 0; i < BUFFER_SIZE; i++) {
                y_data[BUFFER_SIZE * (N_ROLLING_HISTORY - 1) + i] = l[i] / 32768.0;
                static int ii=0;
                ii++;
                if(ii%SAMPLE_RATE==0)
                  ESP_LOGD("custom","%lu\t%d\n",millis(),ii);
            }
            fft->t2mel( y_data, mel_data );

            switch(CurrentMode){
              case MODE_SCROLL:
                effect->visualize_scroll(mel_data, physic_leds);
                break;
              case MODE_ENERGY:
                effect->visualize_energy(mel_data, physic_leds);
                break;
              case MODE_SPECTRUM:
                effect->visualize_spectrum(mel_data, physic_leds);
                break;
            }

            for (int i = 0; i < p_it->size(); i++) {
                light::ESPColor c;
                c.r = physic_leds[i].r;
                c.g = physic_leds[i].g;
                c.b = physic_leds[i].b;
                c.w = 0;
                (*p_it)[i] = c;
            }
}

class MusicLeds music_leds;
