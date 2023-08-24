#ifndef CONFIG_BOARD_WIRELESS_DUPLEX_DONGLE_CFG_H
#define CONFIG_BOARD_WIRELESS_DUPLEX_DONGLE_CFG_H

#ifdef CONFIG_BOARD_WIRELESS_DUPLEX_DONGLE

#include "board_wireless_duplex_dongle_global_build_cfg.h"

//*********************************************************************************//
//                                 无线麦配置                                      //
//*********************************************************************************//
//双工方案，耳机端配置为从机，dongle配置为主机
#define WIRELESS_ROLE_SEL				APP_WIRELESS_MASTER//APP_WIRELESS_SLAVE//角色选择
#define WIRELESS_24G_ENABLE				ENABLE //使能此功能可以屏蔽手机搜索到此无线设备名
//编解码采样率如果有修改，请对应修改earphone端的编解码采样率，dongle的编码要对应earphone的解码，dongle的解码要对应earphone的编码
#define WIRELESS_CODING_SAMPLERATE		(48000)
#define WIRELESS_DECODE_SAMPLERATE		(48000)
#define WIRELESS_EARPHONE_MIC_EN        DISABLE//DISABLE//dongle做耳麦使能,linein立体声输入，dac输出

#if (WIRELESS_EARPHONE_MIC_EN)
#define	WIRELESS_MIC_STEREO_EN			1
#else
#define	WIRELESS_MIC_STEREO_EN			0
#endif

#define WIRELESS_CODING_FRAME_LEN		50

//配对绑定
#ifndef WIRELESS_PAIR_BONDING
#define WIRELESS_PAIR_BONDING				DISABLE
#endif

#define	WIRELESS_TOOL_BLE_NAME_EN			ENABLE
//rf测试，由工具触发样机进入dut模式
#define TCFG_RF_TEST_EN						DISABLE

//产线近距离快速配对测试功能
//如果配对名以特殊字符串开头，程序会判断信号强度来进行连接,即靠得近的才可以连上
#define SPECIFIC_STRING						"#@#@#@"//用户可自定义修改
#define TCFG_WIRELESS_RSSI					50//demo板测试50大概对应1米的距离,越大越远

//使用PA延长距离,需要硬件添加PA电路,默认使用PC2/PC3
#define CONFIG_BT_RF_USING_EXTERNAL_PA_EN	DISABLE
//*********************************************************************************//
//                                  app 配置                                       //
//*********************************************************************************//
#define TCFG_APP_BT_EN			            1
#define TCFG_APP_MUSIC_EN			        0
#define TCFG_APP_LINEIN_EN					0
#define TCFG_APP_FM_EN					    0
#define TCFG_APP_PC_EN					    0
#define TCFG_APP_RTC_EN					    0
#define TCFG_APP_RECORD_EN				    0
#define TCFG_APP_SPDIF_EN                   0
//*********************************************************************************//
//                                 UART配置                                        //
//*********************************************************************************//
#define TCFG_UART0_ENABLE					ENABLE_THIS_MOUDLE                     //串口打印模块使能
#define TCFG_UART0_RX_PORT					NO_CONFIG_PORT                         //串口接收脚配置（用于打印可以选择NO_CONFIG_PORT）
#ifndef TCFG_UART0_TX_PORT
#define TCFG_UART0_TX_PORT  				IO_PORTC_05                            //串口发送脚配置
#endif
#define TCFG_UART0_BAUDRATE  				1000000                                //串口波特率配置

//*********************************************************************************//
//                                 USB 配置                                        //
//*********************************************************************************//
#if (WIRELESS_EARPHONE_MIC_EN)
#ifndef TCFG_PC_ENABLE
#define TCFG_PC_ENABLE                      0
#endif
#define USB_MALLOC_ENABLE                   0
#define USB_PC_NO_APP_MODE                  0
#define USB_MEM_NO_USE_OVERLAY_EN		    0
#define USB_DEVICE_CLASS_CONFIG             0
#else
#ifndef TCFG_PC_ENABLE
#define TCFG_PC_ENABLE                      1
#endif
#define USB_MALLOC_ENABLE                   1
#define USB_PC_NO_APP_MODE                  1
#define USB_MEM_NO_USE_OVERLAY_EN		    1
#define USB_DEVICE_CLASS_CONFIG             (MIC_CLASS | SPEAKER_CLASS | HID_CLASS)
#endif

