#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include <WebServer.h>

// ===================== 引脚定义（按需求固定） =====================
// 输出引脚（NMOS 驱动）
constexpr uint8_t PIN_ZONG_JINSHUI_DCF = 33;   // 总进水电磁阀
constexpr uint8_t PIN_CHALV_CHONGXI_DCF = 25;  // 超滤冲洗电磁阀
constexpr uint8_t PIN_BENG_QIAN_JINSHUI_DCF = 26; // 泵前进水电磁阀
constexpr uint8_t PIN_JINSHUI_DCF = 14;        // 净水电磁阀
constexpr uint8_t PIN_FEISHUI_CHONGXI_DCF = 12; // 废水冲洗电磁阀
constexpr uint8_t PIN_ZENGYA_BENG = 4;         // 增压泵（开机默认低电平）
constexpr uint8_t PIN_FANGDONG_JIARE = 27;     // 防冻加热

// 输入引脚（开关量）
constexpr uint8_t PIN_CHALV_QIAN_DIYA = 36;    // 超滤前低压开关
constexpr uint8_t PIN_CHALV_HOU_DIYA = 39;     // 超滤后低压开关
constexpr uint8_t PIN_JINGSHUI_GAOYA = 34;     // 净水高压开关
constexpr uint8_t PIN_JINGSHUI_DIYA = 35;      // 净水低压开关
constexpr uint8_t PIN_GAOYA_KG = 32;           // 高压力开关
constexpr uint8_t PIN_LIULIANG = 13;           // 流量传感器（默认 GPIO13，可按硬件调整）

// ===================== PWM 参数 =====================
constexpr uint8_t PWM_CHANNEL_ZONG = 0;
constexpr uint8_t PWM_CHANNEL_BENG_QIAN = 1;
constexpr uint8_t PWM_CHANNEL_JINSHUI = 2;
constexpr uint16_t PWM_FREQ = 1000;
constexpr uint8_t PWM_RESOLUTION = 8;
constexpr uint8_t PWM_START_DUTY = 255; // 启动占空比
constexpr uint8_t PWM_HOLD_DUTY = 130;  // 维持占空比
constexpr uint16_t PWM_START_MS = 500;  // 吸合时间

// ===================== 状态与参数 =====================
struct RuntimeFlags {
  bool flow_pulse = false;
  bool chlv_need_flush = false;
  bool alarm = false;
};

struct InputStatus {
  bool chalv_qian_low = false;
  bool chalv_hou_low = false;
  bool jingshui_gaoya = false;
  bool jingshui_diya = false;
  bool gaoya_kg = false;
};

struct OutputCommand {
  bool zong_jinshui = false;
  bool chalv_chongxi = false;
  bool beng_qian_jinshui = false;
  bool jinshui = false;
  bool feishui_chongxi = false;
  bool zengya_beng = false;
  bool fangdong_jiare = false;
};

struct ParameterSet {
  // 可由手机网页设置的参数（单位：秒/分钟）
  uint16_t yuanshui_chongxi_s = 15;          // 制水前原水冲洗
  uint16_t shuiman_zhishui_delay_min = 5;   // 水满后延时制水
  uint16_t shuiman_chongxi_s = 15;          // 水满后纯净水冲洗
  uint16_t feishui_chongxi_jg_min = 10;     // 制水中废水冲洗间隔
  uint16_t feishui_chongxi_s = 15;          // 制水中废水冲洗时长
  uint16_t chlv_duanshi_s = 3;              // 超滤后短暂断开时间
  uint16_t chlv_chixu_s = 8;                // 超滤后持续断开时间
  uint16_t chlv_open_s = 20;                // 超滤冲洗：开总进水时长
  uint16_t chlv_close_s = 20;               // 超滤冲洗：开超滤阀时长
  uint8_t chlv_cycles = 3;                  // 超滤冲洗次数
  uint16_t zhishui_min = 10;                // 制水最小运行时间
};

enum class WorkState : uint8_t {
  Idle = 0,
  PreStore = 10,
  MakeWater = 20,
  Full = 30,
  FullAfterFlush = 40,
  Alarm = 90,
};

extern WebServer server;
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern ParameterSet params;
extern RuntimeFlags flags;
extern WorkState work_state;
extern volatile uint32_t flow_pulse_count;

// 显示内容（上/下两部分）
extern const char *state_message;
extern const char *data_message;

