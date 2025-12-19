#include "canshu.h"
#include <Arduino.h>
#include <Preferences.h>
#include <PubSubClient.h>   

Preferences pre;
WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);






Metro wifi_metro(wifi_jg_shch * 1000); // Metro for 1 second intervals
/*-------------------------------产品----------------------------------------*/
 String device_name="jingshuiqi";
  String jianjie=""; //产品简介
  String gongneng=""; //产品功能
  /*--------------------------------工作状态数字------------------------------------------*/
  byte zs_dci_chshi_n = 1; //制水多次超时
  byte yc_dci_bn_sm_n = 3; //预存多次不能水满
  byte feish_dci_guosh_n = 5; //废水多次过少
  byte loushui_dci_n = 7; //漏水多次
  
  byte yucun_buneng_shuiman_n = 18; //预存不能水满
  byte zhishui_chao_shijian_n = 28; //制水超时
  byte zhishui_feishui_guoshao_n = 24; //制水废水太少  

  byte send_onenet_data_n = 60; //发送onenet数据
  byte receive_onenet_data_n = 61; //接收onenet数据
  byte send_product_data_n = 62; //发送产品数据
  byte receive_product_data_n = 63; //接收产品数据
  byte send_zs_data_n = 64; //发送制水数据
  byte receive_zs_data_n = 65; //接收制水数据
  byte send_chlv_data_n = 66; //发送超滤数据
  byte receive_chlv_data_n = 67; //接收超滤数据
  byte send_wifi_data_n = 68; //发送wifi数据
  byte receive_wifi_data_n = 69; //接收wifi数据
  byte send_ota_data_n = 70; //发送ota数据
  byte receive_ota_data_n = 71; //接收ota数据
  byte send_wifi_list_n = 72; //发送wifi列表


  byte chlv_chongxi_n = 80; //超滤冲洗
  byte chlv_chongxi_guanbi_n = 81; //超滤冲洗关闭
  byte shtmo_chxi_n = 85; //渗透膜冲洗
 
  byte shuiya_zhch_n = 50; //水压正常
  byte guanfa_tji_n = 51; //关阀停机
  byte chaolv_qian_dz_queshui_n = 53; //超滤前短暂缺水
  byte chaolv_qian_cj_queshui_n = 54; //超滤前长久缺水
  byte chlv_hou_dz_queshui_n = 56; //超滤后短暂缺水
  byte chlv_hou_cj_queshui_n = 58; //超滤后长久缺水

  byte yucun_qch_open_n=15; //预存强冲开启
  byte yucun_qch_close_n=16; //预存强冲关闭
  byte zhishui_qch_open_n=25; //制水强冲开启
  byte zhishui_qch_close_n=26; //制水强冲关闭
  byte zhishui_hou_chxi_open_n=35; //制水后冲洗开启
  byte zhishui_hou_chxi_close_n=36; //制水后冲

/*--------------------------------ota------------------------------------------*/
 uint32_t version=20251219; //ota版本
 unsigned long lastUpdateTime=0; //上次ota更新时间
 String partionName=""; //分区名称
  const char* chanpin_lei="xiaoshangyongzuji"; //产品类别
  String    versionUrl="";
/* ---------------         wifi       ----------------------  */
//const char *ssid = "lyd";	// WiFi����
String s_ssid="lyd";
//const char *password = "19711128";	
String s_password="19711128";
byte wifi_fail_cishu=0;//WiFi连接失败次数
byte wifi_cg_cishu=0;//WiFi连接成功次数
 unsigned long wifi_jg_shch=35;//wifi连接时长 秒
unsigned int set_property_reply_id=0;

const char *mqtt_server = "mqtts.heclouds.com"; // MQTT��������ַ
const int mqtt_port = 1883;						// MQTT�������˿�
//const char *product_id= "WoMShsmX5V"; // ��ƷID
//const char *device_id = "test"; // �豸ID
//const char *api_key="version=2018-10-31&res=products%2FWoMShsmX5V%2Fdevices%2Ftest&et=1774492099&method=md5&sign=HWgHJaui2rMvgEnsg8Swaw%3D%3D"; // API��Կ
 