#define TCFG_USB_APPLE_DOCK_EN              0


#define USB_SOF_FILL_DATA_MODE              1

//*********************************************************************************//
//                                 key 配置                                        //
//*********************************************************************************//
//#define KEY_NUM_MAX                        	10
//#define KEY_NUM                            	3
#define KEY_IO_NUM_MAX						6
#define KEY_AD_NUM_MAX						10
#define KEY_IR_NUM_MAX						21
#define KEY_TOUCH_NUM_MAX					6
#define KEY_RDEC_NUM_MAX                    6
#define KEY_CTMU_TOUCH_NUM_MAX				6

#define MULT_KEY_ENABLE						DISABLE 		//是否使能组合按键消息, 使能后需要配置组合按键映射表

#define TCFG_KEY_TONE_EN					DISABLE		// 按键提示音。建议音频输出使用固定采样率

//*********************************************************************************//
//                                 iokey 配置                                      //
//*********************************************************************************//
#ifndef TCFG_IOKEY_ENABLE
#define TCFG_IOKEY_ENABLE					ENABLE_THIS_MOUDLE //是否使能IO按键
#endif
#define TCFG_IOKEY_POWER_CONNECT_WAY		ONE_PORT_TO_LOW    //按键一端接低电平一端接IO

#define TCFG_IOKEY_POWER_ONE_PORT			IO_PORTB_01        //IO按键端口

// #define TCFG_IOKEY_PREV_CONNECT_WAY			ONE_PORT_TO_LOW  //按键一端接低电平一端接IO
// #define TCFG_IOKEY_PREV_ONE_PORT			IO_PORTB_00

// #define TCFG_IOKEY_NEXT_CONNECT_WAY 		ONE_PORT_TO_LOW  //按键一端接低电平一端接IO
// #define TCFG_IOKEY_NEXT_ONE_PORT			IO_PORTB_02
//*********************************************************************************//
//                                 adkey 配置                                      //
//*********************************************************************************//
#ifndef TCFG_ADKEY_ENABLE
#define TCFG_ADKEY_ENABLE                   DISABLE_THIS_MOUDLE//是否使能AD按键
#endif
#define TCFG_ADKEY_LED_IO_REUSE				DISABLE_THIS_MOUDLE	//ADKEY 和 LED IO复用，led只能设置蓝灯显示
#define TCFG_ADKEY_IR_IO_REUSE				DISABLE_THIS_MOUDLE	//ADKEY 和 红外IO复用
#define TCFG_ADKEY_LED_SPI_IO_REUSE			DISABLE_THIS_MOUDLE	//ADKEY 和 LED SPI IO复用


#define TCFG_ADKEY_PORT                     IO_PORTB_01         //AD按键端口(需要注意选择的IO口是否支持AD功能)
#define TCFG_ADKEY_AD_CHANNEL               AD_CH_PB1
#define TCFG_ADKEY_EXTERN_UP_ENABLE         ENABLE_THIS_MOUDLE //是否使用外部上拉
#define R_UP    220                 //22K，外部上拉阻值在此自行设置

