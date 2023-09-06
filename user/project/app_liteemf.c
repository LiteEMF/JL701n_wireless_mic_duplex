/*********************************************************************************************
    *   Filename        : app_keyboard.c

    *   Description     :

    *   Author          :

    *   Email           :

    *   Last modifiled  : 2019-07-05 10:09

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "server/server_core.h"
#include "app_action.h"
#include "app_config.h"

#if(CONFIG_APP_LITEEMF || 1)
#include "system/app_core.h"
#include "os/os_api.h"
#include "rcsp_bluetooth.h"
#include "update_loader_download.h"
#include "app/emf.h"
#include "api/api_log.h"
#include  "app/app_command.h"
#if API_USBD_BIT_ENABLE
#include "api/usb/device/usbd.h"
#endif
#if API_USBH_BIT_ENABLE
#include "api/usb/host/usbh.h"
#endif
#if API_OTG_BIT_ENABLE
#include "api/usb/api_otg.h"
#endif
#include "adapter_process.h"
/******************************************************************************************************
** Defined
*******************************************************************************************************/
// #define    LITEEMF_APP           /*使用LITEEMFP APP*/


#define     TASK_NAME               "emf_task"
#define     HEARTBEAT_EVENT         0x01000000  
#define     UART_RX_EVENT           0x02000000       //低24bit 作为参数
#define     BT_REC_DATA_EVENT       0x03000000       //bt_t len

/******************************************************************************************************
**	public Parameters
*******************************************************************************************************/
static volatile u8 is_app_liteemf_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗

volatile uint8_t heartbeat_msg_cnt = 1;
/*****************************************************************************************************
**  Function
******************************************************************************************************/
void api_timer_hook(uint8_t id)
{
    if(0 == id){
        m_systick++;

        if(0 == heartbeat_msg_cnt){
            int err = os_taskq_post_msg(TASK_NAME, 1, HEARTBEAT_EVENT);
            if(err){
                logd("heatbeat post err%d!!!\n",err);
            }else{
                heartbeat_msg_cnt = 1;
            }
        }
        
    }
}





extern u32 get_jl_rcsp_update_status();
static u32 check_ota_mode()
{
    if (UPDATE_MODULE_IS_SUPPORT(UPDATE_APP_EN)) {
        #if RCSP_UPDATE_EN
        if (get_jl_rcsp_update_status()) {
            r_printf("OTA ing");
            set_run_mode(OTA_MODE);
            usb_sie_close_all();//关闭usb
            JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);//关闭串口
            return 1;
        }
        #endif
    }
    return 0;
}


bool rf_command_send(uint8_t cmd, uint8_t*pbuf, uint16_t len)
{
    bool ret = false;
    #if BT0_SUPPORT & (BIT_ENUM(TR_RF) | BIT_ENUM(TR_RFC))
    api_bt_ctb_t* rf_ctbp;
    trp_handle_t rf_handle = {TR_RF, BT_ID0, U16(DEV_TYPE_VENDOR,0)};
    #if BT0_SUPPORT & BIT_ENUM(TR_RF)
    rf_ctbp = api_bt_get_ctb(BT_RF);
    #endif
    #if BT0_SUPPORT & BIT_ENUM(TR_RFC)
    rf_ctbp = api_bt_get_ctb(BT_RFC);
    rf_handle.trp = BT_RFC;
    #endif

    if(rf_ctbp->sta == BT_STA_READY){
        ret = api_command_tx(&rf_handle,cmd, pbuf, len);
    }
    #endif

    return ret;
}

#if BT0_SUPPORT & BIT_ENUM(TR_RF)
extern s32 cur_music_vol;
extern s32 cur_mic_vol;
void adapter_music_vol(uint16_t music_vol, uint16_t mic_vol)
{
    if (cur_music_vol != music_vol) {
        cur_music_vol = music_vol;
        adapter_process_event_notify(ADAPTER_EVENT_SET_MUSIC_VOL, music_vol);
    }
    if (cur_mic_vol != mic_vol) {
        cur_mic_vol = mic_vol;
        adapter_process_event_notify(ADAPTER_EVENT_SET_MIC_VOL, mic_vol);
    }
}
#endif

#if BT0_SUPPORT & BIT_ENUM(TR_RFC)
void rfc_info_handle(void)
{
    #if BT0_SUPPORT & BIT_ENUM(TR_RFC)
    static timer_t rf_timer;
    static uint16_t s_vol_l=0, s_vol_r=0, s_vol_mic=0;

    uint8_t buf[3];
    uint16_t vol_l, vol_r, vol_mic;


    if( (m_systick - rf_timer) >= 50){
        rf_timer = m_systick;

        uac_get_cur_vol(0, &vol_l, &vol_r);
        vol_mic = uac_get_mic_vol(0);

        //同步状态
        if((s_vol_l != vol_l) || (s_vol_r != vol_r) || (s_vol_mic != vol_mic)){
            buf[0] = (u8)vol_l;
            buf[1] = (u8)vol_r;
            buf[2] = (u8)vol_mic;
            if(rf_command_send(CMD_MUSIC_VOL,buf, 3)){
                s_vol_l = vol_l;
                s_vol_r = vol_r;
                s_vol_mic = vol_mic;
            }
        }else{
            s_vol_l=0;
            s_vol_r=0;
            s_vol_mic=0;
        }
    }
    
    #endif
}
#endif