ProductKey productKeys[] = {
  {"租机", "hFOtBOuext", "version=2018-10-31&res=products%2FhFOtBOuext&et=1884338023&method=md5&sign=t4d3GdlE6JmT0ehUcPL7Jg%3D%3D"},
  {"小商用", "MaYhjDb876", "version=2018-10-31&res=products%2FMaYhjDb876&et=1884338023&method=md5&sign=cZeLMcp6wLGnrjaJrTOYEQ%3D%3D"},
  {"售水机", "WoMShsmX5V", "version=2018-10-31&res=products%2FWoMShsmX5V&et=1884338023&method=md5&sign=fN8hz97ctXeq460a9W1dSg%3D%3D"}
};// 定义三种产品类型的密钥
   const int PRODUCT_COUNT = sizeof(productKeys) / sizeof(productKeys[0]);


String s_product_id="hFOtBOuext";//租机
 //String s_product_id="MaYhjDb876";//小商用
 String s_product_leibie="";//产品类别               
  String s_device_id="ceshi";
 String s_api_key="";//
 //const char *s_api_key="version=2018-10-31&res=products%2FMaYhjDb876&et=1884338023&method=md5&sign=cZeLMcp6wLGnrjaJrTOYEQ%3D%3D";//小商用
                 //  2029-09-17 16:13:14 小商用租机产品级                                                                                    
const char* data_topic_template = "$sys/%s/%s/thing/property/post"; // �����ϴ�����
const char* data_topic_reply_template = "$sys/%s/%s/thing/property/post/reply";//备属性上报响应,OneNET---->设备
const char* set_topic_template = "$sys/%s/%s/thing/property/set"; // �����·�����
const char* set_reply_topic_template = "$sys/%s/%s/thing/property/set_reply"; // ������Ӧ����
const char* cmd_topic_template = "$sys/%s/%s/cmd/request/+"; // 设备属性设置请求, OneNET---->设备
const char* cmd_response_topic_template = "$sys/%s/%s/cmd/response/%d"; // 命令响应主题

char data_topic[100];
char data_reply_topic[100];
char set_topic[100];
char set_reply_topic[100];
char cmd_topic[100];
char cmd_response_topic[100];
int postMsgId = 0; 
bool send_reset_reason=false; // 是否发送重启原因
unsigned long send_jiange = 0; // 用于间隔发送状态显示

/*--------------------------------调试------------------------------------------*/

 bool tiao_shi=false;

 /*--------------------------------状态控制------------------------------------------*/
 // 预存：״̬10   正常制水：20. 水满洗膜：30  水满洗膜完成：40
 byte jishi_dangqian_zhtai=0,jishi_yiqian_zhtai=0;
 byte changjiu_xian_zhtai=0,changjiu_qian_zhtai=0;
 unsigned long xiantai_jin_shike=0,zhtai_yanshi_kaishi_shike=0;
 unsigned long qiantai_jin_shike=0;
 
 unsigned long zhtai_xianshi_jiange=0; //公用变量,用于间隔显示状态
 byte work_state=0;
 unsigned long gang_qidong_shike=0; //刚启动时刻
/*--------------------------------输入-----------------------------------------*/
int    chlv_qian_diya_kg=36; //超滤前压力开关
int    chlv_hou_diya_kg=37; //超滤后压力开关
int    yucun_kg=34; //预存开关
int    ylt_kg=35; //水满开关
 const int shuiliu_chuanganqi = 12; //水流传感器
 /*--------------------------------输出------------------------------------------*/
int zengya_beng =13;//增压泵
int zong_dcf = 32;//总进水电磁阀
int chlv_chxi_dcf=33;// 超滤冲洗电磁阀
int jinshui_dcf = 25;//泵前进水电磁阀
int js_dcf = 26;//净水电磁阀
int feishui_chxi_dcf=27;//废水冲洗电磁阀
int fdjr=14;//防冻加热
int fengmingqi=15;//蜂鸣器