//必须从小到大填电阻，没有则同VDDIO,填0x3ffL
#define TCFG_ADKEY_AD0      (0)                                 //0R
#define TCFG_ADKEY_AD1      (0x3ffL * 30   / (30   + R_UP))     //3k
#define TCFG_ADKEY_AD2      (0x3ffL * 62   / (62   + R_UP))     //6.2k
#define TCFG_ADKEY_AD3      (0x3ffL * 91   / (91   + R_UP))     //9.1k
#define TCFG_ADKEY_AD4      (0x3ffL * 150  / (150  + R_UP))     //15k
#define TCFG_ADKEY_AD5      (0x3ffL * 240  / (240  + R_UP))     //24k
#define TCFG_ADKEY_AD6      (0x3ffL * 330  / (330  + R_UP))     //33k
#define TCFG_ADKEY_AD7      (0x3ffL * 510  / (510  + R_UP))     //51k
#define TCFG_ADKEY_AD8      (0x3ffL * 1000 / (1000 + R_UP))     //100k
#define TCFG_ADKEY_AD9      (0x3ffL * 2200 / (2200 + R_UP))     //220k
#define TCFG_ADKEY_VDDIO    (0x3ffL)

#define TCFG_ADKEY_VOLTAGE0 ((TCFG_ADKEY_AD0 + TCFG_ADKEY_AD1) / 2)
#define TCFG_ADKEY_VOLTAGE1 ((TCFG_ADKEY_AD1 + TCFG_ADKEY_AD2) / 2)
#define TCFG_ADKEY_VOLTAGE2 ((TCFG_ADKEY_AD2 + TCFG_ADKEY_AD3) / 2)
#define TCFG_ADKEY_VOLTAGE3 ((TCFG_ADKEY_AD3 + TCFG_ADKEY_AD4) / 2)
#define TCFG_ADKEY_VOLTAGE4 ((TCFG_ADKEY_AD4 + TCFG_ADKEY_AD5) / 2)
#define TCFG_ADKEY_VOLTAGE5 ((TCFG_ADKEY_AD5 + TCFG_ADKEY_AD6) / 2)
#define TCFG_ADKEY_VOLTAGE6 ((TCFG_ADKEY_AD6 + TCFG_ADKEY_AD7) / 2)
#define TCFG_ADKEY_VOLTAGE7 ((TCFG_ADKEY_AD7 + TCFG_ADKEY_AD8) / 2)
#define TCFG_ADKEY_VOLTAGE8 ((TCFG_ADKEY_AD8 + TCFG_ADKEY_AD9) / 2)
#define TCFG_ADKEY_VOLTAGE9 ((TCFG_ADKEY_AD9 + TCFG_ADKEY_VDDIO) / 2)

#define TCFG_ADKEY_VALUE0                   0
#define TCFG_ADKEY_VALUE1                   1
#define TCFG_ADKEY_VALUE2                   2
#define TCFG_ADKEY_VALUE3                   3
#define TCFG_ADKEY_VALUE4                   4
#define TCFG_ADKEY_VALUE5                   5
#define TCFG_ADKEY_VALUE6                   6
#define TCFG_ADKEY_VALUE7                   7
#define TCFG_ADKEY_VALUE8                   8
#define TCFG_ADKEY_VALUE9                   9
//*********************************************************************************//
//                                  NTC配置                                       //
//*********************************************************************************//
#define NTC_DET_EN  					    DISABLE_THIS_MOUDLE
#define NTC_POWER_IO   						IO_PORTC_04
#define NTC_DETECT_IO   					IO_PORTC_05
#define NTC_DET_AD_CH   					(AD_CH_PC5)   //根据adc_api.h修改通道号

#define NTC_DET_UPPER        				799  //正常范围AD值上限，0度时
#define NTC_DET_LOWER        				297  //正常范围AD值下限，45度时

//*********************************************************************************//
//                                  充电参数配置                                   //
//*********************************************************************************//
#ifndef TCFG_CHARGE_ENABLE
#define TCFG_CHARGE_ENABLE					DISABLE_THIS_MOUDLE
#endif
#define TCFG_TEST_BOX_ENABLE				ENABLE_THIS_MOUDLE
#ifndef TCFG_CHARGESTORE_PORT
#define TCFG_CHARGESTORE_PORT				IO_PORTP_00
#endif
//是否支持开机充电
#define TCFG_CHARGE_POWERON_ENABLE			DISABLE
//是否支持拔出充电自动开机功能
#define TCFG_CHARGE_OFF_POWERON_NE			DISABLE
#define TCFG_CHARGE_FULL_V					CHARGE_FULL_V_4199
#ifndef TCFG_CHARGE_FULL_MA
#define TCFG_CHARGE_FULL_MA					CHARGE_FULL_mA_15
#endif
/*恒流充电电流可选配置*/
#ifndef TCFG_CHARGE_MA
#define TCFG_CHARGE_MA						CHARGE_mA_60
#endif
/*涓流充电电流配置*/
#ifndef TCFG_CHARGE_TRICKLE_MA
#define TCFG_CHARGE_TRICKLE_MA              CHARGE_mA_10
#endif

