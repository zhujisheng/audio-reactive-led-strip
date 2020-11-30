#include "esphome.h"
using namespace esphome;

#include "driver/i2s.h"

#define MAX_CLIENTS 10
const uint16_t LISTEN_PORT = 3344;

const uint16_t BUFFER_SIZE = 1024;
const uint16_t SAMPLE_RATE = 16000;

i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_PCM,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 32,
    .dma_buf_len = 32
};

i2s_pin_config_t pin_config = {
    .bck_io_num = -1,
    .ws_io_num = GPIO_NUM_19,
    .data_out_num = -1,
    .data_in_num = GPIO_NUM_22
};

class MicrophoneSwitch : public Component, public switch_::Switch {

private:
    bool _state;
    WiFiClient *_clients[MAX_CLIENTS] = { NULL };
    WiFiServer _server;

public:
    MicrophoneSwitch(){
        _state = true;
    }

    void setup() override {
        i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
        i2s_set_pin(I2S_NUM_0, &pin_config);
        i2s_stop(I2S_NUM_0);
        i2s_start(I2S_NUM_0);
        write_state(_state);
    }

    void write_state(bool state) override {
        _state = state;
        publish_state(state);
    }

    void loop() override {
        if (_state){

            int16_t l[BUFFER_SIZE];
            unsigned int read_num;
            i2s_read(I2S_NUM_0, l, BUFFER_SIZE*2, &read_num, portMAX_DELAY);
            for (int i = 0; i < BUFFER_SIZE; i++)
                l[i] = (l[i]-966)*64;

            if(!_server){
               _server.begin(LISTEN_PORT);
                ESP_LOGD("custom","Waiting for connecting ...");
            }
            WiFiClient newClient = _server.available();
            if (newClient) {
                for (int i=0 ; i<MAX_CLIENTS ; ++i) {
                    if (NULL==_clients[i]) {
                        _clients[i] = new WiFiClient(newClient);
                        ESP_LOGD("custom","Client No.%d, Connected from %s", i, _clients[i]->remoteIP().toString().c_str());
                        break;
                    }
                }
            }
            for (int i=0 ; i<MAX_CLIENTS ; ++i) {
                if(NULL != _clients[i]){
                    if(_clients[i]->connected()){
                        _clients[i]->write((uint8_t*)l,BUFFER_SIZE * 2);
                    }
                    else{
                        ESP_LOGD("custom","Disconnected from client No.%d", i);
                        _clients[i]->stop();
                        delete _clients[i];
                        _clients[i] = NULL;
                    }
                }
            }

        }
        else{
            for (int i=0 ; i<MAX_CLIENTS ; ++i) {
                if(NULL != _clients[i]){
                    ESP_LOGD("custom","Disconnected to client No.%d(%s)", i, _clients[i]->remoteIP().toString().c_str());
                    _clients[i]->stop();
                    delete _clients[i];
                    _clients[i] = NULL;
                }
            }
        }
        yield();
    }
};