int cewen=23;//���´�����

/*---------------------------- pwm-----------------------------------------------*/
const int CHANNEL0 = 0;           // PWMͨ��0
const int CHANNEL1 = 1;           // PWMͨ��1
const int CHANNEL2 = 2;           // PWMͨ��2
const int PWM_FREQ = 1000;       // PWMƵ��(Hz)
const int RESOLUTION = 8;        // 8λ�ֱ���(0-255)
 int START_POWER = 255;     // ��������(100%)
 int HOLD_POWER = 130;       // ά�ֹ���(Լ31%)
 unsigned long START_DURATION = 500; // ��������ʱ��(ms)
 bool zong_dcf_state=false; //总进水电磁阀状态
/*------------------------------显示屏--------------------------------------*/
 U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/22, /* data=*/21);
String state_str="",data_str="";
String pre_state_str="",pre_data_str="";  
char state_1[25]={0};
char state_2[25]={0};
char data_1[25]={0};
char data_2[25]={0};
 // enum shang_xia{state,data};
 const  char *state_message="";
 const char *pre_state_message="";
 const char *pre_data_message="";
  const  char *data_message="";
/*--------------------------------水流传感器-------------------------------------------*/
volatile byte zs_pulse = 0; //正常制水脉冲数
 volatile byte zs_cx_pulse=0;//制水状态下强冲洗脉冲数
unsigned long jiance_kaishike=0; //��ˮ���ʱ��??
unsigned long jiance_jieshu_shike=0; //��ˮ������ʱ��
volatile byte chlv_pulse=0;
bool pulse_flag=false; //脉冲标志，采集后为真，显示后为假
//bool jiance_wancheng=false;

/*--------------------------------超滤-------------------------------------------*/

  unsigned long accumulative=0 ; /*�����ۻ���ˮʱ��*/
 byte chlv_chxi_cishu=3 ;			//����ÿ�س�ϴ����
 byte chxi_leiji_bzh=40 ;//分钟
 bool chlv_xuyao_chxi=false ; //���˳�ϴ�Ƿ�����
 byte chlv_yi_chxi_cishu=0;//�Ѿ���ϴ����
 
 byte chlv_open_shch=10,chlv_close_shch=20;// ��λ���볬�˵�ŷ���??�ر�ʱ��
 byte chlv_state_int=0;//超滤冲洗状态
 unsigned long begin_t;

 bool leiji_chlv_xuyao_chxi=false;
 byte chlv_chxi_reason=0;
/*--------------------------------保护------------------------------------------*/
byte baohu_zht=0;//保护状态

 /*--------------------------------预存-------------------------------------------*/
   bool  yucun_buneng_shuiman= false; //Ԥ�治��ˮ��
byte yucun_buneng_shuiman_cishu=0; //Ԥ�治��ˮ��������0
byte yucun_buneng_shuiman_cish_bzhun=3;
//bool cengjing_yucun=false; //����Ԥ��ϴĤˮ����
unsigned long yc_delay=80000; //为了避开开始制水时的高tds值净水 80秒
unsigned long yucun_kaishi_shike=0; //预存洗膜水开始时刻
byte yucun_ximo_yunxu_shch=4; // 预存允许时长 分钟
/* /*--------------------------------渗透膜冲洗------------------------------------------*/
    /* byte fangdou=250;//按键防抖
    byte max_interval=1;//多次按键间的最大间隔 秒  
     int chxi_btn=0; //下载按键三次短按作渗透膜冲洗 */ 
bool shtmo_chxi_flag=false; // 渗透膜冲洗标志
unsigned long shtmo_chxi_start_time=0; // 渗透膜冲洗开始时间
unsigned long shtmo_chxi_duration=180; // 渗透膜冲洗持续时间 秒


/*-------------------------------制水-------------------------------------------*/

byte dc_zs_shch_bz = 50;	 //60分钟
unsigned long yi_zhishui_shch=0; //

byte xianshi=0;

byte danci_zhishui_shch=0;//分钟

