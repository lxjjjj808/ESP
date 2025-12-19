/* 2025.6.8
带压力桶净水器   双高压开关  高压开关防抖，有总进水电磁阀 外压超滤
 1:故障停机  3 预存不能水满  4 制水超时 5：制水废水太少   6:停水停机 7：超滤后短暂缺水  10：预存 20：制水  30  水满  40：洗膜后水满
  15：发送onenet数据 16：接收onenet数据   22：超滤冲洗 24：关闭超滤冲洗，打开总进
  25：制水中废水冲洗启动 26：制水中废水冲洗停止 28：制水中强冲开始  35：原水强冲洗
*/
// 2025.7.30 手机网页按键，收到了在预存和之水
// 2025.9.17 所有设备使用产品级api_key,同产品下密码一样
// 2025.9.22 刚开机等待2分钟进水再制水
//2025.12.19 改为售水机程序，水满由高水位开关控制，增加手机端水满后制水延时调节，弃用净水出单向阀
#include <WiFi.h>
#include "canshu.h"
#include <Arduino.h>
#include "esp_task_wdt.h"
#include "Lyd.h"
#include <Wire.h>
#include <ArduinoJson.h>
#include <rom/rtc.h>
#include "esp_ota_ops.h"
#include <nvs_flash.h>
#include "RTClib.h"
#include "esp_wifi.h"
RTC_DS3231 rtc;

