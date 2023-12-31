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
#include <stdio.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>


using namespace std;

//#define M_MQTT_HOST_ADDR  "localhost"
#define M_MQTT_HOST_ADDR "broker.hivemq.com"
#define M_MQTT_HOST_USER  "admin"
#define M_MQTT_HOST_PASS  "admin13589"
#define M_MQTT_PORT        1883
#define M_HOST_PORT       "/dev/ttyUSB0"
#define M_PUB_TOPIC       "hustBMS/group3/elevator/status"
#define M_SUB_TOPIC_CTL   "hustBMS/group3/elevator/cmd"
#define M_SUB_TOPIC_FIRE  "hustBMS/group3/fire/cmd"


#define LINUX_CMD_CURRENT_LEVEL 'C'

#define LINUX_CMD_CHANGE_LEVEL  'L'

#define LINUX_CMD_FIRE_LEVEL    'F'

float current_level;

int32_t g_fd;
struct mosquitto *m_mosq;
char * now_time;
uint8_t g_buff[100] = "TEST HOST";

int32_t my_string_cmp(char* str1, char* str2, uint32_t len){
    for(int i = 0; i < len; i++){
        if(str1[i] != str2[i]){
            return 1;
        }
    }
    return 0;
}

void on_connect(struct mosquitto *_mosq, void *_obj, int _reason_code)
{
    printf("Mqtt on_connect: %s\n", mosquitto_connack_string(_reason_code));
    if(_reason_code != 0){
        printf("Mqtt connect to sever fail. \n");
        mosquitto_disconnect(_mosq);
    }

    int mid = 1;

    int rc = mosquitto_subscribe(_mosq, NULL, M_SUB_TOPIC_FIRE, 1);
    if(rc != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(_mosq);
    }

    mid = 2;

    rc = mosquitto_subscribe(_mosq, NULL, M_SUB_TOPIC_CTL, 1);
    if(rc != MOSQ_ERR_SUCCESS){
        fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(_mosq);
    }

}

void on_publish(struct mosquitto *_mosq, void *_obj, int _mid)
{
//    printf("Message mqtt with mid %d has been published.\n", _mid);
}

void rev_data_poll() {
    uint8_t packet[1024] = {0,};
    while (1) {
        int ret = serial_recv_bytes(g_fd,packet,1024);
        if(ret > 0){
            if(packet[1] == LINUX_CMD_CURRENT_LEVEL){
                char payload[50] ;
                current_level = (float)packet[2]/10;
                sprintf(payload,"Current level: %1.1f\n",current_level);
                int8_t rc = mosquitto_publish(m_mosq, NULL, M_PUB_TOPIC, strlen(payload), payload, 2, false);
                if(rc != MOSQ_ERR_SUCCESS){
                    printf("Error publishing.\n");
                }
            }
        }
    }
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
    int i;
    bool have_subscription = false;

    for(i=0; i < qos_count; i++){
        printf("on_subscribe: %d :granted qos = %d\n", i, granted_qos[i]);
        if(granted_qos[i] <= 2){
            have_subscription = true;
        }
    }

}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    uint8_t cmd = *((uint8_t*)msg->payload) - '0';
    if(!my_string_cmp((char *)msg->topic,(char*)M_SUB_TOPIC_FIRE, strlen(M_SUB_TOPIC_FIRE)-1)){
        printf("fire!!!!\n");
        uint8_t buff_tx[32] ={0,};
        buff_tx[0] = '*';
        buff_tx[1] = LINUX_CMD_FIRE_LEVEL;
        buff_tx[2] = cmd;
        buff_tx[3] = ';';
        serial_send_bytes(g_fd,buff_tx,4);
    }

    if(!my_string_cmp((char *)msg->topic,(char*)M_SUB_TOPIC_CTL, strlen(M_SUB_TOPIC_CTL)-1)){
        printf("change elevator level to level %d!!!!\n",cmd);
        uint8_t buff_tx[32] ={0,};
        buff_tx[0] = '*';
        buff_tx[1] = LINUX_CMD_CHANGE_LEVEL;
        buff_tx[2] = cmd;
        buff_tx[3] = ';';
        serial_send_bytes(g_fd,buff_tx,4);
    }

}

int main() {

    int rc;

    if (system("echo 0528 | sudo -S chmod 777 /dev/ttyUSB0") == -1) {
        perror("error sudo\n");
    }

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
    mosquitto_subscribe_callback_set(m_mosq, on_subscribe);
    mosquitto_message_callback_set(m_mosq, on_message);

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

    std::thread Receive_data_thread(rev_data_poll);

    sleep(2);

    while(1){
        printf("\n/*************** Elevator Control *****************/\n");
        printf(" Please choose the command to control elevator:\n ");
        printf("Choose 1 to control level\n");
        printf(" Choose 2 to get current level\n");
        printf(" Choose 3 to control fire emergency\n");
        int c = getchar();
        while (getc(stdin) != '\n');
        if(c == '1'){
            printf("Please enter the level (1 to 4)!\n ");
            c = getchar();
            while (getc(stdin) != '\n');
            if((c - '0' <= 0) || (c - '0' > 4)){
                printf("Please choose the right elevator level!!!\n\n ");
                continue;
            }
            printf("Change elevator level to level %d!!!!\n",c - '0');
            uint8_t buff_tx[32] ={0,};
            buff_tx[0] = '*';
            buff_tx[1] = LINUX_CMD_CHANGE_LEVEL;
            buff_tx[2] = c - '0';
            buff_tx[3] = ';';
            serial_send_bytes(g_fd,buff_tx,4);
        }else if(c == '2'){
            printf("Current level is: %1.1f\n",current_level);

        }else if(c == '3'){
            printf("Please enter the fire status (1 is fire, 0 is not fire)!\n ");
            c = getchar();
            while (getc(stdin) != '\n');

            if((c == '1') || (c == '0')){
                printf("Change fire lever to %d", c - '0');
                uint8_t buff_tx[32] ={0,};
                buff_tx[0] = '*';
                buff_tx[1] = LINUX_CMD_FIRE_LEVEL;
                buff_tx[2] = c - '0';
                buff_tx[3] = ';';
                serial_send_bytes(g_fd,buff_tx,4);
            }else{
                printf("Please choose the right fire level!!!\n\n ");
            }

        }
        else{
            printf("Please choose the right command!!!\n\n ");
        }



    }

    return 0;
}
