#include <Arduino.h>
#include "canshu.h"
#include <EEPROM.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "Lyd.h"
#include <Preferences.h>
#include "canshu.h"
#include <PubSubClient.h>
#include <rom/rtc.h>
#include <HTTPClient.h>
#include <Update.h>
#include "esp_ota_ops.h"
#include <esp_partition.h>
#include <nvs_flash.h>
#include <base64.h>
void switchToOta0AndReboot()
{
  const esp_partition_t *ota0 = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
  if (ota0)
  {
    esp_ota_set_boot_partition(ota0);
    Serial.println("已设置下次启动到 ota_0，正在重启...");
    delay(1000);
    ESP.restart();
  }
  else
  {
    Serial.println("未找到 ota_0 分区！");
  }
}

void print_reset_reason()
{
  Serial.println("\n===== 详细重启原因 =====");

  // 获取 CPU0 �??? CPU1 的重�???原因（双�??? ESP32�???
  RESET_REASON reason0 = rtc_get_reset_reason(0);
  RESET_REASON reason1 = rtc_get_reset_reason(1);

  Serial.print("CPU0 重启原因: ");
  Serial.println(reset_reason_to_string(reason0));

  Serial.print("CPU1 重启原因: ");
  Serial.println(reset_reason_to_string(reason1));
  upload("reset_reason", reset_reason_to_string(reason0)); // 上传 CPU0 重启原因
  upload("reset_reason", reset_reason_to_string(reason1)); // 上传 CPU1 重

  // Serial.println("=======================");
}
const char *reset_reason_to_string(RESET_REASON reason)
{
  switch (reason)
  {
  case POWERON_RESET:
    return "上电复位 (POWERON_RESET)";
  case SW_RESET:
    return "软件复位 (SW_RESET)";
  case OWDT_RESET:
    return "软件看门狗复位 (OWDT_RESET)";
  case DEEPSLEEP_RESET:
    return "深度睡眠唤醒复位 (DEEPSLEEP_RESET)";
  case SDIO_RESET:
    return "SDIO 复位 (SDIO_RESET)";
  case TG0WDT_SYS_RESET:
    return "Timer Group0 看门狗复位 (TG0WDT_SYS_RESET)";
  case TG1WDT_SYS_RESET:
    return "Timer Group1 看门狗复位 (TG1WDT_SYS_RESET)";
  case RTCWDT_SYS_RESET:
    return "RTC 看门狗复位 (RTCWDT_SYS_RESET)";
  case INTRUSION_RESET:
    return "非法访问复位 (INTRUSION_RESET)";
  case TGWDT_CPU_RESET:
    return "Timer Group CPU 看门狗复位 (TGWDT_CPU_RESET)";
  case SW_CPU_RESET:
    return "软件CPU 复位 (SW_CPU_RESET)";
  case RTCWDT_CPU_RESET:
    return "RTC 看门狗 CPU 复位 (RTCWDT_CPU_RESET)";
  case EXT_CPU_RESET:
    return "外部 CPU 复位 (EXT_CPU_RESET)";
  case RTCWDT_BROWN_OUT_RESET:
    return "低电压复位 (RTCWDT_BROWN_OUT_RESET)";
  case RTCWDT_RTC_RESET:
    return "RTC 看门狗RTC 复位 (RTCWDT_RTC_RESET)";
  default:
    return "未知复位原因";
  }
}
void count_function()
{
  if (digitalRead(feishui_chxi_dcf) == 1) // 废水冲洗打开
  {
    zs_cx_pulse++; // 制水状态下强冲洗脉冲数
  }
  else if (digitalRead(chlv_chxi_dcf) == 1) // 超滤冲洗打开
  {
    chlv_pulse++; // 超滤冲洗脉冲数
  }
  else
  {
    zs_pulse++; // 正常制水脉冲数
  }
}

void liuliang_detect(unsigned long yongshi) /*???????,??��????*/
{
  zs_pulse = 0;    /*????????????,???????*/
  zs_cx_pulse = 0; /*????????????,???????*/
  chlv_pulse = 0;  /*超滤冲洗脉冲数归0*/
  // unsigned long begintime2 = millis();

  attachInterrupt(shuiliu_chuanganqi, count_function, RISING);
  /*do
  {
    endtime2 = millis();
  } while (endtime2 - begintime2 < yongshi);*/
  delay(yongshi); // ��ʱ���ȴ�ˮ���������������
  detachInterrupt(shuiliu_chuanganqi);
}

/*-------------------------ֵ防止数据过大过小----------------------------*/
void yiwai_defaut()
{

  if (dc_zs_shch_bz > 60 || dc_zs_shch_bz < 20) // ��ˮ����ʱ���趨 ��λ������
  {
    dc_zs_shch_bz = 40;
  }
  if (zs_pulse_bzh > 40 || zs_pulse_bzh < 5) // ��ˮ�����׼
  {
    zs_pulse_bzh = 15;
  }

  if (chlv_chxi_cishu > 5 || chlv_chxi_cishu < 2) // ����ÿ�س�ϴ����
  {
    chlv_chxi_cishu = 2;
  }
}

// �򿪵�ŷ���������ģʽ��

// =================== 电磁阀控制 ===================
void openValve(byte pin)
{
  int channel = (pin == zong_dcf) ? CHANNEL0 : (pin == jinshui_dcf) ? CHANNEL1
                                           : (pin == js_dcf)        ? CHANNEL2
                                                                    : -1;
  if (channel >= 0)
  {
    ledcWrite(channel, START_POWER);
    delay(START_DURATION);
    ledcWrite(channel, HOLD_POWER);
  }
}

void closeValve(byte pin)
{
  int channel = (pin == zong_dcf) ? CHANNEL0 : (pin == jinshui_dcf) ? CHANNEL1
                                           : (pin == js_dcf)        ? CHANNEL2
                                                                    : -1;
  if (channel >= 0)
    ledcWrite(channel, 0);
}

void chaolv_chxi_panduan(int long1) // ��λ����
{
  // yi_zhishui_shch = yi_zhishui_shch + long1;
  // EEPROM.writeLong(3, yi_zhishui_shch);

  accumulative = accumulative + long1;
  pre.putInt("accumulative", accumulative);
  if (accumulative > chxi_leiji_bzh) // ���ӻ��ɺ���
  {
    chlv_xuyao_chxi = true;
    accumulative = 0;
    chlv_chxi_reason = 2; // 累积冲洗超滤
  }
}

void receive_zs_data()
{
  if (work_state != receive_zs_data_n) // 工作状态制水废水过少
  {
    work_state = receive_zs_data_n; // 工作状态为预存洗膜状态
    upload("state", work_state);    // 上传工作状态
  }
  if (server.hasArg("plain"))
  {
    server.send(200, "text/plain", "ok");
    String body = server.arg("plain");
    Serial.println("Received data: " + body);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
      Serial.println("Failed to parse JSON: " + String(error.c_str()));
      return;
    }
    // 从 JSON 中提取 SSID 和密码
    yucun_ximo_yunxu_shch = doc["yc_bzh"].as<byte>();
    zs_pulse_bzh = doc["zs_pulse_bzh"].as<byte>();

    dc_zs_shch_bz = doc["dc_zs_shch_bz"].as<byte>();
    zhsh_yshi_bzh = doc["zhsh_yshi_bzh"].as<byte>();
    chaoshi_cishu_bzh = doc["bm_cishu"].as<byte>();
    zs_gs_cs_bzh = doc["zs_gs_cs_bzh"].as<byte>();
    ys_chxi_shch = doc["ys_chxi_shch"].as<byte>();
    ys_chxi_gb_shch = doc["ys_chxi_gb_shch"].as<byte>();
    zsh_chxi_sch = doc["zsh_chxi_sch"].as<byte>();
    xm_gd_bzh = doc["xm_gd_bzh"].as<byte>();
    xm_gj_bzh = doc["xm_gj_bzh"].as<byte>();

    /* Serial.println("readed  parameters:");
    Serial.println("zs_pulse_bzh: " + String(zs_pulse_bzh));
    Serial.println("dc_zs_shch_bz: " + String(dc_zs_shch_bz));
    Serial.println("zs_gs_cs_bzh: " + String(zs_gs_cs_bzh));
    Serial.println("ys_chxi_shch: " + String(ys_chxi_shch));
    Serial.println("ys_chxi_gb_shch: " + String(ys_chxi_gb_shch)); */
    // 存储到 Preferences
    pre.putInt("yc_bzh", yucun_ximo_yunxu_shch); // 存储预存洗膜时间
    pre.putInt("zs_pulse_bzh", zs_pulse_bzh);       // 存储制水脉冲数
    pre.putInt("dc_zs_shch_bz", dc_zs_shch_bz);     // 存储单次制水时长
    pre.putInt("zhsh_yshi_bzh", zhsh_yshi_bzh);     // 存储制水延时标准
    pre.putInt("bm_cishu", chaoshi_cishu_bzh);      // 存储制水超时次数标准
    pre.putInt("zs_gs_cs_bzh", zs_gs_cs_bzh);       // 存储制水过少处理次数
    pre.putInt("ys_chxi_shch", ys_chxi_shch);       // 存储预存洗膜时间
    pre.putInt("ys_chxi_gb_shch", ys_chxi_gb_shch); // 存储预存洗膜关闭时间
    pre.putInt("zsh_chxi_sch", zsh_chxi_sch);       // 存储制水后冲洗时长
    pre.putInt("xm_gd_bzh", xm_gd_bzh);             // 存储洗膜时间过短标准
    pre.putInt("xm_gj_bzh", xm_gj_bzh);             // 存储洗膜时间过久标准
  }
  else
  {
    Serial.println("No data received.");
  }
}

