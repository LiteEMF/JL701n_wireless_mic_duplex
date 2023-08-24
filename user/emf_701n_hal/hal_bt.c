/* 
*   BSD 2-Clause License
*   Copyright (c) 2022, LiteEMF
*   All rights reserved.
*   This software component is licensed by LiteEMF under BSD 2-Clause license,
*   the "License"; You may not use this file except in compliance with the
*   License. You may obtain a copy of the License at:
*       opensource.org/licenses/BSD-2-Clause
* 
*/

#include "hw_config.h"
#if API_BT_ENABLE
#include "api/bt/api_bt.h"
#include "api/api_log.h"

#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "config/config_transport.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_common.h"
#include "le_common.h"
#include "rcsp_bluetooth.h"
#include "app_comm_bt.h"
#include "adapter_media.h"
#include "adapter_process.h"
/*******************************************************************************************************************
**	Hardware  Defined
********************************************************************************************************************/


/*******************************************************************************************************************
**	public Parameters
********************************************************************************************************************/
extern uint8_t sdk_rf_tx_len;
#if BT0_SUPPORT & BIT_ENUM(TR_RF)
extern uint8_t sdk_rf_tx_buf[RF_TX_LL_MTU];
#elif BT0_SUPPORT & BIT_ENUM(TR_RFC)
extern uint8_t sdk_rf_tx_buf[RFC_TX_LL_MTU];
#endif

/*******************************************************************************************************************
**	static Parameters
********************************************************************************************************************/
#if BT0_SUPPORT & BIT_ENUM(TR_BLE)
static const ble_init_cfg_t ble_default_config = {
    .same_address = 0,
    .appearance = BLE_ICON,
    .report_map = NULL,         //hid map 在后面设置
    .report_map_size = 0,
};
#endif


//选择蓝牙从机模式
extern void user_ble_pri_or_normal_sw(u8 status);
extern u8 flag_user_private;


//BLE 从机接收数据
void ble_hid_transfer_channel_recieve(uint8_t* p_attrib_value,uint16_t length)
{
    bt_evt_rx_t evt;
    evt.bts = BT_HID;
	evt.buf = p_attrib_value;
	evt.len = length;
    if(length) api_bt_event(BT_ID0,BT_BLE,BT_EVT_RX,&evt);    
}
/*****************************************************************************************************
**  hal bt Function
******************************************************************************************************/
bool hal_bt_select_mode(uint8_t id, uint16_t trps)
{
    if(BT_ID0 != id) return false;
    
    logi("bt trps %x\n", trps);

    if(trps & BT0_SUPPORT & BIT(TR_RF)){
        logi("---------app select 24g--------\n");
        if(!flag_user_private){
            user_ble_pri_or_normal_sw(1);
        }
        
    }else if(trps & BT0_SUPPORT & BIT(TR_BLE)) {
        logi("---------app select ble--------\n");
        if(flag_user_private){
            user_ble_pri_or_normal_sw(0);
        }
    }else{
        hal_bt_enable(BT_ID0, BT_BLE, 0);
    }

}

bool hal_bt_get_mac(uint8_t id, bt_t bt, uint8_t *buf )
{
    bool ret = false;

    if(BT_ID0 != id) return false;
	
	memcpy(buf,bt_get_mac_addr(),6);        //获取基础mac地址, EDR地址为基地址
	
    return ret;
}