void setup(void)
{
  Serial.begin(9600);
  delay(1000); // 等待串口稳定
  /*打印出flash 大小*/
  // uint32_t flash_size = ESP.getFlashChipSize(); // 获取 Flash 大小（字节）
  // Serial.printf("Flash Size: %u bytes (%.1f MB)\n", flash_size, flash_size / (1024.0 * 1024));
  pinMode(zengya_beng, OUTPUT_OPEN_DRAIN);
  digitalWrite(zengya_beng, LOW); //   zengya_beng
  Wire.begin();
  // switchToOta0AndReboot() ;

  pinMode(zong_dcf, OUTPUT);
  pinMode(chlv_chxi_dcf, OUTPUT);

  pinMode(jinshui_dcf, OUTPUT);
  pinMode(js_dcf, OUTPUT);
  pinMode(feishui_chxi_dcf, OUTPUT);
  pinMode(fdjr, OUTPUT);
  pinMode(fengmingqi, OUTPUT);

  pinMode(chlv_qian_diya_kg, INPUT);
  pinMode(chlv_hou_diya_kg, INPUT);
  pinMode(yucun_kg, INPUT);
  pinMode(ylt_kg, INPUT);

  pinMode(shuiliu_chuanganqi, INPUT); // 废水水流传感�?????? 12
  pinMode(cewen, INPUT);              // 测温传感�???

  ledcSetup(CHANNEL0, PWM_FREQ, RESOLUTION);
  ledcSetup(CHANNEL1, PWM_FREQ, RESOLUTION);
  ledcSetup(CHANNEL2, PWM_FREQ, RESOLUTION);
  // 附加PWM通道到GPIO引脚
  ledcAttachPin(zong_dcf, CHANNEL0);
  ledcAttachPin(jinshui_dcf, CHANNEL1);
  ledcAttachPin(js_dcf, CHANNEL2);

  openValve(zong_dcf); // 默认总进水电磁阀打开
  zong_dcf_state = true;
  digitalWrite(chlv_chxi_dcf, LOW);    // chlv_chxi_dcf
  closeValve(jinshui_dcf);             // 泵前进水电磁阀
  closeValve(js_dcf);                  // js_dcf
  digitalWrite(feishui_chxi_dcf, LOW); // feishui_chxi_dcf
  digitalWrite(fdjr, LOW);             // 防冻加热
  digitalWrite(fengmingqi, HIGH);      // 蜂鸣器置高，关闭
  digitalWrite(cewen, INPUT_PULLDOWN); // 测温传感器置高，防�?��??触发

  // digitalWrite(shuiliu_chuanganqi, INPUT_PULLUP); // 废水水流传感器置高，防�?��??触发
  // digitalWrite(ylt_kg, INPUT_PULLDOWN);                // ylt_kg
  // digitalWrite(yucun_kg, INPUT_PULLDOWN);                // yucun_kg
  // digitalWrite(chlv_hou_diya_kg, INPUT_PULLDOWN);                // 超滤后低压开�????
  // digitalWrite(chlv_qian_diya_kg, INPUT_PULLDOWN); // 13 超滤前低压开�????

  u8g2.begin();
  u8g2.setFont(u8g2_font_wqy16_t_gb2312); // choose a suitable font
  state_message="售水机水箱超滤自动冲洗v20251219";
  data_message="售后16638931971";
  display();

  pre_read(); // 读取eeprom参数
  // yiwai_defaut();

  WiFi.mode(WIFI_AP_STA);
  // 设置发射功率（单位：dBm）
  esp_wifi_set_max_tx_power(84); // 84对应20.5dBm（最大值）
  // 设置WiFi参数
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G); // 仅使用802.11b/g，增加兼容性

  // 设置带宽（20MHz比40MHz传输距离更远）
  esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);

  WiFi.softAP("esp-captive", "19711128"); // 设置AP模式的SSID和密码
  WiFi_Connect();

  // HTTP服务器路由
  server.enableCORS(true);
  server.on("/chaolv", HTTP_GET, send_chlv_data);
  server.on("/chaolv", HTTP_POST, receive_chlv_data);

  server.on("/wifi", HTTP_GET, send_wifi_data);
  server.on("/wifi", HTTP_POST, receive_wifi_data);

  server.on("/onenet", HTTP_GET, send_onenet_data);
  server.on("/onenet", HTTP_POST, receive_onenet_data);

  server.on("/zs", HTTP_GET, send_zs_data); // 制水数据 包含洗膜参数
  server.on("/zs", HTTP_POST, receive_zs_data);

  server.on("/product", HTTP_GET, send_product_data);
  server.on("/product", HTTP_POST, receive_product_data); // 接收产品信息
  server.on("/ota", HTTP_GET, send_ota_data);
  server.on("/ota", HTTP_POST, receive_ota_data); // 接收OTA更新数据
  server.on("/scan", HTTP_GET, send_wifi_list);
  // server.on("/ximo", HTTP_POST, shtmo_chxi); // 处理渗透膜洗膜
  server.begin();

  // 构建主题字符串
  snprintf(data_topic, sizeof(data_topic), data_topic_template, s_product_id, s_device_id);                         // 设备属性上报主题
  snprintf(data_reply_topic, sizeof(data_reply_topic), data_topic_reply_template, s_product_id, s_device_id);       // 设备属性上报响应主题
  snprintf(cmd_topic, sizeof(cmd_topic), cmd_topic_template, s_product_id, s_device_id);                            // 订阅设备命令主题
  snprintf(cmd_response_topic, sizeof(cmd_response_topic), cmd_response_topic_template, s_product_id, s_device_id); // 设备命令响应主题
  snprintf(set_topic, sizeof(set_topic), set_topic_template, s_product_id, s_device_id);
  snprintf(set_reply_topic, sizeof(set_reply_topic), set_reply_topic_template, s_product_id, s_device_id); // 设备属性设置主题
  // 设置MQTT服务器
  // client.setServer(mqtt_server, mqtt_port);
  if (WiFi.status() == WL_CONNECTED)
  {

    OneNet_Connect();
  }

  delay(1000);
  client.setCallback(callback);
  /* // 初始化 NVS（必需 for OTA 功能）
      esp_err_t ret = nvs_flash_init();
      if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
          ESP_ERROR_CHECK(nvs_flash_erase());
          ret = nvs_flash_init();
      }
      ESP_ERROR_CHECK(ret); */
  // printRunningPartition();
  printRunningPartitionInfo();

  

  if (!rtc.begin())
  {
    Serial.println("找不到RTC模块");
  }
  /*  if (rtc.lostPower())
   {
     Serial.println("RTC失去电力，设置时间");
     // 设置为编译时间
     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
     // 或者手动设置时间
     // rtc.adjust(DateTime(2023, 1, 1, 12, 0, 0));
   }
   DateTime now = rtc.now();

   Serial.print("当前时间: ");
   Serial.print(now.year(), DEC);
   Serial.print('/');
   Serial.print(now.month(), DEC);
   Serial.print('/');
   Serial.print(now.day(), DEC);
   Serial.print(" ");
   Serial.print(now.hour(), DEC);
   Serial.print(':');
   Serial.print(now.minute(), DEC);
   Serial.print(':');
   Serial.print(now.second(), DEC);
   Serial.println();

   float temp = rtc.getTemperature();

   // 或者直接读取寄存器值（整数）
   // int8_t temp = rtc.getTemperatureAsByte();

   Serial.print("温度: ");
   Serial.print(temp);
   Serial.println(" °C"); */
  // 如果RTC失去电力，设置时间（只需执行一次）
}
/* void loop(void)
{
  Serial.println("loop running...");
     delay(1000);

   printRunningPartitionInfo();
   delay(1000);
  // ... (rest of the loop code)
} */