void send_zs_data()
{
  if (work_state != send_zs_data_n) // 工作状态为发送制水数据
  {
    work_state = send_zs_data_n; // 工作状态为发送制水数据
    upload("state", work_state); // 上传工作状态
  }
  JsonDocument doc;
  doc["deviceName"] = device_name;
  doc["zs_pulse_bzh"] = zs_pulse_bzh;
  doc["dc_zs_shch_bz"] = dc_zs_shch_bz;
  doc["yc_bzh"]= yucun_ximo_yunxu_shch;
  doc["zs_gs_cs_bzh"] = zs_gs_cs_bzh;
  doc["zhsh_yshi_bzh"] = zhsh_yshi_bzh;
  doc["bm_cishu"] = chaoshi_cishu_bzh;
  doc["ys_chxi_shch"] = ys_chxi_shch;
  doc["ys_chxi_gb_shch"] = ys_chxi_gb_shch;
  doc["zsh_chxi_sch"] = zsh_chxi_sch;
  doc["xm_gd_bzh"] = xm_gd_bzh;
  doc["xm_gj_bzh"] = xm_gj_bzh;
  // Serial.println("dc_zs_shch_bz: " + String(dc_zs_shch_bz));

  String jsonResponse;
  serializeJson(doc, jsonResponse);

  server.send(200, "application/json", jsonResponse);
}

void send_onenet_data()
{
  if (work_state != send_chlv_data_n) // 工作状态制水废水过少
  {
    work_state = send_chlv_data_n; // 工作状态为预存洗膜状态
    upload("state", work_state);   // 上传工作状态
  }
  JsonDocument doc;
  doc["deviceName"] = device_name;
  //doc["product_id"] = s_product_id;
  doc["device_id"] = s_device_id;
  if(s_product_leibie==""){

    for (int i = 0; i < PRODUCT_COUNT; i++)
    {
      if (s_product_id == productKeys[i].product_id)
      {
        
        s_product_leibie = productKeys[i].leibie;
       
        break;
      }
    }
  }

   doc["productCategory"] = s_product_leibie;

  if (client.connected())
  {
    doc["onenetStatus"] = true;
  }
  else
  {
    doc["onenetStatus"] = false;
  }

  String jsonResponse;
  serializeJson(doc, jsonResponse);

  server.send(200, "application/json", jsonResponse);
}
void receive_onenet_data()
{
  if (work_state != receive_onenet_data_n) //
  {
    work_state = receive_onenet_data_n; //
    upload("state", work_state);        // 上传工作状态
  }
  if (server.hasArg("plain"))
  {
    server.send(200, "text/plain", "ok");
    String body = server.arg("plain");
    Serial.println("Received data: " + body);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
      Serial.println("Failed to parse JSON: " + String(error.c_str()));
      return;
    }
    // 从 JSON 中提取 SSID 和密码

    device_name = doc["deviceName"].as<String>();
    // Serial.println(s);
    // product_id =const_cast<char *>(s.c_str());
    // Serial.println(product_id);

    s_device_id = doc["device_id"].as<String>();
    // s.trim();
    // device_id = const_cast<char *>(s.c_str());

    String s = doc["productCategory"].as<String>();
    // s_api_key = doc["s_api_key"].as<const char*>();
    for (int i = 0; i < PRODUCT_COUNT; i++)
    {
      if (s == productKeys[i].leibie)
      {
        s_api_key = productKeys[i].api_key;
        s_product_leibie = productKeys[i].leibie;
        s_product_id = productKeys[i].product_id;
        break;
      }
    }

    Serial.println("Updated onenet parameters:");
    Serial.println(s_product_id);
    Serial.println(s_device_id);
    Serial.print("productCategory: ");
    Serial.println(s_product_leibie);

    pre.putString("s_product_id", s_product_id); // 存储产品名称
    pre.putString("s_device_id", s_device_id);   // 存储onenet产品ID
    pre.putString("s_api_key", s_api_key);// 存储onenet产品访问密钥
   // pre.putString("product_leibie", s_product_leibie);
    OneNet_Connect();
  }
  else
  {
    Serial.println("No data received.");
  }
}
void receive_wifi_data()
{
  if (work_state != receive_wifi_data_n) // 工作状态制水废水过少
  {
    work_state = receive_wifi_data_n; // 工作状态为预存洗膜状态
    upload("state", work_state);      // 上传工作状态
  }
  if (server.hasArg("plain"))
  {
    server.send(200, "text/plain", "ok");
    String body = server.arg("plain");
    Serial.println("Received data: " + body);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
      Serial.println("Failed to parse JSON: " + String(error.c_str()));
      return;
    }
    // 从 JSON 中提取 SSID 和密码

    s_ssid = doc["ssid"].as<String>();
    // ssid =const_cast<char *>(s.c_str());
    s_password = doc["password"].as<String>();
    // password = const_cast<char *>(s.c_str());
    WiFi_Connect();
    Serial.println("Updated WiFi parameters:");
    Serial.print("s_SSID: ");
    Serial.println(s_ssid);
    Serial.print("s_Password: ");
    Serial.println(s_password);

    pre.putString("s_ssid", s_ssid);         // 存储WiFi SSID
    pre.putString("s_password", s_password); // 存储WiFi密码
  }
  else
  {
    Serial.println("No data received.");
  }
}
void send_wifi_data()
{
  if (work_state != send_wifi_data_n) // 工作状态制水废水过少
  {
    work_state = send_wifi_data_n; // 工作状态为预存洗膜状态
    upload("state", work_state);   // 上传工作状态
  }
  JsonDocument doc;
  doc["deviceName"] = device_name;
  doc["ssid"] = s_ssid;
  doc["password"] = s_password;
  if (WiFi.status() == WL_CONNECTED)
  {
    doc["wifiStatus"] = true;
  }
  else
  {
    doc["wifiStatus"] = false;
  }

  String jsonResponse;
  serializeJson(doc, jsonResponse);

  server.send(200, "application/json", jsonResponse);
}
/**/

void send_wifi_list()
{
  if (work_state != send_wifi_list_n) // 工作状态制水废水过少
  {
    work_state = send_wifi_list_n; // 工作状态为预存洗膜状态
    upload("state", work_state);   // 上传工作状态
  }

  JsonDocument doc;
  JsonArray wifiList = doc.to<JsonArray>();

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i)
  {
    JsonObject wifi = wifiList.createNestedObject();
    wifi["ssid"] = WiFi.SSID(i);
    wifi["rssi"] = WiFi.RSSI(i);
    wifi["encryptionType"] = WiFi.encryptionType(i);
  }

  String jsonResponse;
  serializeJson(doc, jsonResponse);

  server.send(200, "application/json", jsonResponse);
  WiFi.scanDelete(); // 清除扫描结果
}

void send_chlv_data()
{
  if (work_state != send_chlv_data_n) // 工作状态制水废水过少
  {
    work_state = send_chlv_data_n; // 工作状态为发送超滤数据
    upload("state", work_state);   // 上传工作状态
  }

  JsonDocument doc;
  doc["deviceName"] = device_name;
  doc["chlv_chxi_cishu"] = chlv_chxi_cishu;
  doc["chlv_open_shch"] = chlv_open_shch;
  doc["chlv_close_shch"] = chlv_close_shch;
  doc["chxi_leiji_bzh"] = chxi_leiji_bzh;
  doc["accumulative"] = accumulative;
  String jsonResponse;
  serializeJson(doc, jsonResponse);

  server.send(200, "application/json", jsonResponse);
}
void receive_chlv_data()
{
  if (work_state != receive_chlv_data_n) // 工作状态制水废水过少
  {
    work_state = receive_chlv_data_n; // 工作状态为预存洗膜状态
    upload("state", work_state);      // 上传工作状态
  }

  // if (server.hasArg("plain"))
  if (server.hasArg("plain"))
  {
    server.send(200, "text/plain", "ok");
    String body = server.arg("plain");
    Serial.println("Received data: " + body);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
      Serial.println("Failed to parse JSON: " + String(error.c_str()));
      return;
    }

    chlv_chxi_cishu = doc["chlv_chxi_cishu"];
    pre.putShort("chlv_chxi_cishu", chlv_chxi_cishu); // 存储超滤冲洗次数
    chlv_open_shch = doc["chlv_open_shch"];
    pre.putShort("chlv_open_shch", chlv_open_shch); // 存储超滤冲洗打开时间
    chlv_close_shch = doc["chlv_close_shch"];
    pre.putShort("chlv_close_shch", chlv_close_shch); // 存储超滤冲洗关闭时间
    chxi_leiji_bzh = doc["chxi_leiji_bzh"];
    pre.putShort("chxi_leiji_bzh", chxi_leiji_bzh); // 存储超滤累计标准

    Serial.println("Updated parameters:");
    // Serial.println("name:"+String(name));
    Serial.println("chlv_chxi_cishu: " + String(chlv_chxi_cishu));
    Serial.println("chlv_open_shch: " + String(chlv_open_shch));
    Serial.println("chlv_close_shch: " + String(chlv_close_shch));
    Serial.println("chxi_leiji_bzh: " + String(chxi_leiji_bzh));
  }
  else
  {
    Serial.println("No data received.");
  }
}

