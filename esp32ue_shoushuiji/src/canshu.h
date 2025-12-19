#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <WebServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <Metro.h>
#include <OneWire.h>
#include <DallasTemperature.h>

struct ProductKey {
  const char* leibie;           // 产品类别
  const char* product_id;     // 产品ID
  const char* api_key;     // 访问密钥
};
extern Preferences pre;
extern  WebServer server;
extern WiFiClient espClient;
extern  PubSubClient client;

extern  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
extern Metro wifi_metro; // Metro for 1 second intervals
/*extern����
����1�����롰C��һ��ʹ��ʱ���� extern "C" void fun(int a, int b)�����������fun�������������c���Թ�����������������c++�涨��������C++�п������ø÷������c����
����2���������롰C"��һ�����α������ߺ���ʱ������ͷ�ļ��У�extern int g_Num�������þ��������������߱��������÷�Χ�Ĺؼ��֣��������ĺ����ͱ��������ڱ����뵥Ԫ�����������뵥Ԫʹ��


/*-------------------------------产品----------------------------------------*/
    extern  String device_name ;	
    extern String  jianjie; //产品简介
    extern String  gongneng; //产品功能
/*--------------------------------温度-------------------------------------------*/
 extern  OneWire oneWire; // d19
  extern  DallasTemperature sensors;
 extern DeviceAddress address_18b20;
   extern float wendu;

/*--------------------------------ota------------------------------------------*/
extern uint32_t version; //ota版本
extern unsigned long lastUpdateTime; //上次检查更新的时间
extern String partionName; //分区名称
extern const char* chanpin_lei;
extern String    versionUrl;
/*--------------------------------工作状态数字------------------------------------------*/
extern byte zs_dci_chshi_n; //制水多次超时
extern byte yc_dci_bn_sm_n; //预存多次不能水满
extern byte feish_dci_guosh_n; //废水多次过少
extern byte loushui_dci_n; //漏水多次
extern byte yucun_buneng_shuiman_n; //预存不能水满
extern byte zhishui_chao_shijian_n; //制水超时
extern byte zhishui_feishui_guoshao_n; 
extern byte shuiya_zhch_n; //水压正常
extern byte guanfa_tji_n; //管阀停机
extern byte chaolv_qian_dz_queshui_n; //超滤前短暂缺水
extern byte chaolv_qian_cj_queshui_n; //超滤前长久缺水
extern byte chlv_hou_dz_queshui_n; //超滤后短暂缺水
extern byte chlv_hou_cj_queshui_n; //超滤后长久缺水

extern byte send_onenet_data_n; //发送onenet数据
extern byte receive_onenet_data_n; //接收onenet数据
extern byte send_wifi_data_n; //发送wifi数据
extern byte send_wifi_list_n; //发送wifi列表  
extern byte receive_wifi_data_n; //接收wifi数据
extern byte send_product_data_n; //发送产品数据
extern byte receive_product_data_n; //接收产品数据
extern byte send_chlv_data_n; //发送超滤数据
extern byte receive_chlv_data_n; //接收超滤数据
extern byte send_zs_data_n; //发送制水数据
extern byte receive_zs_data_n; //接收制水数据
extern byte send_ota_data_n; //发送ota数据
extern byte receive_ota_data_n; //接收ota数据
extern byte shtmo_chxi_n; //渗透膜冲洗

extern byte chlv_chxi_n; //超滤冲洗
extern byte chlv_chxi_gb_n; //超滤冲洗关闭

extern byte shuiya_zhch_n; //水压正常

extern byte yucun_qch_open_n; //预存强冲开启
extern byte yucun_qch_close_n; //预存强冲关闭
extern byte zhishui_qch_open_n; //制水强冲开启
extern byte zhishui_qch_close_n; //制水强冲关闭
extern byte zhishui_hou_chxi_open_n; //制水后冲洗开启
extern byte zhishui_hou_chxi_close_n; //制水后冲洗关闭

/*--------------------------------整机状态------------------------------------------*/
 extern    byte   jishi_dangqian_zhtai,jishi_yiqian_zhtai;
 extern    byte      changjiu_xian_zhtai,changjiu_qian_zhtai;
 extern    unsigned long xiantai_jin_shike,zhtai_yanshi_kaishi_shike;
 extern unsigned long qiantai_jin_shike;
 extern byte yucun_ximo_yunxu_shch;//yucun_ximo_yunxu_shch ��λ������
 extern unsigned long zhtai_xianshi_jiange; //��ʾ���?????
extern byte work_state;
extern unsigned long gang_qidong_shike; //刚启动时刻



/*--------------------------------保护------------------------------------------*/
extern  byte baohu_zht;//保护状态
/*--------------------------------输入输出------------------------------------------*/
extern const int    shuiliu_chuanganqi;//水流传感器
extern  int    chlv_qian_diya_kg,chlv_hou_diya_kg,yucun_kg,ylt_kg; //����
extern int zong_dcf;//pcf8575 1��
extern int chlv_chxi_dcf;   
extern int jinshui_dcf;//jinshui_dcf
extern int zengya_beng;
extern int feishui_chxi_dcf;
extern int js_dcf;
extern int fengmingqi;
extern int fdjr;
extern  int  cewen;