//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
#ifndef TCFG_LOWPOWER_POWER_SEL
#define TCFG_LOWPOWER_POWER_SEL				PWR_LDO15                   //电源模式设置，可选DCDC和LDO
#endif

//*********************************************************************************//
//                                 Audio配置                                       //
//*********************************************************************************//
#define TCFG_AUDIO_ADC_ENABLE				ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_ADC_LINE_CHA				AUDIO_ADC_LINE0
#define TCFG_AUDIO_ADC_MIC_CHA				AUDIO_ADC_MIC_0
/*MIC LDO电流档位设置：
    0:0.625ua    1:1.25ua    2:1.875ua    3:2.5ua*/
#define TCFG_AUDIO_ADC_LDO_SEL				3

#define TCFG_AUDIO_DAC_ENABLE				ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_DAC_LDO_SEL				1
/*
            DAC模式选择
#define DAC_MODE_L_DIFF          (0)  // 低压差分模式   , 适用于低功率差分耳机  , 输出幅度 0~2Vpp
#define DAC_MODE_H1_DIFF         (1)  // 高压1档差分模式, 适用于高功率差分耳机  , 输出幅度 0~3Vpp
#define DAC_MODE_H1_SINGLE       (2)  // 高压1档单端模式, 适用于高功率单端PA音箱, 输出幅度 0~1.5Vpp
#define DAC_MODE_H2_DIFF         (3)  // 高压2档差分模式, 适用于高功率差分PA音箱, 输出幅度 0~5Vpp
#define DAC_MODE_H2_SINGLE       (4)  // 高压2档单端模式, 适用于高功率单端PA音箱, 输出幅度 0~2.5Vpp
*/
#define TCFG_AUDIO_DAC_MODE         DAC_MODE_H1_SINGLE    // DAC_MODE_L_DIFF 低压， DAC_MODE_H1_DIFF 高压


/*
DACVDD电压设置(要根据具体的硬件接法来确定):
    DACVDD_LDO_1_20V        DACVDD_LDO_1_30V        DACVDD_LDO_2_35V        DACVDD_LDO_2_50V
    DACVDD_LDO_2_65V        DACVDD_LDO_2_80V        DACVDD_LDO_2_95V        DACVDD_LDO_3_10V*/
#define TCFG_AUDIO_DAC_LDO_VOLT				DACVDD_LDO_2_80V
/*预留接口，未使用*/
#define TCFG_AUDIO_DAC_PA_PORT				NO_CONFIG_PORT
/*
DAC硬件上的连接方式,可选的配置：
    DAC_OUTPUT_MONO_L               左声道
    DAC_OUTPUT_MONO_R               右声道
    DAC_OUTPUT_LR                   立体声
*/
#define TCFG_AUDIO_DAC_CONNECT_MODE    DAC_OUTPUT_MONO_LR_DIFF

#define AUDIO_OUT_WAY_TYPE           AUDIO_WAY_TYPE_DAC
#define LINEIN_INPUT_WAY            LINEIN_INPUT_WAY_ANALOG

#define AUDIO_OUTPUT_AUTOMUTE       0//ENABLE

#define  DUT_AUDIO_DAC_LDO_VOLT   				DACVDD_LDO_1_25V

//每个解码通道都开启数字音量管理,音量类型为VOL_TYPE_DIGGROUP时要使能
#define SYS_DIGVOL_GROUP_EN     DISABLE