void send_product_data()
{
  if (work_state != send_product_data_n) // 工作状态制水废水过少
  {
    work_state = send_product_data_n; // 工作状态为发送超滤数据
    upload("state", work_state);      // 上传工作状态
  }

  JsonDocument doc;
  doc["deviceName"] = device_name;
  doc["version"] = version;
  doc["jianjie"] = jianjie;
  doc["gongneng"] = gongneng;
  String jsonResponse;
  serializeJson(doc, jsonResponse);

  server.send(200, "application/json", jsonResponse);
}
void receive_product_data()
{
  if (work_state != receive_product_data_n) // 工作状态接收手机端发送的数据
  {
    work_state = receive_product_data_n; // 工作状态为接收手机端发送的数据
    upload("state", work_state);         // 上传工作状态
  }

  if (server.hasArg("plain"))
  {
    server.send(200, "text/plain", "ok");
    String body = server.arg("plain");
    Serial.println("Received data: " + body);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
      Serial.println("Failed to parse JSON: " + String(error.c_str()));
      return;
    }

    device_name = doc["name"].as<String>();
    jianjie = doc["jianjie"].as<String>();
    gongneng = doc["gongneng"].as<String>();
    version = doc["version"].as<int>();

    pre.putString("name", device_name);         // 存储产品名称
    pre.putString("jianjie", jianjie);   // 存储产品简介
    pre.putString("gongneng", gongneng); // 存储产品功能
    pre.putInt("version", version);      // 存储产品版本

    Serial.println("Updated parameters:");
    Serial.print("device_name: ");
    Serial.println(device_name);
    Serial.print("jianjie: ");
    Serial.println(jianjie);
    Serial.print("gongneng: ");
    Serial.println(gongneng);
    Serial.print("version: ");
    Serial.println(version);
  }
  else
  {
    Serial.println("No data received.");
  }
}
void send_ota_data()
{
  if (work_state != send_ota_data_n) // 工作状态发送OTA数据
  {
    work_state = send_ota_data_n; // 工作状态为发送超滤数据
    upload("state", work_state);  // 上传工作状态
  }

  JsonDocument doc;
  doc["deviceName"] = device_name;
  doc["ota"] = partionName;

  String jsonResponse;
  serializeJson(doc, jsonResponse);

  server.send(200, "application/json", jsonResponse);
}

void receive_ota_data()
{
  if (work_state != receive_ota_data_n) // 工作状态接收手机端发送的数据
  {
    work_state = receive_ota_data_n; // 工作状态为接收手机端发送的数据
    upload("state", work_state);     // 上传工作状态
  }

  if (server.hasArg("plain"))
  {
    server.send(200, "text/plain", "ok");
    String body = server.arg("plain");
    Serial.println("Received data: " + body);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
      Serial.println("Failed to parse JSON: " + String(error.c_str()));
      return;
    }
    const char *action = doc["action"];
    const char *partition = doc["partition"];

    if (strcmp(action, "switch_partition") == 0)
    {

      // Serial.println("zhb qie huan");
      //  查找目标分区
      esp_partition_subtype_t subtype = (strcmp(partition, "ota_0") == 0) ? ESP_PARTITION_SUBTYPE_APP_OTA_0 : ESP_PARTITION_SUBTYPE_APP_OTA_1;
      Serial.println(subtype);
      const esp_partition_t *target = esp_partition_find_first(
          ESP_PARTITION_TYPE_APP, subtype, NULL);

      // Serial.println(target->subtype);
      if (target)
      {
        Serial.println("finded target partition");
        // 设置启动分区并重启
        esp_ota_set_boot_partition(target);
        state_str = "重启切换到分区：" + String(partition);
        state_message = const_cast<char *>(state_str.c_str());
        display();
        server.send(200, "text/plain", "OK: Partition switched, rebooting...");
        delay(1000);
        esp_restart();
      }
      else
      {
        server.send(404, "text/plain", "Error: Partition not found!");
      }
    }
    else
    {
      server.send(400, "text/plain", "Error: Invalid action!");
    }
  }
  else
  {
    Serial.println("No data received.");
  }
}

void shtmo_chxi()
{

  if (work_state != shtmo_chxi_n) // 工作状态制水废水过少
  {
    work_state = shtmo_chxi_n;   // 工作状态为预存洗膜状态
    upload("state", work_state); // 上传工作状态
  }
  shtmo_chxi_flag = true; // 设置洗膜冲洗标志位
  Serial.println("shtmo_chxi_flag set to true");
  // 返回响应
  server.send(200, "text/plain", "Flushing started");
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("recept: ");
  Serial.println(topic);
  Serial.print("content: "); // recept: $sys/WoMShsmX5V/test/thing/property/set??content: {"id":"12","version":"1.0","params":{"feishui_liuliang_bzh":12}}

  for (int i = 0; i < length; i++) // ������ӡ��Ϣ����
  {
    Serial.print((char)payload[i]); // ��ӡ��Ϣ����
  }
  Serial.println();

  String s = topic;
  // Serial.println(s);
  if (s.indexOf("/cmd/request/") > 0) // ����յ�mqtt�·�����
  {
    Serial.println("receive command ");

    // 1. 提取命令ID（cmdid）
    String cmdid = s.substring(s.lastIndexOf('/') + 1);
    Serial.println("Command ID: " + cmdid);
    // 2. 准备响应内容（明文）

    JsonDocument doc;
    // doc["id"] = message_id;
    doc["errno"] = 0;
    doc["error"] = "Success";
    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    // String jsonResponse = "{\"status\":\"ok\"}"; // 你的实际响应数据

    // 5. 发布到响应主题
    String responseTopic = "$sys/" + String(s_product_id) + "/" + String(s_device_id) + "/cmd/response/" + cmdid;
    // String responseTopic = "/res?qt=1&device_id=" + String(s_device_id) + "&api_key=" + String(s_api_key);
    client.publish(responseTopic.c_str(), jsonBuffer);
    Serial.println(jsonBuffer);

    Serial.println("Sent topic: " + responseTopic);
  }

  else if (s.indexOf("/property/set") > 0) // ����յ�mqtt�·�����
  {
    Serial.println("receive property set");
    String body = "";
    for (int i = 0; i < length; i++)
    {
      body += (char)payload[i];
    }
    Serial.println("Received data: " + body);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
      Serial.println("Failed to parse JSON: " + String(error.c_str()));
      return;
    }
    if (doc.containsKey("id"))
    {
      String message_id = doc["id"].as<String>();
      Serial.println("Message ID: " + message_id);
      send_set_response(message_id);
    }
    // 从 JSON 中提取参数
    if (doc.containsKey("params"))
    {
      JsonObject params = doc["params"];

      if (params.containsKey("chlv_chxi_cishu"))
      {
        chlv_chxi_cishu = params["chlv_chxi_cishu"].as<int>();
        pre.putInt("chlv_chxi_cishu", chlv_chxi_cishu);
        Serial.println("Updated chlv_chxi_cishu: " + String(chlv_chxi_cishu));
      }
      if (params.containsKey("chlv_open_shch"))
      {
        chlv_open_shch = params["chlv_open_shch"].as<int>();
        pre.putInt("chlv_open_shch", chlv_open_shch);
        Serial.println("Updated chlv_open_shch: " + String(chlv_open_shch));
      }
      if (params.containsKey("chlv_close_shch"))
      {
        chlv_close_shch = params["chlv_close_shch"].as<int>();
        pre.putInt("chlv_close_shch", chlv_close_shch);
        Serial.println("Updated chlv_close_shch: " + String(chlv_close_shch));
      }
      if (params.containsKey("chxi_leiji_bzh"))
      {
        chxi_leiji_bzh = params["chxi_leiji_bzh"].as<int>();
        pre.putInt("chxi_leiji_bzh", chxi_leiji_bzh);
        Serial.println("Updated chxi_leiji_bzh: " + String(chxi_leiji_bzh));
      }
    }
  }
}