void loop(void)
{
  /* if (millis() - gang_qidong_shike > 60000 && gang_qidong_shike == 0)//60秒后记录刚启动时刻
  {
    gang_qidong_shike = millis();
  }  */

  if (WiFi.status() == WL_CONNECTED) // 检查OTA更新
  {
    if ((millis() - lastUpdateTime > 600000 || lastUpdateTime == 0) && (changjiu_xian_zhtai == 40 || work_state < 10)) // 每10分钟检查一次更新
    {                                                                                                                  // 每小时检查一次
      Serial.println("Checking for updates...");
      checkForUpdates();
      upload("version", version); // 上传版本号
      lastUpdateTime = millis();
    }
  }

  if (changjiu_xian_zhtai != 10 && changjiu_xian_zhtai != 20 && changjiu_xian_zhtai != 30) // 不是预存和制水状态才检查WiFi连接
  {                                                                                        // Serial.println(changjiu_xian_zhtai);
    // Serial.println("Checking WiFi connection...");

    if (wifi_metro.check() == 1) //
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        WiFi_Connect();
        if (WiFi.status() != WL_CONNECTED)
        {
          wifi_fail_cishu++;
          if (wifi_fail_cishu > 20)
          {
            wifi_fail_cishu = 20;
          }
          wifi_cg_cishu = 0;
          wifi_metro.interval(wifi_jg_shch * 1000 * wifi_fail_cishu);
        }
      }
      else
      {
        wifi_cg_cishu++;
        if (wifi_cg_cishu > 20)
        {
          wifi_cg_cishu = 20;
        }
        wifi_metro.interval(wifi_jg_shch * 1000 * wifi_cg_cishu);
        wifi_fail_cishu = 0;
        if (!client.connected())
        {
          OneNet_Connect();
        }
        else
        {
          if (send_reset_reason == false)
          {
            print_reset_reason();     // 打印上次重启原因
            send_reset_reason = true; // 只发送一次重启原因
          }
        }
      }
    }
  }

  client.loop(); // 保持MQTT连接
  server.handleClient();
  if (zhishui_chaoshi_cishu >= chaoshi_cishu_bzh || yucun_buneng_shuiman_cishu >= yucun_buneng_shuiman_cish_bzhun || feishui_guoshao_ximo_cishu == 2 || loushui_cishu >= 2) // 超时次数超过阈�?
  {
    if (zong_dcf_state == true) // 工作状态报警停机
    {
      closeValve(zong_dcf); // 关闭总进水电磁阀
      zong_dcf_state = false;
      closeValve(jinshui_dcf);           // 关闭泵前进水电�?�阀
      digitalWrite(zengya_beng, 0);      // 关闭增压�??????
      digitalWrite(feishui_chxi_dcf, 0); // 关闭废水冲洗电�?�阀
      digitalWrite(chlv_chxi_dcf, 0);    // 关闭超滤冲洗电�?�阀
      closeValve(js_dcf);                // 关闭净水电磁阀
      // work_state = guzhang_tingji_n;    // 工作状态为故障停机
      // upload("state", work_state);       // 上传工作状态
    }

    baojing = true; // 报�??
    if (baojing_qishi_shike == 0)
    {
      baojing_qishi_shike = millis();
    }
    if (millis() - alarm_jiange > 10000) // �??????5秒报警一�??????
    {
      if (zhishui_chaoshi_cishu >= chaoshi_cishu_bzh)
      {
        work_state = zs_dci_chshi_n; // 工作状态为制水多次超时
        state_message = "制水多次超时，重启查明故障";
      }
      if (yucun_buneng_shuiman_cishu >= yucun_buneng_shuiman_cish_bzhun)
      {
        work_state = yc_dci_bn_sm_n; // 工作状态为预存多次不能水满
        state_message = "预存多次不能水满，重启查明故障";
      }
      if (feishui_guoshao_ximo_cishu == 2)
      {
        work_state = feish_dci_guosh_n; // 工作状态为废水多次过少
        state_message = "废水过少洗膜多次，重启查明故障";
      }
      if (loushui_cishu >= 2)
      {
        work_state = loushui_dci_n; // 工作状态为漏水多次
        state_message = "漏水严重，重启查明故障";
      }
      upload("state", work_state); // 上传工作状态
      data_str = "故障停机";
      int i = (millis() - baojing_qishi_shike) / 60000; // 已水满分钟数
      data_str.concat(i);
      data_str += " 分钟";
      data_message = const_cast<char *>(data_str.c_str());

      display(); //
      alarm_jiange = millis();
    }
    return; // 故障时不再执行后续流程
  }

  if (yucun_buneng_shuiman == true || zhishui_chaoshi == true || zhishui_feishui_guoshao == true || chlv_xuyao_chxi == true) // 预存不能水满或制水超时或废水过少
  {
    if ( chlv_xuyao_chxi==1) //
    {

      if (digitalRead(zengya_beng) == 1) // 增压泵打开
      {
        digitalWrite(zengya_beng, 0);      // 关闭增压�?????
        closeValve(jinshui_dcf);           // 关闭泵前进水电�?�阀
        closeValve(js_dcf);                // 关闭净水电磁阀
        digitalWrite(feishui_chxi_dcf, 0); // 关闭废水电�?�阀
      }
      chlv_xuyao_chxi = chaolv_chongxi(chlv_xuyao_chxi);
    }
    if (yucun_buneng_shuiman == true)
    {
      if (work_state != yucun_buneng_shuiman_n) // 工作状态为预存不能水满
      {
        work_state = yucun_buneng_shuiman_n; // 工作状态为预存不能水满
        upload("state", work_state);         // 上传工作状态
      }
      if (millis() - alarm_jiange > 5000) // �??????5秒报警一�??????
      {
        state_message = "预存不能水满";
        display();
        alarm_jiange = millis();
      }
      yucun_buneng_shuiman = yuanshui_chxi(yucun_buneng_shuiman); // 预存不能水满
      if (yucun_buneng_shuiman == false)
      {
        jishi_dangqian_zhtai = 0;
        jishi_yiqian_zhtai = 0;
        changjiu_xian_zhtai = 0;
        changjiu_qian_zhtai = 0;
        yucun_buneng_shuiman_cishu++; // 预存洗膜不能水满次数�??????1
        state_str = "预存洗膜水不能水满冲洗";

        state_str.concat(yucun_buneng_shuiman_cishu);
        state_str += "次";
        data_message = const_cast<char *>(state_str.c_str());

        display(); //
      }
    }
    if (zhishui_chaoshi == true)
    {
      if(baohu_zht==0)//防止重复执行
      {
        
         if (work_state != zhishui_chao_shijian_n) // 工作状态为制水超时
      {
        work_state = zhishui_chao_shijian_n; // 工作状态为制水超时
        upload("state", work_state);         // 上传工作状态
      
      }
      baohu_zht++;
      }

     
      if (millis() - alarm_jiange > 5000) // �??????5秒报警一�??????
      {
        data_str = "制水超时了";

        data_str.concat(zhishui_chaoshi_cishu);
        data_str += " 次";
        state_message = const_cast<char *>(data_str.c_str());

        // state_message = "制水超时";

        display();
        alarm_jiange = millis();
      }

      if (baohu_zht == 1)
      {

       if( yuanshui_chxi(1)==0) // 原水冲洗

       { baohu_zht++;}
      }

      if (baohu_zht == 2)             // 原水冲洗完成后
      {                               // 停泵才能冲洗渗透膜
        digitalWrite(zengya_beng, 0); // 关闭增压泵
        closeValve(jinshui_dcf);      // 关闭泵前进水电磁阀
        closeValve(js_dcf);           // 关闭净水电磁阀
        digitalWrite(feishui_chxi_dcf, 0); // 关闭废水电磁阀
        baohu_zht++;
      }

      if (baohu_zht == 3)
      {
        chaolv_chxi_panduan((millis() - zhishui_kaishi_shke) / 60000); // 决定是否需要冲洗超滤膜
        baohu_zht++;
      }

      if (baohu_zht == 4)
      {
        if (digitalRead(yucun_kg) == 0) // 原水冲洗后等待洗膜开关闭合
        {
          jishi_dangqian_zhtai = 0;
          jishi_yiqian_zhtai = 0;
          changjiu_xian_zhtai = 0;
          changjiu_qian_zhtai = 0;
          data_str = "制水超时了";

          data_str.concat(zhishui_chaoshi_cishu);
          data_str += " 次，继续制水";
          data_message = const_cast<char *>(data_str.c_str());

          display(); //
          zhishui_chaoshi = false;
        }
        else
        {
          state_message = "制水超时，等待洗膜完成";
          display();
        }
      }
    }
    if (zhishui_feishui_guoshao == true)
    {
      if (work_state != zhishui_feishui_guoshao_n) // 工作状态制水废水过少
      {
        work_state = zhishui_feishui_guoshao_n; // 工作状态为制水废水过少
        upload("state", work_state);            // 上传工作状态
      }
      if (millis() - alarm_jiange > 5000) // �??????5秒报警一�??????
      {
        state_message = "制水时废水过少";
        display();
        alarm_jiange = millis();
      }
      // zhishui_feishui_guoshao = yuanshui_chxi(zhishui_feishui_guoshao); // 废水过少

      if (digitalRead(zengya_beng) == 1) // 增压泵打开
      {
        digitalWrite(zengya_beng, 0);      // 关闭增压�?????
        closeValve(jinshui_dcf);           // 关闭泵前进水电�?�阀
        closeValve(js_dcf);                // 关闭净水电磁阀
        digitalWrite(feishui_chxi_dcf, 0); // 关闭废水电�?�阀
      }

      zhishui_feishui_guoshao = chaolv_chongxi(zhishui_feishui_guoshao);

      if (zhishui_feishui_guoshao == false)

      {
        if (zhishui_feishui_guoshao == false)
        {
          jishi_dangqian_zhtai = 0;
          jishi_yiqian_zhtai = 0;
          changjiu_xian_zhtai = 0;
          changjiu_qian_zhtai = 0;
        }
        feishui_guoshao_ximo_cishu++;
        state_str = "制水时废水过少已冲洗超滤";

        // state_str.concat(feishui_guoshao_ximo_cishu);
        // state_str += "次";
        // data_message = const_cast<char *>(state_str.c_str());

        display(); //
      }
    }

    return; // 故障时不再执行后续流程
  }
  /*  if (gang_qidong_shike == 0)//配合刚启动等待2分钟进水再制水
   {

       return;
     } */

  // 状态判定与切换
  int ylt_val = digitalRead(ylt_kg);
  int yucun_val = digitalRead(yucun_kg);
  if (ylt_val == LOW)
  {
    jishi_dangqian_zhtai = (yucun_val == LOW) ? 10 : 20;
  }
  else
  {
    jishi_dangqian_zhtai = (yucun_val == HIGH) ? 30 : 40;
  }

  // 即时状态变化判�??????
  if (jishi_yiqian_zhtai != jishi_dangqian_zhtai) //
  {                                               // 状态变化一次，延时赋值最新时�??????
    jishi_yiqian_zhtai = jishi_dangqian_zhtai;
    zhtai_yanshi_kaishi_shike = millis();
  }

  if (jishi_dangqian_zhtai != changjiu_xian_zhtai) //
  {
    {

      if (millis() - zhtai_yanshi_kaishi_shike > 2000) // 稳定�??????2秒后进入新状�??????
      {
        changjiu_qian_zhtai = changjiu_xian_zhtai;  // 赋值前�??????
        changjiu_xian_zhtai = jishi_dangqian_zhtai; // 长久现状�??????=即时当前状�?
        qiantai_jin_shike = xiantai_jin_shike;      // 赋值前状态进入时�??????
        xiantai_jin_shike = millis();               // 现状态进入状态时�??????
      }
    }
  }

  switch (changjiu_xian_zhtai) // 根据长久现状态判�??????
  {
  case 10:
    if ((millis() - zyb_tzhi_shike > 180000) && zyb_tzhi_shike != 0) // 不是刚上机 增压泵停止3分钟后才允许再次启动，防止频繁启动增压泵
    {
      yucun_ximo(); // 预存洗膜水状态定10，�?�存洗膜水满后�?�常制水�??????20. 制水�??????水�?�水满定30  水满后�?�存净水洗膜完成后�??????40
    }
    else if (zyb_tzhi_shike == 0 && millis() - gang_qidong_shike > 60000) // 第一次启动，等待1分钟进水时间
    {
      yucun_ximo(); // 预存洗膜水状态定10，�?�存洗膜水满后�?�常制水�??????20. 制水�??????水�?�水满定30  水满后�?�存净水洗膜完成后�??????40
    }
    break;
  case 20:

    zhch_zhishui(); // �??????水�?�高位开关闭合，洗膜高压开关断开
    break;
  case 30: // 预存水高压开关和洗膜高压开关都断开，纯水水满，预存洗膜水水满/*
    zhishui_shuiman();
    break;
  case 40: // 预存洗膜水洗膜完成后洗膜开关闭合
    yucun_ximo_wancheng();
    break;
  case 0:
    state_message = "刚启动,等待进水";
    display();
    break;
  default:
    state_message = "未知状态";
    display();
    break;
  }

  if (zong_dcf_state == true) // 总进水电磁阀打开状态下且不是刚启动
  {

    // 水压不正常后的恢复
    if (digitalRead(chlv_qian_diya_kg) == 0 && digitalRead(chlv_hou_diya_kg) == 0 && shuiya_zhch == false) // 超滤前后低压开关都�??????合了�??????
    {                                                                                                      // 设�?? 水压正常防抖
      if (shuiya_zhch_shike == 0)
      {
        shuiya_zhch_shike = millis();
      }
      if (millis() - shuiya_zhch_shike > 5000)
      {
        chaolv_hou_queshui_shike = 0; // 复位
        chaolv_qian_queshui_shike = 0;
        shuiya_zhch = true;
        shuiya_zhch_shike = 0; // 复位
        state_message = "水压正常";
        display();
        upload("state", shuiya_zhch_n);                             // 上传水压正常状态
        if (changjiu_xian_zhtai == 10 || changjiu_xian_zhtai == 20) // 预存洗膜水状态或制水状�?
        {
          changjiu_xian_zhtai = 0;
        }
        // 在等待时间内任何一个低压开关有变化，赋值最新时间继�??????等待
        if (digitalRead(chlv_qian_diya_kg) == 1 || digitalRead(chlv_hou_diya_kg) == 1)
        {
          shuiya_zhch_shike = millis();
        }
      }
    }
    // 水源停水
    if (digitalRead(chlv_qian_diya_kg) == 1 && digitalRead(chlv_chxi_dcf) == 0 && digitalRead(feishui_chxi_dcf) == 0) // 冲洗状态下不检查
    {
      if (chaolv_qian_queshui_shike == 0)
      {
        state_message = "超滤前开关短暂断开";
        display();
        if (work_state != chaolv_qian_dz_queshui_n) // 工作状态不为超滤前短暂缺水和超滤前长久缺水
        {
          work_state = chaolv_qian_dz_queshui_n; // 工作状态为超滤前短暂缺水
          upload("state", work_state);           // 上传工作状态
        }
        chaolv_qian_queshui_shike = millis();
      }
      if (millis() - chaolv_qian_queshui_shike > 30000) // 防止短时缺水
      {
        // Serial.println("水源停水2");
        shuiya_zhch = false;
        closeValve(zong_dcf);
        zong_dcf_state = false; // 关闭总进水电磁阀,程序不再進入
        state_message = "水源停水，关闭总进水电磁阀";

        display();
        zong_dcf_gb_shike = millis();              // 记录关闭总进水电磁阀的时间
        upload("state", chaolv_qian_cj_queshui_n); // 上传工作状态
      }
    }
    else
    {
      chaolv_qian_queshui_shike = 0; // 复位
    }

    // 超滤后供水不足
    if (digitalRead(chlv_qian_diya_kg) == 0 && digitalRead(chlv_hou_diya_kg) == 1 && digitalRead(feishui_chxi_dcf) == 0) // 超滤前低压开关闭合，超滤后低压开关断开 不是冲洗状态
    {
      state_message = "超滤后开关短暂断开";
      display();
      if (work_state != chlv_hou_dz_queshui_n && work_state != chlv_hou_cj_queshui_n) // 工作状态不为超滤后短暂缺水和超滤后长久缺水
      {
        work_state = chlv_hou_dz_queshui_n; // 工作状态为超滤后短暂缺水
        upload("state", work_state);        // 上传工作状态

        chaolv_hou_queshui_shike = millis();
      }
      if (millis() - chaolv_hou_queshui_shike > 6000)
      {
        chlv_chxi_reason = 1;
        shuiya_zhch = false;
        chlv_xuyao_chxi = true;
        state_message = "超滤堵，需要冲洗";
        display();
        chaolv_hou_queshui_shike = millis();    //
        upload("state", chlv_hou_cj_queshui_n); // 上传工作状态
      }
    }
  }
  else
  {
    if ((changjiu_xian_zhtai != 30 && changjiu_xian_zhtai != 40) && shuiya_zhch == false) // 水满不打开
    {
      // chaolv_qian_queshui_shike = 0;
      if (millis() - zong_dcf_gb_shike > 300000) // 关闭5分钟打开
      {

        openValve(zong_dcf);
        zong_dcf_state = true; // 打开总进水电磁阀
        state_message = "持续缺水后打开总进水电磁阀了";
      }
      else
      {
        if (millis() - queshui_xianshi_jiange > 5000)
        {
          upload("state", guanfa_tji_n); // 上传工作状态
          queshui_xianshi_jiange = millis();
          state_message = "持续缺水关闭总进水电磁阀了";
          // display();
          data_str = "剩";
          int i = ((300000 + zong_dcf_gb_shike) - millis()) / 60000; // 已水满分钟数
          data_str.concat(i);
          data_str += "分钟打开总进水电磁阀";
          data_message = const_cast<char *>(data_str.c_str());

          display();
        }
      }
    }
  }
}

/* sensors.requestTemperatures(); // Send the command to get temperatures
 wendu = sensors.getTempC(address_18b20);
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(wendu);
   delay(3000);*/