byte zs_pulse_bzh= 15;

byte zhishui_chaoshi_cishu = 0; /*????2?��???????????*/
byte feishui_guoshao_cishu = 0;
byte  zs_gs_cs_bzh=10;//制水过少次数标准

unsigned long zhishui_xianshi_jiange=0;
 unsigned long zyb_qidong_shike=0; //制水开始时刻    
 unsigned long zyb_tzhi_shike=0; //制水停止时刻
bool zhishui_delay=false; //制水延时
  byte zhsh_yshi_bzh=5; //制水延时标准 分钟
unsigned long zhishui_kaishi_shke=0;

//unsigned long shuiman_shike=0;//水满时刻
unsigned long zhishui_shich=0;//制水时刻
/*-------------------------------原水冲洗-----------------------------------------*/
 bool xuyao_feishui_chxi=false;//需要废水冲洗
 byte ys_chxi_shch=18;//原水冲洗时长 秒
  byte ys_chxi_gb_shch=15;//分钟
  byte zsh_chxi_sch=25;//制水后冲洗秒
 unsigned long feishui_chxi_kshi_shke=0,feishui_chxi_jieshu_shike=0;
 //bool feishui_chxi_hukong=false;
byte feishui_chxi_state=0; //废水冲洗状态
byte zhishui_hou_chxi_state=0;//制水后冲洗状态
byte feishui_guoshao_ximo_cishu=0; //废水过少洗膜次数 
unsigned long kaishi_shike=0;
/*--------------------------------��ˮ����ˮϴĤ-------------------------------------------*/
 //bool xuyao_ximo=false;
 //byte ximo_chxi_shch =10, ximo_gbi_shch=60 ;//ϴĤʱ��ϴʱ���͹ر�ʱ�� ��λ��
 //byte yi_ximo_cishu=0;
 //unsigned long ximo_yanshi_shike,ximo_xianshi_jiange;
 //byte  ximo_state_byte=0; //ϴĤ״̬
 
/*--------------------------------洗膜----------------------------------------*/
    //unsigned long  yucun_kaishi_shike; //Ԥ��ϴĤ��ʱ��ʼʱ��
     byte xm_gd_bzh=60;// 洗膜时间过短 秒
     byte xm_gj_bzh=6;//洗膜时间过久 分钟
     unsigned long ximo_kshi_shike=0; // 预存洗膜水开始时刻
/*--------------------------------alarm-------------------------------------------*/
//String state = ""; //??????
unsigned long alarm_jiange = 0;
bool  zhishui_feishui_guoshao = false;

//unsigned long ��ˮ��ˮ���ٿ�ʼʱ��;//����ʱ���ڻ���ͣ��
bool zhishui_chaoshi=false; //��ˮ��ʱ
//unsigned long feishuiguoshao_xianshi_jiange=0;
bool baojing=false;
  bool baojing_chuli=false; //��������

byte chaoshi_cishu_bzh =5; //制水超时次数标准
unsigned long baojing_qishi_shike=0;
/*-------------------------------ȱˮ------------------------------------------*/
unsigned long queshui_xianshi_jiange=0,queshui_start=0;
bool shuiya_zhch=true;
unsigned long shuiya_zhch_shike=0;
unsigned long chaolv_qian_queshui_shike=0;
unsigned long chaolv_hou_queshui_shike=0;
unsigned long  zong_dcf_gb_shike=0;
byte loushui_cishu=0;//©ˮ����
/*-------------------------------tds���??--------------------------------------

//unsigned long tds_jiange=0;
unsigned int jingshui_tds_i,nongshui_tds_i;
unsigned char tds_buffer[20];
unsigned int tds_buffer_index=0;
bool jingshui_tds_jiexi=false,nongshui_tds_jiexi=false;*/
/*-------------------------------�¶�------------------------------------------
 
 /*--------------------------------温度-------------------------------------------*/
OneWire oneWire(5); // d19
DallasTemperature sensors(&oneWire);
DeviceAddress address_18b20;
float wendu;

 /*------------------------------������--------------------------------------*/