void pre_read()
{

  pre.begin("lyd"); // �������ռ䣨����������򴴽���????
  if (!pre.isKey("zong_shch"))
  {

    Serial.println("zong shch not found, initializing to 0");
    pre.putULong("zong_shch", 0);
  }
  else
  {
    Serial.println("zong shch found, reading value");
  }
  zhishui_shich = pre.getULong("zong_shch");
  /* ---------------------------------------------------超滤---------------------------*/
  chlv_chxi_cishu = pre.getInt("chlv_chxi_cishu", chlv_chxi_cishu); // 超滤冲洗次数
  chlv_open_shch = pre.getInt("chlv_open_shch", chlv_open_shch);    // 超滤冲洗打开时间
  chlv_close_shch = pre.getInt("chlv_close_shch", chlv_close_shch); // 超滤冲洗关闭时间
  chxi_leiji_bzh = pre.getInt("chxi_leiji_bzh", chxi_leiji_bzh);    // 超滤累计标准
  accumulative = pre.getInt("accumulative", accumulative);
  /* -----------------------------------------制水---------------------------*/
  yucun_ximo_yunxu_shch = pre.getInt("yc_bzh", yucun_ximo_yunxu_shch); // 预存洗膜时间
  zs_pulse_bzh = pre.getInt("zs_pulse_bzh", zs_pulse_bzh);
  dc_zs_shch_bz = pre.getInt("dc_zs_shch_bz", dc_zs_shch_bz);//单次制水时长标准
  zhsh_yshi_bzh = pre.getInt("zhsh_yshi_bzh", zhsh_yshi_bzh); // 制水延时标准
  chaoshi_cishu_bzh = pre.getInt("bm_cishu", chaoshi_cishu_bzh); // 制水超时次数标准
  zs_gs_cs_bzh = pre.getInt("zs_gs_cs_bzh", zs_gs_cs_bzh);
  ys_chxi_shch = pre.getInt("ys_chxi_shch", ys_chxi_shch);
  ys_chxi_gb_shch = pre.getInt("ys_chxi_gb_shch", ys_chxi_gb_shch);
  zsh_chxi_sch = pre.getInt("zsh_chxi_sch", zsh_chxi_sch); // 原水强冲秒数
  /*Serial.println("readed  parameters:");
  Serial.println("zs_pulse_bzh: " + String(zs_pulse_bzh));
  Serial.println("dc_zs_shch_bz: " + String(dc_zs_shch_bz));
  Serial.println("zs_gs_cs_bzh: " + String(zs_gs_cs_bzh));
  Serial.println("ys_chxi_shch: " + String(ys_chxi_shch));
  Serial.println("ys_chxi_gb_shch: " + String(ys_chxi_gb_shch));*/
  /* -----------------------------------------洗膜---------------------------*/
    xm_gd_bzh=(pre.getInt("xm_gd_bzh", xm_gd_bzh)); // 洗膜时间过短标准
    xm_gj_bzh=(pre.getInt("xm_gj_bzh", xm_gj_bzh)); // 洗膜时间过久标准



  /* ---------------------------------------wifi onenet--------------------------*/
  s_ssid = pre.getString("s_ssid", "lyd");
  s_password = pre.getString("s_password", "19711128");
  s_product_id = pre.getString("s_product_id", s_product_id);
  s_device_id = pre.getString("s_device_id", s_device_id);
  s_api_key= pre.getString("s_api_key");
  // Serial.println(s);
   //s_api_key =s.c_str();
  Serial.println(s_api_key);
  /* Serial.println("s_product_id:"+s_product_id);
  Serial.println("s_device_id:"+s_device_id);
  Serial.println("s_api_key:"+s_api_key); */
}

void OneNet_Connect()
{
  if (WiFi.status() == WL_CONNECTED) // 如果WiFi未连接
  {

    client.setServer(mqtt_server, mqtt_port); // 设置MQTT服务器地址和端口
    // device_id = const_cast<char *>(s_device_id.c_str());
    // product_id = const_cast<char *>(s_product_id.c_str());
    // api_key = const_cast<char *>(s_api_key.c_str());

    // Serial.println(device_id);Serial.println(s_device_id);Serial.println(s_device_id.c_str());
    // Serial.println(product_id);Serial.println(s_product_id);Serial.println(s_product_id.c_str());
    // Serial.println(api_key);Serial.println(s_api_key);Serial.println(s_api_key.c_str());
    // client.connect(device_id, product_id, api_key); // 连接OneNet

    client.connect(s_device_id.c_str(), s_product_id.c_str(), s_api_key.c_str()); // 连接OneNet
    delay(1000);
    if (client.connected()) // 如果连接成功
    {
      // LED_Flash(500);
      Serial.println("OneNet okOK!");
      state_message = "物联网连接成功";
      display();
      client.subscribe(data_topic); // 订阅设备属性上报响应,OneNET---->设备
      // client.subscribe("$sys/hFOtBOuext/ceshi/cmd/#");
      client.subscribe(set_topic); // 订阅属性设置，OneNET---->设备
      // client.subscribe(set_reply_topic); // 订阅属性设置响应，OneNET---->设备
      client.subscribe(cmd_topic); // 订阅下发命令, OneNET---->设备
      // client.subscribe(cmd_response_topic); // 订阅命令响应, OneNET---->设备
      Serial.println("Subscribed to cmd topic: " + String(cmd_topic));
      // client.subscribe(cmd_response_topic); // 命令响应主题
    }
    else
    {
      Serial.println("OneNet fail!");
      state_message = "物联网连接失败";
      display();
      Serial.println("错误代码: " + String(client.state())); // #define MQTT_CONNECTED               0
                                                             //   #define MQTT_CONNECT_BAD_PROTOCOL    1
      // #define MQTT_CONNECT_BAD_CLIENT_ID   2
      // #define MQTT_CONNECT_UNAVAILABLE     3
      // #define MQTT_CONNECT_BAD_CREDENTIALS 4
      // #define MQTT_CONNECT_UNAUTHORIZED    5
    }

    // client.subscribe  (data_reply_topic); //订阅后才有回复                                     // client.subscribe(ONENET_TOPIC_test_sub);
    // client.subscribe(onenet_property_get);
    // client.subscribe(ONENET_TOPIC_cmd);
    //  client.subscribe(ONENET_TOPIC_PROP_DESIRED_GET_REPLY);//
  }
  else
  {
    Serial.println("WiFi not connected, cannot connect to OneNet");
    state_message = "WiFi未连接，无法连接OneNet";
    display();
  }
}
void zs_upload()
{
  if (client.connected()) // 如果连接成功
  {
    // 构建数据JSON (符合OneNET物模型)
    JsonDocument doc;
    char jsonBuf[256];
    doc["id"] = String(postMsgId++); // id必须是字符串类型
    doc["version"] = "1.0";

    // JsonObject params = doc.createNestedObject("params");
    doc["params"].to<JsonObject>();                // 创建params对象
    doc["params"]["zs_pulse"]["value"] = zs_pulse; // 制水脉冲数
    // doc["params"]["dc_zs_shch"]["value"] = danci_zhishui_shch;
    doc["params"]["zs_mc_bzh"]["value"] = zs_pulse_bzh;

    // 正确格式：{"id":"13","version":"1.0","params":{"zong_shch":{"value":0},"dc_zs_shch":{"value":0},"zs_mc_bzh":{"value":18}}}

    serializeJson(doc, jsonBuf);

    // 发送数据
    client.publish(data_topic, jsonBuf);
    // Serial.println("Data uploaded to OneNET");

    // 打印上传的数据
    // Serial.println("Uploaded data:");
    // Serial.println(jsonBuf);
  }
}
void send_set_response(String message_id)
{
  // 构建响应JSON
  JsonDocument doc;
  doc["id"] = message_id;
  doc["code"] = 0;
  doc["msg"] = "Success";

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  // 发送响应
  client.publish(set_reply_topic, jsonBuffer);
  Serial.println("Set response sent: " + String(jsonBuffer));
  Serial.println("Set response sent to topic: " + String(set_reply_topic));
}
void upload(String item, int value)
{
  if (client.connected()) // 如果连接成功
  {
    JsonDocument doc;
    char jsonBuf[150];
    doc["id"] = String(postMsgId++); // id必须是字符串类型
    doc["version"] = "1.0";
    doc["params"].to<JsonObject>(); // 创建params对象
    doc["params"][item]["value"] = value;
    serializeJson(doc, jsonBuf);
    client.publish(data_topic, jsonBuf);
    // Serial.println("Data uploaded to OneNET");
    // Serial.println(jsonBuf);
  }
}
void upload(String item, String value)
{
  if (client.connected()) // 如果连接成功
  {
    JsonDocument doc;
    char jsonBuf[150];
    doc["id"] = String(postMsgId++); // id必须是字符串类型
    doc["version"] = "1.0";
    doc["params"].to<JsonObject>();       // 创建params对象
    doc["params"][item]["value"] = value; //
    serializeJson(doc, jsonBuf);
    client.publish(data_topic, jsonBuf);
    // Serial.println("Data uploaded to OneNET");
    // Serial.println(jsonBuf);
  }
}

