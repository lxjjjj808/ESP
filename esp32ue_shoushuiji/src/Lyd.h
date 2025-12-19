
#pragma once
#include <Arduino.h>
#include "canshu.h"
#include <PubSubClient.h>
#include <rom/rtc.h>
void count_function();

void liuliang_detect(unsigned long yongshi); 

void chaolv_chxi_panduan(int long1);

void yiwai_defaut();
void yucun_ximo();
void zhch_zhishui();
void zhishui_shuiman();
void yucun_ximo_wancheng();
void qidong_zengyabeng();
void WiFi_Connect();
void openValve(byte pin);
void closeValve(byte pin);
bool yuanshui_chxi(bool _true);
void switchToOta0AndReboot() ;
void printRunningPartition();
void printRunningPartitionInfo();
void display();
int countChineseCharacters(const char *str);
void splitChineseString(const char *input, char *part1, char *part2, int charCount);
int getUtf8CharLength(byte firstByte);
bool chaolv_chongxi(bool _true);
void pre_read();
void print_reset_reason();
const char *reset_reason_to_string(RESET_REASON reason);

void send_chlv_data();
void receive_chlv_data();
void send_wifi_data();
void send_wifi_list();
void receive_wifi_data();
void send_onenet_data();
void receive_onenet_data();
void send_product_data();
void receive_product_data();
void send_zs_data();
void receive_zs_data();
void send_ota_data();
void receive_ota_data();
void shtmo_chxi(); // 渗透膜冲洗

void callback(char *topic, byte *payload, unsigned int length);
void send_set_response(String message_id);
void OneNet_Connect();
void zs_upload();
void upload(String item,   int para);
void upload(String item,   String para);
void checkForUpdates();
void downloadAndUpdate(String firmwareUrl);

