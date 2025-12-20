#include "Lyd.h"
#include <ArduinoJson.h>

void setupPins() {
  pinMode(PIN_ZENGYA_BENG, OUTPUT);
  digitalWrite(PIN_ZENGYA_BENG, LOW); // 开机默认低电平，防止增压泵误启动

  pinMode(PIN_ZONG_JINSHUI_DCF, OUTPUT);
  pinMode(PIN_CHALV_CHONGXI_DCF, OUTPUT);
  pinMode(PIN_BENG_QIAN_JINSHUI_DCF, OUTPUT);
  pinMode(PIN_JINSHUI_DCF, OUTPUT);
  pinMode(PIN_FEISHUI_CHONGXI_DCF, OUTPUT);
  pinMode(PIN_FANGDONG_JIARE, OUTPUT);

  pinMode(PIN_CHALV_QIAN_DIYA, INPUT);
  pinMode(PIN_CHALV_HOU_DIYA, INPUT);
  pinMode(PIN_JINGSHUI_GAOYA, INPUT);
  pinMode(PIN_JINGSHUI_DIYA, INPUT);
  pinMode(PIN_GAOYA_KG, INPUT);
  pinMode(PIN_LIULIANG, INPUT);

  digitalWrite(PIN_ZONG_JINSHUI_DCF, LOW);
  digitalWrite(PIN_CHALV_CHONGXI_DCF, LOW);
  digitalWrite(PIN_BENG_QIAN_JINSHUI_DCF, LOW);
  digitalWrite(PIN_JINSHUI_DCF, LOW);
  digitalWrite(PIN_FEISHUI_CHONGXI_DCF, LOW);
  digitalWrite(PIN_FANGDONG_JIARE, LOW);
}

void setupPwm() {
  ledcSetup(PWM_CHANNEL_ZONG, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_BENG_QIAN, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_JINSHUI, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_ZONG_JINSHUI_DCF, PWM_CHANNEL_ZONG);
  ledcAttachPin(PIN_BENG_QIAN_JINSHUI_DCF, PWM_CHANNEL_BENG_QIAN);
  ledcAttachPin(PIN_JINSHUI_DCF, PWM_CHANNEL_JINSHUI);
}

void setupDisplay() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  updateDisplay();
}

void setupWebServer() {
  server.enableCORS(true);
  server.on("/params", HTTP_GET, handleGetParams);
  server.on("/params", HTTP_POST, handleSetParams);
  server.begin();
}

void handleFlowPulse() {
  flow_pulse_count++;
  flags.flow_pulse = true;
}

InputStatus readInputs() {
  InputStatus inputs{};
  inputs.chalv_qian_low = digitalRead(PIN_CHALV_QIAN_DIYA) == HIGH;
  inputs.chalv_hou_low = digitalRead(PIN_CHALV_HOU_DIYA) == HIGH;
  inputs.jingshui_gaoya = digitalRead(PIN_JINGSHUI_GAOYA) == HIGH;
  inputs.jingshui_diya = digitalRead(PIN_JINGSHUI_DIYA) == HIGH;
  inputs.gaoya_kg = digitalRead(PIN_GAOYA_KG) == HIGH;
  return inputs;
}

void runStateMachine(const InputStatus &inputs, OutputCommand &outputs) {
  // 这里是整机逻辑框架：按照需求描述划分状态，后续填充细节
  switch (work_state) {
    case WorkState::Idle:
      state_message = "待机";
      data_message = "等待低压启动";
      outputs = OutputCommand{};
      if (inputs.jingshui_diya) {
        work_state = WorkState::PreStore;
      }
      break;
    case WorkState::PreStore:
      state_message = "预存洗膜水";
      data_message = "原水冲洗中";
      outputs.zong_jinshui = true;
      outputs.beng_qian_jinshui = true;
      outputs.zengya_beng = true;
      // TODO: 根据洗膜开关、冲洗时长等参数补充逻辑
      if (!inputs.jingshui_gaoya) {
        work_state = WorkState::MakeWater;
      }
      break;
    case WorkState::MakeWater:
      state_message = "制水";
      data_message = "监控流量/压力";
      outputs.zong_jinshui = true;
      outputs.beng_qian_jinshui = true;
      outputs.zengya_beng = true;
      // TODO: 按需求补充超滤堵、废水冲洗、超时等逻辑
      if (!inputs.gaoya_kg) {
        work_state = WorkState::Full;
      }
      break;
    case WorkState::Full:
      state_message = "水满";
      data_message = "延时/冲洗";
      outputs = OutputCommand{};
      // TODO: 按需求补充水满后延时制水与纯净水冲洗
      work_state = WorkState::FullAfterFlush;
      break;
    case WorkState::FullAfterFlush:
      state_message = "水满待机";
      data_message = "等待再启动";
      outputs = OutputCommand{};
      if (inputs.jingshui_diya) {
        work_state = WorkState::PreStore;
      }
      break;
    case WorkState::Alarm:
    default:
      state_message = "故障停机";
      data_message = "请排查";
      outputs = OutputCommand{};
      break;
  }
}