void yucun_ximo() // 预存洗膜状态
{

  if (shuiya_zhch == true ) 
  {
    // �??????动后�??????0状态，或从预存洗膜完成后的40状态过来。�?�果高水位和预存洗膜水均满，在水满纯水�?�洗膜发生吸水故障，
    // 会在�??????水�?�吸水洗膜时造成洗膜开关闭合，如果凑巧高水位开关闭合，会从30状态直接到10状态�?

    if (changjiu_qian_zhtai == 0 || changjiu_qian_zhtai == 40 || changjiu_qian_zhtai == 30 || changjiu_qian_zhtai == 20) // �??????进入一�??????
    {

      openValve(zong_dcf);    // 打开总进水电磁阀
      zong_dcf_state = true;  // 总进水电磁阀打开状态
      openValve(jinshui_dcf); // 打开泵前进水电磁阀

      digitalWrite(zengya_beng, 1); /*打开增压泵*/
      zyb_qidong_shike = millis();  // 记录制水开始时刻
      openValve(js_dcf);            // 打开净水电磁阀

      if (changjiu_qian_zhtai == 0) // 预存洗膜水满
      {
        state_message = "刚启动";
        display(); // 现状态进入状态时�??????
      }
      if(changjiu_qian_zhtai == 20) // 预存洗膜水满
      {
        state_message = "正常制水时洗膜预存开关闭合，检查预存开关或漏水";
        display(); // 现状态进入状态时�??????
      }
      if (changjiu_qian_zhtai == 40) // 预存洗膜水满
      {
        state_message = "前态是40";
        display(); // 现状态进入状态时刻hui
      }
      if (changjiu_qian_zhtai == 30) //
      {
        state_message = "前态是30";
        display(); // 现状态进入状态时�??????
      }
      queshui_start = 0; // 为缺水处理复位

      zhishui_kaishi_shke = millis(); // 开始制水时刻，为计算制水时间准备
      feishui_chxi_state = 0;

      yucun_kaishi_shike = 0;                    // 预存洗膜开始时刻，为计算预存洗膜时间准备
      upload("state_qian", changjiu_qian_zhtai); // 上传前态状态
      changjiu_qian_zhtai++;                     // 加1为了代码执行一次
    }
    if (millis() - zyb_qidong_shike > yc_delay && yucun_kaishi_shike == 0) // 进入一次
    {
      closeValve(js_dcf);            // 关闭js_dcf
      yucun_kaishi_shike = millis(); // 预存洗膜开始时刻，为计算预存洗膜时间准备
      state_message = "开始预存洗膜水";
      display(); //
    }
    qidong_zengyabeng(); //

    if (millis() - yucun_kaishi_shike > yucun_ximo_yunxu_shch * 60000 && yucun_kaishi_shike != 0) //  单位：分钟 保证预存延迟后再计时
    {

      state_message = "预存洗膜超时";
      display();

      yucun_buneng_shuiman = true; // 预存洗膜不能水满
    }

    if (millis() - zhtai_xianshi_jiange > 10000 && digitalRead(feishui_chxi_dcf) != 1) // 间隔显示,多次进入
    {
      state_str = "预存洗膜耗时";
      int i = (millis() - xiantai_jin_shike) / 60000; // 分钟�??????
      state_str.concat(i);
      state_str += " 分钟";
      state_message = const_cast<char *>(state_str.c_str());

      display(); //

      zhtai_xianshi_jiange = millis();
      if (work_state != 10) // 工作状态制水废水过少
      {
        work_state = 10;             // 工作状态为预存洗膜状�?
        upload("state", work_state); // 上传工作状态
      }
    }
  }
  else
  {
    if (queshui_start == 0) // 缺水后延迟制水停机，为了多存洗膜水或避免�??????暂缺�??????
    {
      digitalWrite(feishui_chxi_dcf, 0); // 提前关闭废水冲洗电�?�阀
      queshui_start = millis();
    }
    if (millis() - queshui_start > 10000 && digitalRead(zengya_beng) == 1)
    {
      queshui_start = 0; // 缺水处理复位
      digitalWrite(zengya_beng, 0);
      digitalWrite(jinshui_dcf, 0);
      digitalWrite(js_dcf, 0); // 关闭js_dcf
    }
  }
}
void zhch_zhishui() // 正常制水状态
{

  if (shuiya_zhch == true)
  {

    if (changjiu_qian_zhtai == 10 || changjiu_qian_zhtai == 0 || changjiu_qian_zhtai == 30) //
    {

      openValve(zong_dcf);
      zong_dcf_state = true; // 总进水电磁阀打开状态
      openValve(js_dcf);     // 打开净水电磁阀
      openValve(jinshui_dcf);

      digitalWrite(zengya_beng, 1);  // 增压泵
      if (changjiu_qian_zhtai != 10) // 开机直接制水
      {
        zyb_qidong_shike = millis(); // 记录制水开始时刻`
      }

      if (changjiu_qian_zhtai == 30) // 预存洗膜水满
      {
        state_message = "水满后又快速不水满，检查水满开关";
        display(); // 现状态进入状态时�??????
      }
      if (changjiu_qian_zhtai == 10) // 预存洗膜水满
      {
        state_message = "前态是预存洗膜";
        // cengjing_yucun = true; // 记录曾经预存洗膜�???
        data_str = "预存洗膜水满用了";

        data_str.concat((xiantai_jin_shike - yucun_kaishi_shike) / 60000); // 计算预存洗膜水满用时
        data_str += "分钟";
        data_message = const_cast<char *>(data_str.c_str());
        display();                                                                 //
        upload("yucun_yongshi", (xiantai_jin_shike - yucun_kaishi_shike) / 60000); // 上传预存洗膜水满用时 单位分钟
      }
      if (changjiu_qian_zhtai == 0) // 开机就正常制水，�?�存开关断开
      {
        zhishui_kaishi_shke = millis(); // 开始制水时刻，为计算制水时间准备
        state_message = "刚启动";
        display(); // 现状态进入状态时�??????
      }

      queshui_start = 0; // 为缺水�?�理复位

      if (changjiu_qian_zhtai != 10)
      {

        feishui_chxi_state = 0; // 废水冲洗状态�?�位
      }

      // feishui_chxi_hukong = false;
      // feishui_chxi_kshi_shke = millis(); // 废水冲洗开始时间点

      yucun_buneng_shuiman_cishu = 0;            // 预存洗膜不能水满次数�??????0
      upload("state_qian", changjiu_qian_zhtai); // 上传前态状态

      changjiu_qian_zhtai++;
       if (work_state != 20) // 工作状态制水废水过少
      {
        work_state = 20; // 工作状态为预存洗膜状�?
      }
            upload("state", work_state); // 上传工作状态
    }

    qidong_zengyabeng();
    if (millis() - zhishui_kaishi_shke > dc_zs_shch_bz * 60000) // 制水超时不满  制水允�?�时�?????? 单位：分钟
    { zhishui_hou_chxi_state = 0;//为了原水冲洗复位0 
      zhishui_chaoshi_cishu++;
      baohu_zht=0;
      zhishui_chaoshi = true; // 制水超时

      closeValve(js_dcf); // 关闭js_dcf
    }

    if (millis() - zhtai_xianshi_jiange > 6000) // 间隔显示,多次进入
    { 

      switch (xianshi)
      {
      case 0:
        
      state_str = "设定制水时长";
      state_str.concat(dc_zs_shch_bz);
     state_str += " 分钟";
      state_message = const_cast<char *>(state_str.c_str());
      
       data_str = "制水了";
       
      data_str.concat((millis() - zhishui_kaishi_shke) / 60000);
      data_str += " 分钟";
      data_message = const_cast<char *>(data_str.c_str());

        break;
      
      case 1:
        state_str = "设定制水超时次数";
      state_str.concat(chaoshi_cishu_bzh);
     state_str += "次";
      state_message = const_cast<char *>(state_str.c_str());
      
       data_str = "已超时";
      data_str.concat(zhishui_chaoshi_cishu);
      data_str += " 次";      
      data_message = const_cast<char *>(data_str.c_str());
        break;
       
      }
      xianshi++;
      if (xianshi > 1)
      {
        xianshi = 0;//置0为下次显示准备
      }

      display();                   // 显示制水时间     
     
      zhtai_xianshi_jiange = millis();
    }
  }
  else
  {
    if (queshui_start == 0) //
    {
      closeValve(js_dcf); // 关闭js_dcf
      digitalWrite(feishui_chxi_dcf, 0);
      queshui_start = millis();
    }
    if (millis() - queshui_start > 10000 && digitalRead(zengya_beng) == 1) // 缺水后延迟制水停机，防�?�短暂缺�??????
    {

      digitalWrite(zengya_beng, 0);
      closeValve(jinshui_dcf);
      queshui_start = 0; // 缺水处理复位
    }
  }
}