extern   const int CHANNEL0;           // PWMͨ��0
extern   const int CHANNEL1;
extern   const int CHANNEL2;           // PWMͨ��2
extern   const int PWM_FREQ ;       // PWMƵ��(Hz)
extern  const int RESOLUTION;        // 8λ�ֱ���(0-255)
// 功率设置 (根据电�?�阀特性调�?)
extern int START_POWER ;     // 启动功率(100%)
extern int HOLD_POWER ;       // 维持功率(�?31%)
extern unsigned long START_DURATION ; // �?动持续时间(ms)
extern bool zong_dcf_state; //总进水电磁阀状态
/*--------------------------------渗透膜冲洗------------------------------------------*/
   /*  extern byte fangdou;//按键防抖
    extern byte max_interval;//多次按键间的最大间隔  
    extern int chxi_btn; //下载按键三次短按作渗透膜冲洗 */
    extern bool shtmo_chxi_flag; // 渗透膜冲洗标志
    extern unsigned long shtmo_chxi_start_time; // 渗透膜冲洗开始时间
    extern unsigned long shtmo_chxi_duration; // 渗透膜冲洗持续时间

/*-------------------------------洗膜-----------------------------------------*/
 extern unsigned long ximo_kshi_shike;  // 洗膜开始时刻，为计算洗膜时间准备
 /*-------------------------------超滤------------------------------------------*/
 extern   unsigned long accumulative; /*超滤累计时长*/
extern byte chlv_chxi_cishu;			//超滤冲洗次数
  extern byte chxi_leiji_bzh ;//超滤累积冲洗标准
  extern bool chlv_xuyao_chxi; //���˳�ϴ�Ƿ�����
extern byte chlv_yi_chxi_cishu;
extern byte chlv_open_shch;//超滤冲洗打开时长 秒
 extern byte chlv_close_shch;//关闭时长 秒
extern volatile byte chlv_pulse;
extern byte chlv_state_int;
extern   unsigned long begin_t;
extern  bool leiji_chlv_xuyao_chxi;
extern byte chlv_chxi_reason;//超滤冲洗原因，累积 超滤后缺水
/*-------------------------------温度-----------------------------------------*/
 
 extern  unsigned long wendu_xianshi_jiange;
 extern float wendu;
 extern unsigned long fangdong_jiange;
 extern  bool shifou_cewen;
/*--------------------------------调试-------------------------------------------*/

extern bool tiao_shi;
/*-------------------------------预存-------------------------------------------*/
 //extern bool cengjing_yucun; //����Ԥ��ϴĤˮ����
 extern unsigned long yc_delay; //为了避开开始制水时的高tds值净水
 extern unsigned long yucun_kaishi_shike; //预存洗膜谁开始时刻

 extern unsigned long  yucun_kaishi_shike; //Ԥ��ϴĤ��ʱ��ʼʱ��
 extern byte xm_gd_bzh;
 extern byte xm_gj_bzh;
 /*--------------------------------水流传感器-------------------------------------------*/
extern volatile byte zs_pulse; //
extern volatile byte zs_cx_pulse;//制水状态下强冲洗脉冲数
extern unsigned long jiance_kaishike; //
extern unsigned long jiance_jieshu_shike; //��ˮ������ʱ��
extern bool pulse_flag;  //脉冲标志，采集后为真，显示后为假
 /*--------------------------------制水-------------------------------------------*/

 
 
 //extern  byte jiance_state ; //��ˮ���״�?
 //extern bool jiance_wancheng; //��ˮ������
extern  byte dc_zs_shch_bz ;	 //单次制水时长标准
extern  unsigned long yi_zhishui_shch; //

extern byte xianshi;


extern byte zhishui_shjian_add;//��ˮʱ���?????��ַ
extern byte danci_zhishui_shch;//��ˮ���ι���ʱ�� ����
extern  unsigned long zhishui_xianshi_jiange;
extern  byte zs_pulse_bzh;
extern unsigned long zhishui_shich;//����ˮʱ��
/*????2?��???????????*/
extern  byte feishui_guoshao_cishu;
extern  byte  zs_gs_cs_bzh ;//制水过少次数标准
extern unsigned long zhishui_kaishi_shke;
 extern unsigned long zyb_qidong_shike; //制水开始时刻                   
extern unsigned long zyb_tzhi_shike; //制水停止时刻
//extern unsigned long shuiman_shike;//ˮ��ʱ��
extern  bool zhishui_delay; //制水延时
extern byte zhsh_yshi_bzh; //制水延时标准
/*-------------------------------原水冲洗------------------------------------------*/
extern bool xuyao_feishui_chxi;//��ˮ��ԭˮǿ��
extern byte ys_chxi_shch,ys_chxi_gb_shch;
 
  extern byte zsh_chxi_sch; //��ˮ����?��ŷ���ϴʱ��????? ��λ����
  extern byte zhishui_hou_chxi_state;//��ˮ���ϴ״�?
