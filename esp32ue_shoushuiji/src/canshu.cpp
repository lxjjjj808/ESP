#include "canshu.h"

WebServer server(80);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/22, /* data=*/21);

ParameterSet params{};
RuntimeFlags flags{};
WorkState work_state = WorkState::Idle;
volatile uint32_t flow_pulse_count = 0;

const char *state_message = "系统初始化";
const char *data_message = "等待启动";