void setValveWithPwm(uint8_t pin, uint8_t channel, bool open) {
  if (!open) {
    ledcWrite(channel, 0);
    return;
  }
  ledcWrite(channel, PWM_START_DUTY);
  delay(PWM_START_MS);
  ledcWrite(channel, PWM_HOLD_DUTY);
}

void applyOutputs(const OutputCommand &outputs) {
  setValveWithPwm(PIN_ZONG_JINSHUI_DCF, PWM_CHANNEL_ZONG, outputs.zong_jinshui);
  digitalWrite(PIN_CHALV_CHONGXI_DCF, outputs.chalv_chongxi ? HIGH : LOW);
  setValveWithPwm(PIN_BENG_QIAN_JINSHUI_DCF, PWM_CHANNEL_BENG_QIAN, outputs.beng_qian_jinshui);
  setValveWithPwm(PIN_JINSHUI_DCF, PWM_CHANNEL_JINSHUI, outputs.jinshui);
  digitalWrite(PIN_FEISHUI_CHONGXI_DCF, outputs.feishui_chongxi ? HIGH : LOW);
  digitalWrite(PIN_ZENGYA_BENG, outputs.zengya_beng ? HIGH : LOW);
  digitalWrite(PIN_FANGDONG_JIARE, outputs.fangdong_jiare ? HIGH : LOW);
}

void updateDisplay() {
  u8g2.clearBuffer();
  u8g2.drawUTF8(0, 16, state_message);
  u8g2.drawUTF8(0, 48, data_message);
  u8g2.sendBuffer();
}

void handleGetParams() {
  StaticJsonDocument<512> doc;
  doc["yuanshui_chongxi_s"] = params.yuanshui_chongxi_s;
  doc["shuiman_zhishui_delay_min"] = params.shuiman_zhishui_delay_min;
  doc["shuiman_chongxi_s"] = params.shuiman_chongxi_s;
  doc["feishui_chongxi_jg_min"] = params.feishui_chongxi_jg_min;
  doc["feishui_chongxi_s"] = params.feishui_chongxi_s;
  doc["chlv_duanshi_s"] = params.chlv_duanshi_s;
  doc["chlv_chixu_s"] = params.chlv_chixu_s;
  doc["chlv_open_s"] = params.chlv_open_s;
  doc["chlv_close_s"] = params.chlv_close_s;
  doc["chlv_cycles"] = params.chlv_cycles;
  doc["zhishui_min"] = params.zhishui_min;

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleSetParams() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "missing body");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "text/plain", "invalid json");
    return;
  }

  params.yuanshui_chongxi_s = doc["yuanshui_chongxi_s"] | params.yuanshui_chongxi_s;
  params.shuiman_zhishui_delay_min = doc["shuiman_zhishui_delay_min"] | params.shuiman_zhishui_delay_min;
  params.shuiman_chongxi_s = doc["shuiman_chongxi_s"] | params.shuiman_chongxi_s;
  params.feishui_chongxi_jg_min = doc["feishui_chongxi_jg_min"] | params.feishui_chongxi_jg_min;
  params.feishui_chongxi_s = doc["feishui_chongxi_s"] | params.feishui_chongxi_s;
  params.chlv_duanshi_s = doc["chlv_duanshi_s"] | params.chlv_duanshi_s;
  params.chlv_chixu_s = doc["chlv_chixu_s"] | params.chlv_chixu_s;
  params.chlv_open_s = doc["chlv_open_s"] | params.chlv_open_s;
  params.chlv_close_s = doc["chlv_close_s"] | params.chlv_close_s;
  params.chlv_cycles = doc["chlv_cycles"] | params.chlv_cycles;
  params.zhishui_min = doc["zhishui_min"] | params.zhishui_min;

  server.send(200, "text/plain", "ok");
}