#define SYS_VOL_TYPE            VOL_TYPE_DIGITAL


/*
 *通话的时候使用数字音量
 *0：通话使用和SYS_VOL_TYPE一样的音量调节类型
 *1：通话使用数字音量调节，更加平滑
 */
#define TCFG_CALL_USE_DIGITAL_VOLUME		0

// 使能改宏，提示音音量使用music音量
#define APP_AUDIO_STATE_WTONE_BY_MUSIC      (1)
// 0:提示音不使用默认音量； 1:默认提示音音量值
#define TONE_MODE_DEFAULE_VOLUME            (0)

/*MIC模式配置:单端隔直电容模式/差分隔直电容模式/单端省电容模式*/
/*
 *默认使用差分麦， 对底噪有明显优化， 对于驻极体mic和模拟硅mic JL701N同封装芯片由MIC2_BIAS提供电压
 *如果是动圈麦， 可以不使能MIC2_BIAS， 可以在板级C文件将audio_adc_mic_ldo_en调用注释掉
 *如果使用单端麦，则配置AUDIO_MIC_CAP_MODE即可
 * */
#define TCFG_AUDIO_MIC_MODE					AUDIO_MIC_CAP_DIFF_MODE
#define TCFG_AUDIO_MIC1_MODE				AUDIO_MIC_CAP_MODE
#define TCFG_AUDIO_MIC2_MODE				AUDIO_MIC_CAP_MODE
#define TCFG_AUDIO_MIC3_MODE				AUDIO_MIC_CAP_MODE

/*
    *>>MIC电源管理:根据具体方案，选择对应的mic供电方式
	 *(1)如果是多种方式混合，则将对应的供电方式或起来即可，比如(MIC_PWR_FROM_GPIO | MIC_PWR_FROM_MIC_BIAS)
	  *(2)如果使用固定电源供电(比如dacvdd)，则配置成DISABLE_THIS_MOUDLE
	   */
#define MIC_PWR_FROM_GPIO		(1UL << 0)	//使用普通IO输出供电
#define MIC_PWR_FROM_MIC_BIAS	(1UL << 1)	//使用内部mic_ldo供电(有上拉电阻可配)
#define MIC_PWR_FROM_MIC_LDO	(1UL << 2)	//使用内部mic_ldo供电
//配置MIC电源
#define TCFG_AUDIO_MIC_PWR_CTL				MIC_PWR_FROM_MIC_BIAS

//使用内部mic_ldo供电(有上拉电阻可配)
#if (TCFG_AUDIO_MIC_PWR_CTL & MIC_PWR_FROM_MIC_BIAS)
#define TCFG_AUDIO_MIC0_BIAS_EN				ENABLE_THIS_MOUDLE/*Port:PA2*/
#define TCFG_AUDIO_MIC1_BIAS_EN				ENABLE_THIS_MOUDLE/*Port:PA4*/
#define TCFG_AUDIO_MIC2_BIAS_EN				ENABLE_THIS_MOUDLE/*Port:PG7*/
#define TCFG_AUDIO_MIC3_BIAS_EN				ENABLE_THIS_MOUDLE/*Port:PG5*/
#endif/*MIC_PWR_FROM_MIC_BIAS*/

//使用内部mic_ldo供电(Port:PA0)
#if (TCFG_AUDIO_MIC_PWR_CTL & MIC_PWR_FROM_MIC_LDO)
#define TCFG_AUDIO_MIC_LDO_EN				ENABLE_THIS_MOUDLE
#endif/*MIC_PWR_FROM_MIC_LDO*/
/*>>MIC电源管理配置结束*/


/*
 *支持省电容MIC模块
 *(1)要使能省电容mic,首先要支持该模块:TCFG_SUPPORT_MIC_CAPLESS
 *(2)只有支持该模块，才能使能该模块:TCFG_MIC_CAPLESS_ENABLE
 */