void zhishui_shuiman()//制水水满状态，高水位开关断开
{

  if (changjiu_qian_zhtai == 20 || changjiu_qian_zhtai == 0 || changjiu_qian_zhtai == 10 || changjiu_qian_zhtai == 40) // 制水水满状态，或开机直接水满，或预存洗膜水满
  {

    if (changjiu_qian_zhtai == 0)
    {
      state_message = "开机后直接水满，预存开关断开洗膜";
      display();
    }
    if (changjiu_qian_zhtai == 10)
    {
      state_message = "预存洗膜水满后直接水满";
      display();
    }
    if (changjiu_qian_zhtai == 40)
    {
      state_message = "洗膜开关洗膜后又断开，故障";
      display();
    }
    if (changjiu_qian_zhtai == 0 || changjiu_qian_zhtai == 40)
    {
      digitalWrite(feishui_chxi_dcf, 0); // 关闭废水冲洗电磁阀
      digitalWrite(zengya_beng, 0);      // 先关闭增压泵

      closeValve(zong_dcf);
      zong_dcf_state = false;       // 总进水电磁阀关闭状态
      zong_dcf_gb_shike = millis(); // 关闭总进水电磁阀时刻

      closeValve(jinshui_dcf); // 关闭泵前进水电磁阀
      closeValve(js_dcf);      // 关闭净水电磁阀
                               // ximo_kshi_shike = millis(); // 预存洗膜水洗膜开始时刻
    }

    if (changjiu_qian_zhtai == 20 || changjiu_qian_zhtai == 10)
    {
      state_message = "制水水满，预存洗膜水满";
      display();
      danci_zhishui_shch = (xiantai_jin_shike - zhishui_kaishi_shke) / 60000;
      upload("dc_zs_shch", danci_zhishui_shch); // 上传制水单次工作时间
      zhishui_shich += danci_zhishui_shch;      // 单位分钟
      pre.putULong("zong_shch", zhishui_shich); // 存储制水总时�???? 分钟
      upload("zong_shch", zhishui_shich);       // 上传制水总时�???? 分钟
      chaolv_chxi_panduan(danci_zhishui_shch);  // 决定是否需要冲洗超滤膜

      upload("zsh_chxi_sch", zsh_chxi_sch); // 上传制水后冲洗时长，秒

      
        zhishui_delay = true;
      
    }

    danci_zhishui_shch = 0; // 制水单�?�工作时间置0，为下�?�准�??????
    feishui_guoshao_cishu = 0;

    // shuiman_shike = millis(); // 水满时刻

    zhishui_chaoshi_cishu = 0; // 超时次数置0，为下次制水准备
    // yucun_kaishi_shike = 0;    // 预存洗膜水延时开始时刻�?�位
    upload("state_qian", changjiu_qian_zhtai); // 上传前态状态
    changjiu_qian_zhtai++;                     // 加1，保证只进入一次
  }
  if (zhishui_delay == true) // 制水延时
  {
    if (millis() - xiantai_jin_shike > zhsh_yshi_bzh * 60000) // 延时10秒后
    {
      xuyao_feishui_chxi = true;
      zhishui_delay = false;
      zhishui_hou_chxi_state = 0;

    }
    else
    { 
      state_str = "制水水满延时中，延时";
      state_str.concat(zhsh_yshi_bzh);
      state_str += " 分钟";
      state_message = const_cast<char *>(state_str.c_str());
      data_str = "已延时";

      int i = (millis() - xiantai_jin_shike) / 60000; // 已延时分钟数
      data_str.concat(i);
      data_str += " 分钟";
      data_message = const_cast<char *>(data_str.c_str());
      display();
    }
  }
  if (xuyao_feishui_chxi == true)
  {   
    xuyao_feishui_chxi = yuanshui_chxi(xuyao_feishui_chxi); // 水满后原水冲洗
  }
  if (xuyao_feishui_chxi == false && zhishui_hou_chxi_state == 1) // 预存洗膜水延时开始时�??????
  {
    digitalWrite(feishui_chxi_dcf, 0); // 关闭废水冲洗电磁阀
    digitalWrite(zengya_beng, 0);      // 先关闭增压泵
    zyb_tzhi_shike = millis();         // 记录增压泵停止时刻
    Serial.println("预存洗膜水洗膜延时开始");

    closeValve(zong_dcf);
    zong_dcf_state = false;       // 总进水电磁阀关闭状态
    zong_dcf_gb_shike = millis(); // 关闭总进水电磁阀时刻

    closeValve(jinshui_dcf);    // 关闭泵前进水电磁阀
    closeValve(js_dcf);         // 关闭净水电磁阀
    ximo_kshi_shike = millis(); // 预存洗膜水洗膜开始时刻
    zhishui_hou_chxi_state++;   // 状态加1，表示已经处理有关停机
    // 此范围只进入一次
  }

  if (((millis() - ximo_kshi_shike) > (xm_gj_bzh * 60000)) && zhishui_hou_chxi_state == 2) // 25分钟
  {
    state_message = "洗膜水冲洗渗透膜超时";
    display();
    upload("alarm", "洗膜超时，超过" + (String)xm_gj_bzh + "分钟"); // 上传工作状态
    zhishui_hou_chxi_state++;                                                         // 加1，保证只进入一次
  }
  if (millis() - zhtai_xianshi_jiange > 10000 && digitalRead(zengya_beng) == 0) // 间隔显示,多次进入,读取增压泵状态避免制水延时时间显示
  {
    if (work_state != 30) // 工作状态制水废水过少
    {
      work_state = 30; // 工作状态为预存洗膜状�?
    }
    data_str = "制水水满了";
    int i = (millis() - xiantai_jin_shike) / 60000; // 已水满分钟数
    data_str.concat(i);
    data_str += " 分钟";
    data_message = const_cast<char *>(data_str.c_str());
     if(zhishui_delay==false)
    {state_message = "预存洗膜水正在冲洗渗透膜";}

    display();                   // 显示水满时间
    upload("state", work_state); // 上传工作状态
    zhtai_xianshi_jiange = millis();
  }
}
void yucun_ximo_wancheng()
{

  if (leiji_chlv_xuyao_chxi == true)
  {
    leiji_chlv_xuyao_chxi = chaolv_chongxi(leiji_chlv_xuyao_chxi);
  }

  if (changjiu_qian_zhtai == 0 || changjiu_qian_zhtai == 30 || changjiu_qian_zhtai == 10 || changjiu_qian_zhtai == 20) // 预存洗膜水满后，或制水水满后
  {
    if (changjiu_qian_zhtai == 0)
    {
      state_message = "开机后直接水满，预存开关闭合";
      display();
    }
    if (changjiu_qian_zhtai == 30)
    {
      state_message = "制水水满后预存开关闭合";
      display();
    }
    if (changjiu_qian_zhtai == 20)
    {
      state_message = "制水时预存开关闭合，水满";
      display();
    }
    if (changjiu_qian_zhtai == 10)
    {
      state_message = "预存洗膜水时直接水满，预存一直处于闭合状态";
      display();
    }

    // digitalWrite(zengya_beng, 1); // 增压泵运�??????
    // zyb_tzhi_shike = millis();    // 记录增压泵停止时刻
    // openValve(zong_dcf);          // 打开总进水电磁阀
    // zong_dcf_state = true;        // 总进水电磁阀打开状态
    // openValve(jinshui_dcf);       // 打开泵前进水电磁阀
    // openValve(js_dcf);            // 打开净水电磁阀

    feishui_chxi_state = 0; // 废水冲洗状态复位

    // yucun_kaishi_shike = millis(); // 预存洗膜开始时刻，为�?�算预存洗膜时间准�??
    // xiantai_jin_shike = millis();  // 洗膜开始时刻，为�?�算洗膜时间准�??

    // 害怕意外直接压力桶开关水满，此时洗膜开关没有断开，停机所有
    digitalWrite(zengya_beng, 0);      // 关闭增压泵
    digitalWrite(feishui_chxi_dcf, 0); // 关闭废水冲洗电磁阀
    closeValve(zong_dcf);              // 关闭总进水电磁阀
    zong_dcf_state = false;            // 总进水电磁阀关闭状态
    zong_dcf_gb_shike = millis();      // 记录关闭总进水电磁阀时刻
    closeValve(jinshui_dcf);           // 关闭泵前进水电磁阀
    closeValve(js_dcf);                // 关闭净水电磁阀

    if (changjiu_qian_zhtai == 30)
    {
      upload("ximo_yongshi", (xiantai_jin_shike - ximo_kshi_shike) / 1000); // 上传洗膜用时 单位秒
      data_str = "洗膜用时";
      data_str.concat((xiantai_jin_shike - ximo_kshi_shike) / 1000); // 计算洗膜用时
      data_str += " 秒";
      data_message = const_cast<char *>(data_str.c_str());
      display(); //
      //Serial.print("洗膜用时：");
      //Serial.println((xiantai_jin_shike - ximo_kshi_shike) / 1000); // 打印洗膜用时 单位秒
    }
    upload("state_qian", changjiu_qian_zhtai); // 上传前态状态
    changjiu_qian_zhtai++;
  }
  if (millis() - zhtai_xianshi_jiange > 10000) // 间隔显示,多次进入
  {
    if (work_state != 40) // 工作状态制水废水过少
    {
      work_state = 40; // 工作状态为预存洗膜状�?
    }
    switch (xianshi)
    {
    case 0:

    {
      data_str = "预存水洗膜后水满";
      int i = (millis() - xiantai_jin_shike) / 60000; // 已水满分钟数
      data_str.concat(i);
      data_str += " 分钟";
      data_message = const_cast<char *>(data_str.c_str());
      state_message = "预存开关闭合，压力桶水满";
      if (millis() - send_jiange > 180000) // 每3分钟上传一次
      {
        upload("shuiman_m", i); // 上传洗膜后水满时间 单位分钟
        Serial.print("洗膜后水满时间：");
        Serial.println(i);      // 打印洗膜后水满时间 单位分钟
        send_jiange = millis(); // 记录上传时间
      }
      break;
    }
    case 1:
      if (changjiu_qian_zhtai == 31)
      {
        if (xiantai_jin_shike - ximo_kshi_shike < xm_gd_bzh * 1000)//单位秒
        {
          state_message = "洗膜水冲洗用时过短，请检查";
        }
      }
      break;
    case 2:
      data_str = "总制水";

      data_str.concat(zhishui_shich);
      data_str += " 分钟";
      data_message = const_cast<char *>(data_str.c_str());
      state_message = "预存开关闭合，压力桶水满";

      break;

    case 3:
      data_str = "单次制水";

      data_str.concat(danci_zhishui_shch);
      data_str += " 分钟";
      data_message = const_cast<char *>(data_str.c_str());
      state_message = "预存开关闭合，压力桶水满";

      break;
    case 4:
      upload("state", work_state); // 上传工作状态
      break;
    }
    xianshi++;
    if (xianshi >= 5)
    {
      xianshi = 0;
    }
    zhtai_xianshi_jiange = millis();
    display();
  }
}
void qidong_zengyabeng()
{
  if (shtmo_chxi_flag == true)
  {

    if (shtmo_chxi_start_time == 0)      // 进入一次
    {                                    // 如果开始时间为0，表示未开始
      shtmo_chxi_start_time = millis();  // 记录开始时间
      digitalWrite(feishui_chxi_dcf, 1); // 打开废水冲洗电磁阀
      state_str = "渗透膜冲洗开始";
      state_str.concat(shtmo_chxi_duration);
      state_str += " 秒";
      state_message = const_cast<char *>(state_str.c_str());
      display(); // 显示渗透膜冲洗开始信息
    }
    if (millis() - shtmo_chxi_start_time > shtmo_chxi_duration * 1000)
    {                                    // 如果超过设定的冲洗时间
      digitalWrite(feishui_chxi_dcf, 0); // 关闭废水冲洗电磁阀
      shtmo_chxi_flag = false;           // 重置渗透膜冲洗标志
      shtmo_chxi_start_time = 0;         // 重置开始时间
      state_message = "渗透膜冲洗结束";
      display(); // 显示渗透膜冲洗结束信息
    }
  }

  if (feishui_chxi_state == 0) // 初始化
  {
    if (changjiu_qian_zhtai == 0) // 没有预存直接制水
    {
      if (work_state != zhishui_qch_open_n)
      {
        work_state = zhishui_qch_open_n; // 工作状态为制水中强冲
        upload("state", work_state);     // 上传工作状态
      }
    }
    if (changjiu_qian_zhtai == 10)
    {

      if (work_state != yucun_qch_open_n) // 工作状态预存原水强冲
      {
        work_state = yucun_qch_open_n; // 工作状态为预存强冲
        upload("state", work_state);   // 上传工作状态
      }
    }

    // metro0.reset();
    // metro0.interval(ys_chxi_shch * 1000); // // 设置废水冲洗时间
    feishui_chxi_kshi_shke = millis();          // 废水冲洗开始时间点
    digitalWrite(feishui_chxi_dcf, OPEN_DRAIN); // 废水冲洗打开
    // feishui_chxi_hukong = false;
    // feishui_chxi_kshi_shke = millis(); // 废水冲洗开始时间点
    // 重置计时�??????
    state_str = "原水强冲打开";
    state_str.concat(ys_chxi_shch);
    state_str += " 秒";
    state_message = const_cast<char *>(state_str.c_str());
    display();
    feishui_chxi_state++; // 废水冲洗状态加1
  }
  if (feishui_chxi_state == 1) // 废水冲洗状态为1 关闭
  {

    if (millis() - feishui_chxi_kshi_shke > ys_chxi_shch * 1000) //
    {
      if (changjiu_qian_zhtai == 0) // 没有预存直接制水
      {
        if (work_state != zhishui_qch_close_n)
        {
          work_state = zhishui_qch_close_n; // 工作状态为制水中强冲
          upload("state", work_state);      // 上传工作状态
        }
      }
      if (changjiu_qian_zhtai == 10)
      {

        if (work_state != yucun_qch_close_n) // 工作状态预存原水强冲
        {
          work_state = yucun_qch_close_n; // 工作状态为预存强冲
          upload("state", work_state);    // 上传工作状态
        }
      }
      digitalWrite(feishui_chxi_dcf, 0); // 关闭废水冲洗电磁阀
      state_str = "原水冲洗关闭";
      state_str.concat(ys_chxi_gb_shch);
      state_str += " 分钟";
      state_message = const_cast<char *>(state_str.c_str());
      display();
      // metro0.reset();
      // metro0.interval(ys_chxi_gb_shch * 60000); // 废水冲洗间隔
      feishui_chxi_jieshu_shike = millis(); // 废水冲洗结束时间点
      feishui_chxi_state++;                 // 废水冲洗状态减1
    }
  }

  if (feishui_chxi_state == 2) // 废水冲洗状态为2 打开
  {

    if (millis() - feishui_chxi_jieshu_shike > ys_chxi_gb_shch * 60000) //
    {
      if (changjiu_qian_zhtai == 0) // 没有预存直接制水
      {
        if (work_state != zhishui_qch_open_n)
        {
          work_state = zhishui_qch_open_n; // 工作状态为制水中强冲
          upload("state", work_state);     // 上传工作状态
        }
      }
      if (changjiu_qian_zhtai == 10)
      {

        if (work_state != yucun_qch_open_n) // 工作状态预存原水强冲
        {
          work_state = yucun_qch_open_n; // 工作状态为预存强冲
          upload("state", work_state);   // 上传工作状态
        }
      }

      digitalWrite(feishui_chxi_dcf, OPEN_DRAIN); // 废水冲洗打开

      feishui_chxi_kshi_shke = millis(); // 废水冲洗开始时间点
      state_str = "原水强冲打开";
      state_str.concat(ys_chxi_shch);
      state_str += " 秒";
      state_message = const_cast<char *>(state_str.c_str());
      display();
      feishui_chxi_state--; //
    }
  }

  // 显示屏显�??????

  if (millis() - zhishui_xianshi_jiange > 10000 && pulse_flag == true) // 间隔显示,多次进入
  {

    if (digitalRead(feishui_chxi_dcf) == 1) // 废水冲洗打开
    {
      state_str = "原水强冲脉冲计数";
      state_str.concat(zs_cx_pulse);
      upload("zs_cx_pulse", zs_cx_pulse); // 上传废水冲洗脉冲数
    }
    else
    {
      if (changjiu_xian_zhtai == 10)
      {
        state_str = "预存脉冲计数";
      }
      else
      {
        state_str = "制水脉冲计数";
      }
      state_str.concat(zs_pulse);
      // upload("zs_pulse", zs_pulse); // 上传制水脉冲数
    }

    state_message = const_cast<char *>(state_str.c_str());
    if (changjiu_xian_zhtai == 10)
    {
      data_str = "预存水脉冲数标准：";
    }
    else
    {
      data_str = "正常制水脉冲数标准：";
    }

    data_str.concat(zs_pulse_bzh);
    data_message = const_cast<char *>(data_str.c_str());
    display();
    zs_upload();
    zhishui_xianshi_jiange = millis();
    pulse_flag = false; // 废水脉冲标志，表示已经显示了
  }

  // 废水测量，净水储存与制水�??????�??????
  if (pulse_flag == false)
  {
    liuliang_detect(1000); // 废水脉冲检测，单位�???秒，500�???秒�?�测一次废水脉冲数
    pulse_flag = true;     // 废水脉冲标志，表示废水脉冲数已经更新

    if (digitalRead(feishui_chxi_dcf) != 1) // 废水冲洗没打开
    {
      if (zs_pulse < zs_pulse_bzh)
      {
        feishui_guoshao_cishu++;
        data_str = "废水过少";
        data_str.concat(feishui_guoshao_cishu);
        data_str = data_str + "次";
        data_message = const_cast<char *>(data_str.c_str());
        display();
      }
      else
      {
        feishui_guoshao_ximo_cishu = 0; // 废水过少洗膜次数�??????0
        feishui_guoshao_cishu = 0;
      }
      /* if (feishui_guoshao_cishu >= zs_gs_cs_bzh)
      {
        state_message = "废水过少处理";
        display();
        feishui_guoshao_cishu = 0; // 废水过少次数�??????0,为下次制水准�??????
        zhishui_feishui_guoshao = true;
      }  */
    }
  }
  // �??????的是如果废水过少，实现尽�??????停机保护
}

