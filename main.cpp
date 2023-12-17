#include <stdio.h>
#include <mosquitto.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include "host/sm_host.h"
#include "utils/linux/linux_serial.h"
#include "fcntl.h"
#include <thread>
#include <chrono>


using namespace std;

#define M_MQTT_HOST_ADDR  "localhost"
//#define M_MQTT_HOST_ADDR "broker.hivemq.com"
#define M_MQTT_HOST_USER  "admin"
#define M_MQTT_HOST_PASS  "admin13589"
#define M_MQTT_PORT        1883

#define M_HOST_PORT       "/dev/ttyUSB0"
int32_t g_fd;
struct mosquitto *m_mosq;
char * now_time;
uint8_t g_buff[100] = "TEST HOST";

void on_connect(struct mosquitto *_mosq, void *_obj, int _reason_code)
{
    printf("Mqtt on_connect: %s\n", mosquitto_connack_string(_reason_code));
    if(_reason_code != 0){
        printf("Mqtt connect to sever fail. \n");
        mosquitto_disconnect(_mosq);
    }
}

void on_publish(struct mosquitto *_mosq, void *_obj, int _mid)
{
//    printf("Message mqtt with mid %d has been published.\n", _mid);
}

void rev_data_poll() {
    uint8_t packet[1024] = {0,};
    while (true) {
        int ret = serial_recv_bytes(g_fd,packet,1024);
        if(ret > 0){
//            printf("Receive data with len: %d \ndata is %s\n", ret,packet);
            if(packet[1] == 'C'){
                char payload[50] ;
                float current_level = (float)packet[2]/10;

                printf("Send mqtt: Current level: %1.1f\n",current_level);
                sprintf(payload,"Current level: %1.1f\n",current_level);
                int8_t rc = mosquitto_publish(m_mosq, NULL, "group3/elevator", strlen(payload), payload, 2, false);
                if(rc != MOSQ_ERR_SUCCESS){
                    printf("Error publishing.\n");
                }
            }
        }
    }
}

int main() {

    int rc;

    g_fd = serial_init(M_HOST_PORT,38400,false);

    if(g_fd < 0){
        printf("Could NOT open serial device file: %s\n", M_HOST_PORT);
        return -1;
    }else{
        printf("Just open success serial device file: %s with fd = %d\n", M_HOST_PORT,g_fd);
    }

    mosquitto_lib_init();
    m_mosq = mosquitto_new("vuong.lk", true, NULL);
    mosquitto_username_pw_set(m_mosq,M_MQTT_HOST_USER,M_MQTT_HOST_PASS);
    if(m_mosq == NULL){
        printf("Error: Cant create mosq module, out of memory.\n");
        return 1;
    }
    mosquitto_connect_callback_set(m_mosq, on_connect);
    mosquitto_publish_callback_set(m_mosq, on_publish);
    rc = mosquitto_connect(m_mosq, M_MQTT_HOST_ADDR, M_MQTT_PORT, 600);
    if(rc != MOSQ_ERR_SUCCESS){
        mosquitto_destroy(m_mosq);
        printf("Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }
    printf("Connect success to broker!!!\n");
    rc = mosquitto_loop_start(m_mosq);
    if(rc != MOSQ_ERR_SUCCESS){
        mosquitto_destroy(m_mosq);
        printf("Start mqtt loop fail\n");
        return 1;
    }

    char payload[50] ;


    std::thread Receive_data_thread(rev_data_poll);

    while(1){

//        for(int i = 0; i < 3; i++){
//            sprintf(payload,"Current level: %d",1);
//            rc = mosquitto_publish(m_mosq, NULL, "group3/elevator", strlen(payload), payload, 2, false);
//            if(rc != MOSQ_ERR_SUCCESS){
//                printf("Error publishing.\n");
//            }
//            sleep(1);
//        }
//
//        for(int i = 0; i < 3; i++){
//            sprintf(payload,"Current level: %d",2);
//            rc = mosquitto_publish(m_mosq, NULL, "group3/elevator", strlen(payload), payload, 2, false);
//            if(rc != MOSQ_ERR_SUCCESS){
//                printf("Error publishing.\n");
//            }
//            sleep(1);
//        }
//
//        for(int i = 0; i < 3; i++){
//            sprintf(payload,"Current level: %d",3);
//            rc = mosquitto_publish(m_mosq, NULL, "group3/elevator", strlen(payload), payload, 2, false);
//            if(rc != MOSQ_ERR_SUCCESS){
//                printf("Error publishing.\n");
//            }
//            sleep(1);
//        }
//
//        for(int i = 0; i < 3; i++){
//            sprintf(payload,"Current level: %d",4);
//
//
//
//            rc = mosquitto_publish(m_mosq, NULL, "group3/elevator", strlen(payload), payload, 2, false);
//            if(rc != MOSQ_ERR_SUCCESS){
//                printf("Error publishing.\n");
//            }
//            sleep(1);
//        }

    }

    return 0;
}