extern unsigned long feishui_chxi_kshi_shke,feishui_chxi_jieshu_shike;//��ˮ��
//extern bool feishui_chxi_hukong;
extern byte feishui_chxi_state;//��ˮ�з�ˮ��ϴ
extern byte feishui_guoshao_ximo_cishu; //��ˮ����ϴĤ���� 

/*--------------------------------��ˮ����ˮϴĤ-------------------------------------------*/

//extern bool xuyao_ximo;
//extern byte ximo_chxi_shch , ximo_gbi_shch ;//ϴĤʱ��ϴʱ���͹ر�ʱ�� ��
//extern byte  yi_ximo_cishu,ximo_state_byte;
//extern unsigned long ximo_yanshi_shike,ximo_xianshi_jiange;



/*--------------------------------alarm-------------------------------------------*/
//String state = ""; //??????
extern unsigned long baojing_qishi_shike;
extern  byte zhishui_chaoshi_cishu ; //��ʱ����
extern  byte chaoshi_cishu_bzh ;
extern unsigned long alarm_jiange ;//��Ϸ��ͱ������?
extern  bool jingshui_chaoya; 
extern bool  yucun_buneng_shuiman;//Ԥ�治��ˮ��
extern byte yucun_buneng_shuiman_cishu; //Ԥ�治��ˮ������
extern byte yucun_buneng_shuiman_cish_bzhun; //Ԥ�治��ˮ������������ֵ
extern unsigned long jingshui_chaoya_begintime;//��ˮ��ѹ��ʼʱ��
extern bool zhishui_chaoshi;
//extern unsigned long feishuiguoshao_xianshi_jiange;
 extern bool baojing_chuli, baojing;
extern  bool zhishui_feishui_guoshao;
/*-------------------------------ȱˮ------------------------------------------*/
extern unsigned long queshui_xianshi_jiange,queshui_start;
extern bool shuiya_zhch;
extern unsigned long shuiya_zhch_shike;
extern unsigned long chaolv_qian_queshui_shike;
extern unsigned long chaolv_hou_queshui_shike;
 extern  unsigned long  zong_dcf_gb_shike;
extern byte loushui_cishu;//©ˮ����

/*-------------------------------tdsj���?????--------------------------------------*/

//extern  unsigned long tds_jiange;
extern unsigned int jingshui_tds_i,nongshui_tds_i;
extern unsigned char tds_buffer[20];
extern unsigned int tds_buffer_index;
extern bool jingshui_tds_jiexi,nongshui_tds_jiexi;
/*------------------------------iic-------------------------------------*/
extern String state_str,data_str;
extern String pre_state_str,pre_data_str;
//extern char *message;
extern char state_1[25];
extern char state_2[25];
extern char data_1[25];
extern char data_2[25];
   
   extern const char *state_message;
   extern const char *pre_state_message;
   extern const char *data_message;
   extern const char *pre_data_message;
   /*    pcf ���?????         */

/*------------------------------wifi-onenet-------------------------------------*/
//extern  const char *ssid ;			// WiFi����
//extern   const char *password;	
extern String s_ssid,s_password;
extern byte wifi_fail_cishu;//WiFi连接失败次数
extern byte wifi_cg_cishu;//WiFi连接成功次数
extern unsigned long wifi_jg_shch;
extern  unsigned int set_property_reply_id;

 extern const char *mqtt_server ; // MQTT��������ַ
extern  const int mqtt_port ;			// MQTT�������˿�
//extern  const char *product_id; // ��ƷID
//extern  const char *device_id ; // �豸ID
//extern const char* api_key; // API��Կ
extern  const char* data_topic_template ; // �����ϴ�����
extern const char* data_topic_reply_template ; // 备属性上报响应,OneNET---->设备
extern const char* set_topic_template;//设置属性主题模板
extern const char* set_reply_topic_template;//设置属性主题回复模板
extern const char* cmd_topic_template ; // 订阅命令模板
extern const char* cmd_response_topic_template ; // 订阅命令回复模板
extern  String s_product_leibie; //产品类别
extern  String s_product_id;
extern String s_device_id;
extern  String s_api_key;
extern  ProductKey productKeys[];
extern  const int PRODUCT_COUNT;
extern   char data_topic[100];
extern   char data_reply_topic[100];
 extern  char set_topic[100];
extern   char set_reply_topic[100];
extern char cmd_topic[100];
extern char cmd_response_topic[100];
extern int postMsgId; // MQTT消息ID
//#define ONENET_TOPIC_PROP_FORMAT "{\"id\":\"%u\",\"version\":\"1.0\",\"params\":%s}"
extern bool send_reset_reason;
extern unsigned long send_jiange; // 用于间隔发送状态显示