bool yuanshui_chxi(bool _true)
{

  if (zhishui_hou_chxi_state == 0)
  {
    state_str = "原水强冲";
    state_str.concat(zsh_chxi_sch);
    state_str += " 秒";
    state_message = const_cast<char *>(state_str.c_str());
    display();
    if (zong_dcf_state == 0) // 总进水电磁阀关闭
    {
      openValve(zong_dcf);   // 打开总进水电磁阀
      zong_dcf_state = true; // 总进水电磁阀打开状态
    }
    openValve(jinshui_dcf);            // 打开泵前进水电磁阀
    openValve(js_dcf);                 // 打开净水电磁阀
    digitalWrite(chlv_chxi_dcf, 0);    // 关闭超滤冲洗电磁阀
                                       // openValve(zong_dcf); // 打开总进水电磁阀
    digitalWrite(feishui_chxi_dcf, 1); // 打开废水冲洗电磁阀

    if (digitalRead(zengya_beng) == 0) //
    {
      digitalWrite(zengya_beng, 1); //
    }
    if (work_state != zhishui_hou_chxi_open_n) // 工作状态制水废水过少
    {
      work_state = zhishui_hou_chxi_open_n; // 工作状态为预存洗膜状态
      upload("state", work_state);          // 上传工作状态
    }
    feishui_chxi_kshi_shke = millis(); // 记录原水冲洗开始时刻
    zhishui_hou_chxi_state++; // 变成1
  }

  if (millis() - feishui_chxi_kshi_shke > zsh_chxi_sch * 1000) // 变成秒
  {
    // zhishui_hou_chxi_state = 0;

    _true = false;
    state_message = "原水冲洗完成";
    display();
    digitalWrite(feishui_chxi_dcf, 0);          // 关闭废水冲洗电磁阀
    if (work_state != zhishui_hou_chxi_close_n) // 工作状态制水废水过少
    {
      work_state = zhishui_hou_chxi_close_n; // 工作状态为预存洗膜状态
      upload("state", work_state);           // 上传工作状态
    }
  }

  if (digitalRead(feishui_chxi_dcf) == 1)
  {
    liuliang_detect(500);

    data_str = "原水冲洗脉冲数：";

    data_str.concat(zs_pulse);

    data_message = const_cast<char *>(data_str.c_str());

    display();                          //
    upload("zs_cx_pulse", zs_cx_pulse); // 上传废水冲洗脉冲数
  }

  return _true;
}