#define TCFG_SUPPORT_MIC_CAPLESS			ENABLE_THIS_MOUDLE
//省电容MIC使能
#define TCFG_MIC_CAPLESS_ENABLE				DISABLE_THIS_MOUDLE
//省电容MIC1使能
#define TCFG_MIC1_CAPLESS_ENABLE			DISABLE_THIS_MOUDLE


#if	(WIRELESS_MIC_STEREO_EN)	//701N 使用linein，adcgain建议使用4，音频指标比较好
#define WIRELESS_MIC_ADC_GAIN				(4)//mic增益设置
#else
#define WIRELESS_MIC_ADC_GAIN				(7)//mic增益设置
#endif

#if (WIRELESS_CODING_SAMPLERATE == 44100)
#define WIRELESS_MIC_ADC_POINT_UNIT			(48000*WIRELESS_CODING_FRAME_LEN/10000)
#else
#define WIRELESS_MIC_ADC_POINT_UNIT			(WIRELESS_CODING_SAMPLERATE*WIRELESS_CODING_FRAME_LEN/10000)
#endif


//*********************************************************************************//
//                                  EQ配置                                         //
//*********************************************************************************//
#define TCFG_EQ_ENABLE						DISABLE_THIS_MOUDLE
#define TCFG_EQ_ONLINE_ENABLE               0     //支持在线EQ调试, 如果使用蓝牙串口调试，需要打开宏 APP_ONLINE_DEBUG，否则，默认使用uart调试(二选一)
#define TCFG_PHONE_EQ_ENABLE				DISABLE_THIS_MOUDLE
#define EQ_SECTION_MAX                      10    //eq段数
#define TCFG_USE_EQ_FILE                    1    //离线eq使用配置文件还是默认系数表 1：使用文件  0 使用默认系数表
#define TCFG_AEC_DCCS_EQ_ENABLE				TCFG_MIC_CAPLESS_ENABLE//省电容mic需要加dccs直流滤波
/*省电容mic通过eq模块实现去直流滤波*/
#if (TCFG_SUPPORT_MIC_CAPLESS && (TCFG_MIC_CAPLESS_ENABLE || TCFG_MIC1_CAPLESS_ENABLE))
#if ((TCFG_EQ_ENABLE == 0) || (TCFG_AEC_DCCS_EQ_ENABLE == 0))
#error "MicCapless enable,Please enable TCFG_EQ_ENABLE and TCFG_AEC_DCCS_EQ_ENABLE"
#endif
#endif
//*********************************************************************************//
//                          新音箱配置工具 && 调音工具                             //
//*********************************************************************************//
#define TCFG_EFFECT_TOOL_ENABLE				DISABLE		  	//是否支持在线音效调试,使能该项还需使能EQ总使能TCFG_EQ_ENABL,
#define TCFG_NULL_COMM						0				//不支持通信
#define TCFG_UART_COMM						1				//串口通信
#define TCFG_USB_COMM						2				//USB通信
#if (TCFG_CFG_TOOL_ENABLE || TCFG_EFFECT_TOOL_ENABLE)
#define TCFG_COMM_TYPE						TCFG_UART_COMM	//通信方式选择
#else
#define TCFG_COMM_TYPE						TCFG_NULL_COMM
#endif
#define TCFG_TOOL_TX_PORT					IO_PORT_DP      //UART模式调试TX口选择
#define TCFG_TOOL_RX_PORT					IO_PORT_DM      //UART模式调试RX口选择
#define TCFG_ONLINE_ENABLE                  (TCFG_EFFECT_TOOL_ENABLE)    //是否支持音效在线调试功能


//*********************************************************************************//
//                                  PWM_LED 配置                                       //
//******************************************************************************
#ifndef TCFG_PWMLED_ENABLE
#define TCFG_PWMLED_ENABLE					ENABLE_THIS_MOUDLE			//是否支持PMW LED推灯模块
#endif
#define TCFG_PWMLED_IOMODE					LED_ONE_IO_MODE				//LED模式，单IO还是两个IO推灯
#define TCFG_PWMLED_PIN						IO_PORTB_06					//LED使用的IO口 注意和led7是否有io冲突