error_t os_post_buf_msg(uint32_t evt,uint8_t* buffer,uint16_t length)	//如果使用os,并且需要发送消息通知任务开启时使用
{
	uint8_t* p;
	uint8_t err;
    uint32_t event;
	if(0 == length) return ERROR_LENGTH;

	p = malloc(length);
	if(NULL != p){
		memcpy(p,buffer, length);
	}
    event = evt | length;
	err = os_taskq_post_msg("emf_task", 2, event, p);
	if(err){
		free(p);
		loge("msg event %x err=%d\n",event,err);
		return ERROR_NO_MEM;
	}

	return ERROR_SUCCESS;
}

error_t os_bt_rx(uint8_t id,bt_t bt, bt_evt_rx_t* pa)	
{
    if(NULL != pa){
        return os_post_buf_msg(BT_REC_DATA_EVENT | ((uint32_t)id<<20) | ((uint32_t)bt<<16), pa->buf,pa->len);
    }
	return ERROR_SUCCESS;
}



/*******************************************************************
** Parameters:		
** Returns:	
** Description:	task	
*******************************************************************/
static void emf_task_handle(void *arg)
{
    uint8_t *p;
    int ret = 0;
    int msg[16];

    while (1) {
        ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        if (msg[0] != Q_MSG) {
            continue;
        }

        switch (msg[1] & 0XFF000000) {
            case HEARTBEAT_EVENT:
                heartbeat_msg_cnt = 0;
                m_task_tick10us +=100;      //同步任务tick时钟
                
                #if BT0_SUPPORT & BIT_ENUM(TR_RFC)
                rfc_info_handle();
                #endif

                emf_handler(0);
                #if API_USBD_BIT_ENABLE     //无线升级时候关闭usb
                if (check_ota_mode()) {
                    usb_stop(0);
                }
                #endif
                break;
            case UART_RX_EVENT:
                break;
            case BT_REC_DATA_EVENT:
                bt_evt_rx_t pa;
                pa.bts = BT_UART;
                pa.len = msg[1] & 0XFFFF;
                pa.buf = (bt_evt_rx_t*)msg[2];
                api_bt_rx(msg[1]>>20, msg[1]>>16 & 0x0f,&pa);
                free(msg[2]);
                break;
            default:
                break;
        }
    }
}




/*******************************************************************
** Parameters:		
** Returns:	
** Description:		
*******************************************************************/
extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
void liteemf_app_start(void)
{
    logi("=======================================");
    logi("-------------LITEMEF DEMO-----------------");
    logi("=======================================");
    logi("app_file: %s", __FILE__);

    // clk_set("sys", 96 * 1000000L);		//fix clk
    // clk_set("sys", BT_NORMAL_HZ);

    emf_api_init();
    emf_init();

    logd("start end\n");
    os_task_create_affinity_core(emf_task_handle,NULL,2,2048,512,"emf_task",EMF_CPU);
    // task_create(emf_task_handle, NULL, TASK_NAME);
    heartbeat_msg_cnt = 0;

    #if API_BT_ENABLE || API_RF_ENABLE
    api_bt_init();
    #endif
}

#ifdef LITEEMF_APP

/*******************************************************************
** Parameters:		
** Returns:	
** Description:	app  状态处理	
*******************************************************************/
static int liteemf_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_LITEEMF_MAIN:
            liteemf_app_start();
            break;
        }
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        break;
    case APP_STA_DESTROY:
        logi("APP_STA_DESTROY\n");
        break;
    }

    return 0;
}
/*******************************************************************
** Parameters:		
** Returns:	
** Description:	app 线程事件处理
*******************************************************************/
static int liteemf_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_BT_EVENT:
        #if API_BT_ENABLE
        sys_bt_event_handler(event);
        #endif
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, NULL);    //set_soft_poweroff_call 可以传入关机函数使用系统电池检测
        }
        #if TCFG_CHARGE_ENABLE
        else if ((u32)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            app_charge_event_handler(&event->u.dev);
        }
        #endif
        return 0;

    default:
        return 0;
    }

    return 0;
}
/*******************************************************************
** Parameters:		
** Returns:	
** Description:	注册控制是否进入sleep
*******************************************************************/
//system check go sleep is ok
static u8 liteemf_app_idle_query(void)
{
    return !is_app_liteemf_active;
}
REGISTER_LP_TARGET(app_liteemf_lp_target) = {
    .name = "app_liteemf_deal",
    .is_idle = liteemf_app_idle_query,
};


static const struct application_operation app_liteemf_ops = {
    .state_machine  = liteemf_state_machine,
    .event_handler 	= liteemf_event_handler,
};

/*
 * 注册模式
 */
REGISTER_APPLICATION(app_liteemf) = {
    .name 	= "app_liteemf",
    .action	= ACTION_LITEEMF_MAIN,
    .ops 	= &app_liteemf_ops,
    .state  = APP_STA_DESTROY,
};
#endif

#endif