bool chaolv_chongxi(bool _true)
{
  if (chlv_chxi_reason == 1)
  {

    state_str = "缺水冲洗超滤";
  }
  else if (chlv_chxi_reason == 2)
  {

    state_str = "累积冲洗超滤";
  }
  if (chlv_yi_chxi_cishu < chlv_chxi_cishu) // 冲洗次数控制
  {

    if (chlv_state_int == 0) // �??????进入一�??????,打开电�?�阀冲洗
    {
      begin_t = millis();
      chlv_state_int++;
      closeValve(zong_dcf);           // 关闭总进水电磁阀
      digitalWrite(chlv_chxi_dcf, 1); // 打开超滤冲洗电�?�阀

      state_str += "关闭总进水，冲洗超滤";
      state_str.concat(chlv_open_shch);
      state_str += " 秒";
      state_message = const_cast<char *>(state_str.c_str());
      display();
      if (work_state != 22) // 工作状态制水废水过少
      {
        work_state = 22;             // 工作状态为预存洗膜状�?
        upload("state", work_state); // 上传工作状态
      }
      // Serial.println(chlv_state_int);
    }
    if (chlv_state_int == 1)

    {
      if (millis() - begin_t > chlv_open_shch * 1000) // 冲洗后关�??????超滤，打开总进�??????
      {
        digitalWrite(chlv_chxi_dcf, 0); // 关闭超滤冲洗电�?�阀
        openValve(zong_dcf);            // 打开总进水电磁阀
        state_str += "打开总进水，关闭超滤冲洗";
        state_str.concat(chlv_close_shch);
        state_str += " 秒";
        state_message = const_cast<char *>(state_str.c_str());
        display();

        begin_t = millis(); // 超滤停止时刻
        chlv_state_int++;
        if (work_state != 24) // 工作状态制水废水过少
        {
          work_state = 24;             // 工作状态为预存洗膜状�?
          upload("state", work_state); // 上传工作状态
        }
        // Serial.println(chlv_state_int);
      }
    }
    if (millis() - begin_t > chlv_close_shch * 1000 && chlv_state_int == 2) // 等待时长后重新开�??????
    {
      chlv_yi_chxi_cishu++;
      // begin_t=millis();
      chlv_state_int = 0;
      // Serial.println(chlv_state_int);
    }
    if (digitalRead(chlv_chxi_dcf) == 1)
    {
      liuliang_detect(1000);
      data_str = "超滤冲洗脉冲数：";

      data_str.concat(chlv_pulse);

      data_message = const_cast<char *>(data_str.c_str());

      display();                        // 显示水满时间
      upload("chlv_pulse", chlv_pulse); // 上传超滤冲洗脉冲数
    } // 超滤冲洗时废水流量�?��?
  }
  else
  {
    state_message = "超滤冲洗完成";
    display();
    _true = false;
    chlv_yi_chxi_cishu = 0;
    chlv_state_int = 0;
  }
  return _true;
}

void display()
{
  u8g2.clearBuffer(); // clear the internal memory

  int charCount = countChineseCharacters(state_message);
  if (charCount > 8)
  { // 分割字�?�串
    splitChineseString(state_message, state_1, state_2, 8);
    u8g2.drawUTF8(0, 16, state_1);
    u8g2.drawUTF8(0, 32, state_2);
  }
  else
  {
    u8g2.drawUTF8(0, 16, state_message);
  }
  charCount = countChineseCharacters(data_message);
  if (charCount > 8)
  { // 分割字�?�串
    splitChineseString(data_message, data_1, data_2, 8);
    u8g2.drawUTF8(0, 48, data_1);
    u8g2.drawUTF8(0, 64, data_2);
  }
  else
  {
    u8g2.drawUTF8(0, 48, data_message);
  }
  u8g2.sendBuffer();
    /* if ((String)pre_state_message != (String)state_message)
  {
    Serial.println("上传状态变化");
    upload("state_display", state_message);// 上传显示内容到onenet
  }
  if ((String)pre_data_message != (String)data_message)
  {Serial.println("上传数据变化");
    upload("data_display", data_message);// 上传显示内容到onenet
  }  
   

   pre_state_message = state_message;
   pre_data_message = data_message;
 */
   upload("state_display", state_message);// 上传显示内容到onenet
    upload("data_display", data_message);// 上传显示内容到onenet


  delay(1000);
}
// 计算字�?�串�??????的汉�??????/字�?�数�??????
int countChineseCharacters(const char *str)
{
  int count = 0;
  int i = 0;

  while (str[i] != '\0')
  {
    int charLen = getUtf8CharLength(str[i]);
    i += charLen;
    count++;
  }

  return count;
}
// =================== WiFi连接 ===================
void WiFi_Connect()
{
  WiFi.begin(s_ssid, s_password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 15)
  {
    delay(500);
    Serial.print(".");
    retry++;
  }
  state_message = (WiFi.status() == WL_CONNECTED) ? "wifi已连接" : "wifi不能连接";
  display();
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected");
  }
  else
  {
    Serial.println("WiFi connection failed");
  }
}

void printRunningPartition()
{
  const esp_partition_t *running = esp_ota_get_running_partition();
  Serial.print("Running partition label: ");
  Serial.println(running->label);
}

void printRunningPartitionInfo()
{
  // 获取当前运行的分区
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (running == NULL)
  {
    Serial.println("Error: Failed to get running partition!");
    return;
  }

  // 打印分区详细信息
  Serial.println("\n===== Current Running Partition Info =====");
  Serial.printf("Label:    %s\n", running->label ? running->label : "NULL");
  partionName = running->label ? String(running->label) : "NULL";
  state_str = "运行分区：" + partionName;

  state_message = const_cast<char *>(state_str.c_str());
  data_str = "现版本：" + String(version);
  data_message = const_cast<char *>(data_str.c_str());
  display();
  Serial.printf("Type:     %s\n", running->type == ESP_PARTITION_TYPE_APP ? "APP" : "DATA");

  Serial.printf("SubType:  %d (0x%02X)\n", running->subtype, running->subtype);

  Serial.printf("Address:  0x%08X\n", running->address);
  Serial.printf("Size:     %d bytes (%.2f MB)\n", running->size, (float)running->size / (1024 * 1024));
  Serial.println("=======================================");
  // 打印类似如下：
  //   ===== Current Running Partition Info =====
  // Label:    app0
  // Type:     APP
  // SubType:  16 (0x10)//ESP_PARTITION_SUBTYPE_APP_OTA_MIN = 0x10, //!< Base for OTA partition subtypes
  // Address:  0x00010000
  // Size:     1572864 bytes (1.50 MB)
  // =======================================

  /* // 解释子类型（OTA 相关）
  if (running->type == ESP_PARTITION_TYPE_APP)
  {
    Serial.println("\nSubType Explanation:");
    Serial.println("  0x10 (16): Factory");
    Serial.println("  0x11 (17): OTA_0");
    Serial.println("  0x12 (18): OTA_1");
    Serial.println("  0x20 (32): Test");
  } */
}
// 计算UTF-8字�?�长�??????
int getUtf8CharLength(byte firstByte)
{
  if (firstByte < 0x80)
  {
    return 1; // ASCII字�??
  }
  else if ((firstByte & 0xE0) == 0xC0)
  {
    return 2; // 2字节字�??
  }
  else if ((firstByte & 0xF0) == 0xE0)
  {
    return 3; // 3字节字�?�（包括汉字�??????
  }
  else if ((firstByte & 0xF8) == 0xF0)
  {
    return 4; // 4字节字�??
  }
  return 1; // 默�?�情�??????
}
// 分割汉字字�?�串
void splitChineseString(const char *input, char *part1, char *part2, int charCount)
{
  int charIndex = 0;
  int i = 0;
  int part1Index = 0;

  // 复制前charCount�??????字�?�到part1
  while (charIndex < charCount && input[i] != '\0')
  {
    int charLen = getUtf8CharLength(input[i]);

    // 复制整个字�??
    for (int j = 0; j < charLen; j++)
    {
      part1[part1Index++] = input[i++];
    }

    charIndex++;
  }
  part1[part1Index] = '\0'; // 添加终�?��??

  // 复制剩余字�?�到part2
  int part2Index = 0;
  while (input[i] != '\0')
  {
    part2[part2Index++] = input[i++];
  }
  part2[part2Index] = '\0'; // 添加终�?��??
}

void checkForUpdates()
{
  if (chanpin_lei == nullptr)
  {

    // 获取version.json
    versionUrl = "http://lyd50.vicp.io/update1/version.txt"; // 替换为实际的版本文件URL
  }
  else if (chanpin_lei == "xiaoshangyongzuji")
  {

    versionUrl = "http://lyd50.vicp.io/update_zuji/version.txt";
  }

  HTTPClient http;
  http.begin(versionUrl);
  int httpCode = http.GET();
  Serial.println("检查更新，HTTP状态码: " + String(httpCode));
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    Serial.println("获取到的版本信息: " + payload);
    // 解析JSON
    JsonDocument doc;
    deserializeJson(doc, payload);

    uint32_t latestVer = doc["latest_version"].as<uint32_t>();
    Serial.println("最新版本: " + String(latestVer));
    Serial.println("当前版本: " + String(version));
    if (latestVer > version)
    {
      Serial.println("发现新版本: " + String(latestVer));
      state_message = "发现新版本，下载 更新";
      display();
      String firmwareUrl = doc["firmware_url"].as<String>();
      downloadAndUpdate(firmwareUrl);
    }
  }
  http.end();
}

void downloadAndUpdate(String firmwareUrl)
{
  HTTPClient http;
  http.begin(firmwareUrl);

  int httpCode = http.GET();
  Serial.println("下载固件，HTTP状态码: " + String(httpCode));
  if (httpCode == HTTP_CODE_OK)
  {
    int contentLength = http.getSize();
    WiFiClient *stream = http.getStreamPtr();

    if (Update.begin(contentLength))
    {
      size_t written = Update.writeStream(*stream);
      if (written == contentLength)
      {
        if (Update.end(true))
        {
          Serial.println("OTA更新成功，即将重启");
          state_message = "OTA更新成功，即将重启";
          display();
          ESP.restart();
        }
      }
    }
  }
  http.end();
}