//*********************************************************************************//
//                                  系统配置                                         //
//*********************************************************************************//
#define TCFG_AUTO_SHUT_DOWN_TIME		    0   //没有蓝牙连接自动关机时间
#define TCFG_SYS_LVD_EN						1   //电量检测使能
#define TCFG_POWER_ON_NEED_KEY				0	  //是否需要按按键开机配置
#if TCFG_PC_ENABLE
#define TCFG_LOWPOWER_VDDIOM_LEVEL			VDDIOM_VOL_32V             //VDDIO 设置的值要和vbat的压差要大于300mv左右，否则会出现DAC杂音
#endif
#define TCFG_LOWPOWER_OSC_TYPE              OSC_TYPE_LRC		//低功耗晶振类型，btosc/lrc

//*********************************************************************************//
//                                  蓝牙配置                                       //
//*********************************************************************************//
#if TCFG_RF_TEST_EN
#define TCFG_USER_BT_CLASSIC_ENABLE         1   //经典蓝牙功能使能
#endif

#define TCFG_USER_BLE_ENABLE                1   //BLE功能使能
#define BT_FOR_APP_EN                       0

#if TCFG_USER_BLE_ENABLE
//BLE多连接,多开注意RAM的使用
#define TRANS_MULTI_BLE_EN                  0   //蓝牙BLE多连:1主1从,或者2主
#define TRANS_MULTI_BLE_SLAVE_NUMS          0   //range(0~1)
#define TRANS_MULTI_BLE_MASTER_NUMS         0   //range(0~2)
#endif

#define BLE_WIRELESS_CLIENT_EN          1
//wifi抗干扰
#define TCFG_WIFI_DETECT_ENABLE				DISABLE
#define TCFG_WIFI_DETCET_PRIOR				DISABLE//duplex server收数多 从机优先
#define WIRELESS_24G_CODE_ID                10
#define TCFG_SERVER_RX_PAYLOAD_LEN			105

#if (WIRELESS_EARPHONE_MIC_EN)
#define WIRELESS_CLK						160
#else
#define WIRELESS_CLK						144
#endif

//<!以下门限参数可调， 降噪延时适当调大一些， 非降噪门限可以调低一些， 对延时要求高的话， 可以适当调低， 但不建议低于10
//<!信号好， 低延时门限
#if WIRELESS_LOW_LATENCY
#define WL_LOW_DELAY_LIMIT              9
#else
#define WL_LOW_DELAY_LIMIT              14
#endif
//<!信号不好，高延时门限
#define WL_HIGH_DELAY_LIMIT             WL_LOW_DELAY_LIMIT+6
//*********************************************************************************//
//                                  encoder && decoder 配置                                   //
//*********************************************************************************//

#define TCFG_MEDIA_LIB_USE_MALLOC										1

#define TCFG_DEC_JLA_ENABLE				    ENABLE
#define TCFG_ENC_JLA_ENABLE				    ENABLE
//#define WIRELESS_CODING_CHANNEL_NUM		2

#if	(WIRELESS_MIC_STEREO_EN)
#define WIRELESS_CODING_CHANNEL_NUM		2 //编码通道通道数， 最大为2
#else
#define WIRELESS_CODING_CHANNEL_NUM		2 //编码通道通道数， 最大为2
#endif

#define WIRELESS_DECODE_CHANNEL_NUM		1
// 64~128K
#define WIRELESS_CODING_BIT_RATE		(116000)    //默认128k音频码率,音频压缩比码率越低音频压缩约高音质越差发送的空中数据就越少

#define TCFG_DEV_MANAGER_ENABLE											0

//*********************************************************************************//
//                                 配置结束                                         //
//*********************************************************************************//



#endif //CONFIG_BOARD_WIRELESS_DUPLEX_DONGLE
#endif //CONFIG_BOARD_WIRELESS_DUPLEX_DONGLE_CFG_H