bool hal_bt_is_bonded(uint8_t id, bt_t bt)
{
    bool ret = false;

    if(BT_ID0 != id) return false;
    ret = bt_ble_is_bonded();
    return ret;
}
bool hal_bt_debond(uint8_t id, bt_t bt)
{
    bool ret = true;
    api_bt_ctb_t* bt_ctbp = api_bt_get_ctb(bt);

    if(BT_ID0 != id) return false;

    clear_bonding_info();
	ble_module_enable(0);
    ble_module_enable(1);
    return ret;
}
bool hal_bt_disconnect(uint8_t id, bt_t bt)
{
    bool ret = true;

    if(BT_ID0 != id) return false;

    ble_app_disconnect();

    return ret;
}
bool hal_bt_enable(uint8_t id, bt_t bt,bool en)
{
    bool ret = true;

    if(BT_ID0 != id) return false;

    ble_module_enable(en);

	#if BT0_SUPPORT & (BIT_ENUM(TR_BLE) | BIT_ENUM(TR_RF))
	if(!en != adapter_media_sel){		//adapter_media_sel 0: RF模式,1: usb模式
		adapter_media_sel = !en;
		adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_CLOSE, 0);
		adapter_process_event_notify(ADAPTER_EVENT_ODEV_MEDIA_OPEN, 0);
		adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_CLOSE, 0);
		adapter_process_event_notify(ADAPTER_EVENT_IDEV_MEDIA_OPEN, 0);
	}
	#endif
    
    return ret;
}
bool hal_bt_uart_tx(uint8_t id, bt_t bt,uint8_t *buf, uint16_t len)
{
    bool ret = false;

    if(BT_ID0 != id) return false;
    switch(bt){
    #if BT0_SUPPORT & (BIT_ENUM(TR_BLE) | BIT_ENUM(TR_BLE_RF))
    case BT_BLE: 	
    case BT_BLE_RF: 			//BLE模拟2.4G
        ret = (0 == ble_hid_transfer_channel_send(buf, len));
        break;
    #endif
    #if BT0_SUPPORT & (BIT_ENUM(TR_RF) | BIT_ENUM(TR_RFC))
    case BT_RF:
        sdk_rf_tx_len = MIN(len, sizeof(sdk_rf_tx_buf));
        memcpy(sdk_rf_tx_buf, buf, sdk_rf_tx_len);      //拷贝当前发送
        ret = true;
        break;
    #endif
    }   
    
    return ret;
}
bool hal_bt_hid_tx(uint8_t id, bt_t bt,uint8_t*buf, uint16_t len)
{
    bool ret = false;

    if(BT_ID0 != id) return false;
    switch(bt){
    #if BT0_SUPPORT & (BIT_ENUM(TR_BLE) | BIT_ENUM(TR_BLE_RF))
    case BT_BLE: 	
    case BT_BLE_RF: 			//BLE模拟2.4G
        ret = (0 == ble_hid_data_send(buf[0], buf+1, len-1));
        break;
    #endif
    }   
    
    return ret;
}



#if BT_HID_SUPPORT
uint8_t *ble_report_mapp = NULL;
uint8_t *edr_report_mapp = NULL;
static void user_hid_set_reportmap (bt_t bt)
{
    api_bt_ctb_t* bt_ctbp;
	bt_ctbp = api_bt_get_ctb(bt);

    if(bt_ctbp->types & BIT(DEV_TYPE_HID)){
        uint8_t i;
        uint8_t *hid_mapp, *report_mapp;
        uint16_t len, map_len = 0;

        //get report map len
        for(i=0; i<16; i++){
            if(bt_ctbp->hid_types & BIT(i)){
                len = get_hid_desc_map((trp_t)bt, i ,NULL);;
                map_len += len;
            }
        }

        if(BT_BLE == bt){
            free(ble_report_mapp);                  //防止多次调用内存溢出
            ble_report_mapp = malloc(map_len);
            report_mapp = ble_report_mapp;
        }else{
            free(edr_report_mapp);                  //防止多次调用内存溢出
            edr_report_mapp = malloc(map_len);
            report_mapp = edr_report_mapp;
        }

        if(NULL == report_mapp) return;
        for(i=0; i<16; i++){
            if(bt_ctbp->hid_types & BIT(i)){
                len = get_hid_desc_map((trp_t)bt, i ,&hid_mapp);
                memcpy(report_mapp, hid_mapp, len);
                report_mapp += len;
            }
        }
        
        if(BT_BLE == bt){
            report_mapp = ble_report_mapp;
            #if BT0_SUPPORT & BIT_ENUM(TR_BLE)
            le_hogp_set_ReportMap(ble_report_mapp, map_len);
            #endif
        }else{
            report_mapp = edr_report_mapp;
            #if TCFG_USER_EDR_ENABLE
            user_hid_set_ReportMap(edr_report_mapp, map_len);
            #endif
        }
        logd("bt%d report map %d: ",bt,map_len);dumpd(report_mapp, map_len);
    }
}
#endif


bool hal_bt_init(uint8_t id)
{
    bool ret = false;

    if(BT_ID0 != id) return false;

    #if BT0_SUPPORT & BIT_ENUM(TR_BLE)
    btstack_ble_start_before_init(&ble_default_config, 0);
    #if BLE_HID_SUPPORT
    user_hid_set_reportmap (BT_BLE);
    #endif
    #endif

    //set mac set name
    // bt_set_local_name(DEFAULT_NAME,sizeof(DEFAULT_NAME));			//设置rf名称
    logi("bt name: %s \n", DEFAULT_NAME);

    return ret;
}
bool hal_bt_deinit(uint8_t id)
{
    bool ret = false;

    if(BT_ID0 != id) return false;
    ble_module_enable(0);
    
    return ret;
}
void hal_bt_task(void* pa)
{
    UNUSED_PARAMETER(pa);
}


#endif

