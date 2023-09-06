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

/************************************************************************************************************
**	Description:	
************************************************************************************************************/
#include "hw_config.h"
#if defined GAMEPAD1 && GAMEPAD1
#include "app/emf.h"
#include "app/app_km.h"
#include "api/usb/host/usbh.h"
#include "api/usb/device/usbd.h"
#if APP_GAMEAPD_ENABLE
#include "app/gamepad/app_gamepad.h"
#endif
#include "adapter_process.h"

#include "api/api_log.h"

/******************************************************************************************************
** Defined
*******************************************************************************************************/

/******************************************************************************************************
**	public Parameters
*******************************************************************************************************/


/******************************************************************************************************
**	static Parameters
*******************************************************************************************************/
volatile bool m_memory_index = false;
volatile uint8_t m_memory_len=0,m_memory_buf[2][64];		//cmd + data

/*****************************************************************************************************
**	static Function
******************************************************************************************************/

/*****************************************************************************************************
**  Function
******************************************************************************************************/
extern void adapter_music_vol(uint16_t music_vol, uint16_t mic_vol);

const uint8_t led_channel[] = {0, 1, 2, 3, 4, 5, 10, 11, 12};
bool rgb_driver_show(uint8_t* frame, uint8_t size)
{
	bool ret = false;
    uint8_t brightness;
	uint8_t i;
    
    // logd("show:");dumpd(frame,size);
    // emf_mem_stats();
	for(i=0; i<size; i++){
        brightness = remap(frame[i], 0, 255, 0, 63);
        #ifdef HW_SPI_HOST_MAP
        ret = api_spi_host_write(0,led_channel[i], &brightness, 1);
        #endif
	}

	return ret;
}
bool rgb_driver_init(void)
{
	return true;
}
bool rgb_driver_deinit(void)
{
	return true;
}

#if API_USBH_BIT_ENABLE
void usbh_class_itf_alt_select(uint8_t id,usbh_class_t* pclass)
{
	usbh_dev_t* pdev = get_usbh_dev(id);
	usbh_class_t *pos;

	list_for_each_entry_type(pos,&pdev->class_list, usbh_class_t, list){
		if(pos->itf.if_num == pclass->itf.if_num){
			free_usbh_class(pclass);			//默认使用alt 0
			// free_usbh_class(pos);
			continue;				
		}
	}
}
#endif

#if API_USBD_BIT_ENABLE

char* usbd_user_get_string(uint8_t id, uint8_t index)
{
	char *pstr = NULL;

	if(2 == index){		//product string
		if(m_usbd_types[id] & BIT(DEV_TYPE_HID)){
			if(m_usbd_hid_types[id] & HID_SWITCH_MASK){
				pstr = "Pro Controller";
			}else if(m_usbd_hid_types[id] & HID_PS_MASK){
				pstr = "Wireless Controller";
			}else if(m_usbd_hid_types[id] & HID_XBOX_MASK){
				pstr = "Controller";
			}else if(m_usbd_hid_types[id] & BIT(HID_TYPE_GAMEPADE)){
				pstr = "Hid Gamepade";
			}else if(m_usbd_hid_types[id] & (BIT(HID_TYPE_MT) | BIT(HID_TYPE_TOUCH))){
				pstr = "Touch Screen";
			}else if(m_usbd_hid_types[id] & BIT(HID_TYPE_MOUSE)){
				pstr = "mouse";
			}else if(m_usbd_hid_types[id] & BIT(HID_TYPE_KB)){
				pstr = "keyboard";
			}else if(m_usbd_hid_types[id] & BIT(HID_TYPE_VENDOR)){
				pstr = "Hid Vendor";
			}
		}else if(m_usbd_types[id] & BIT(DEV_TYPE_IAP2)){
			pstr = "iap";
		}else if(m_usbd_types[id] & BIT(DEV_TYPE_MSD)){
			pstr = "msd";
		}else if(m_usbd_types[id] & BIT(DEV_TYPE_PRINTER)){
			pstr = "printer";
		}else if(m_usbd_types[id] & BIT(DEV_TYPE_CDC)){
			pstr = "cdc";
		}else if(m_usbd_types[id] & BIT(DEV_TYPE_AUDIO)){
			pstr = "uac";
		}else{
			pstr = (char*)usbd_string_desc[index];
		}
	}
		
	return pstr;
}
void usbd_user_set_device_desc(uint8_t id, usb_desc_device_t *pdesc)
{
	if(m_usbd_types[id] & BIT(DEV_TYPE_HID)){
		if(m_usbd_hid_types[id] & HID_SWITCH_MASK){
			#if USBD_HID_SUPPORT & HID_SWITCH_MASK
			pdesc->idVendor = SWITCH_VID;
			pdesc->idProduct = SWITCH_PID;
			#endif
		}else if(m_usbd_hid_types[id] & HID_PS_MASK){
			#if USBD_HID_SUPPORT & HID_PS_MASK
			pdesc->idVendor = PS_VID;
			if(m_usbd_hid_types[id] & BIT(HID_TYPE_PS3)){
				pdesc->idProduct = PS3_PID;
			}else{
				pdesc->idProduct = PS4_PID;
			}
			#endif
		}else if(m_usbd_hid_types[id] & BIT_ENUM(HID_TYPE_XBOX)){
			#if USBD_HID_SUPPORT & BIT_ENUM(HID_TYPE_XBOX)
			pdesc->idVendor = XBOX_VID;	//0xFF,0x47,0xD0
			pdesc->idProduct = XBOX_PID;
			pdesc->bDeviceClass       = 0xFF;
			pdesc->bDeviceSubClass    = 0x47;
			pdesc->bDeviceProtocol    = 0xD0;
			#endif
		}else if(m_usbd_hid_types[id] == BIT_ENUM(HID_TYPE_X360)){		//xinput 复合设备使用自定义vid
			#if USBD_HID_SUPPORT & BIT_ENUM(HID_TYPE_X360)
			pdesc->idVendor = XBOX_VID;	//0xFF,0x47,0xD0
			pdesc->idProduct = X360_PID;	//0xff, 0xff, 0xff
			pdesc->bDeviceClass       = 0xFF;
			pdesc->bDeviceSubClass    = 0xff;
			pdesc->bDeviceProtocol    = 0xff;
			#endif
		}
	}
}

#endif


#if BT0_SUPPORT & (BIT_ENUM(TR_RF) | BIT_ENUM(TR_RFC))

static uint8_t s_cmd_buf[32];
static uint8_t s_cmd_len = 0;
void api_bt_rx(uint8_t id,bt_t bt, bt_evt_rx_t* pa)
{
    // logd("weak bt%d rx:%d \n",bt,pa->len);    //dumpd(pa->buf,pa->len);

	#if BT0_SUPPORT & BIT_ENUM(TR_RF)
    uint8_t mtu = RF_CMD_MTU;
    #endif
    #if BT0_SUPPORT & BIT_ENUM(TR_RFC)
    uint8_t mtu = RFC_CMD_MTU;
    #endif

	
	api_bt_ctb_t* bt_ctbp;

	bt_ctbp = api_bt_get_ctb(bt);
	if(NULL == bt_ctbp) return;
	logd("r%d",pa->len);
// return;
	if(BT_UART == pa->bts){					//uart
		uint8_t i;
		command_rx_t rx;
		for(i=0; i<pa->len; i++){
			if(api_command_rx_byte(&rx, mtu, pa->buf[i], s_cmd_buf, &s_cmd_len)){
				logd("decode %d:",rx.len); dumpd(rx.pcmd, rx.len);
				switch(rx.pcmd[3]){
					case CMD_MUSIC_VOL:{
						#if BT0_SUPPORT & BIT_ENUM(TR_RF)
						uint16_t music_vol = (rx.pcmd[4] << 8) | rx.pcmd[5];
						uint16_t mic_vol = rx.pcmd[6] & 0xffff;
						adapter_music_vol(music_vol, mic_vol);
						#endif
						break;
					}
					default:
						break;
				}
				command_rx_free(&rx);
			}
		}
	}
}
#endif

/*******************************************************************
** Parameters:		
** Returns:		true: 用户自定义处理, false: 会走通用处理	
** Description: 蓝牙事件用户处理
*******************************************************************/
bool api_bt_event_weak(uint8_t id,bt_t bt, bt_evt_t const event, bt_evt_pa_t* pa)
{
	api_bt_ctb_t* bt_ctbp;

	if(id >= BT_ID_MAX) return false;
	bt_ctbp = api_bt_get_ctb(bt);
	if(NULL == bt_ctbp) return false;

	switch(event){
	case BT_EVT_TX:
		if((NULL != bt_ctbp->fifo_txp) && (NULL != pa)){
			if(0 == pa->tx.ret){
				uint16_t fifo_len = fifo_length(&bt_ctbp->fifo_txp->fifo);
				if(m_memory_len && (0 == fifo_len)){
					// m_memory_len = 0;
					rf_command_send(m_memory_buf[m_memory_index][0], &m_memory_buf[m_memory_index][1], m_memory_len-1);
				}
			}
		}
		break;
	}
	return false;
}


void app_key_vendor_scan(uint32_t *pkey)
{
	
}

void app_key_event(void)
{
	static bool debond = false;
	if(m_app_key.press_long & HW_KEY_HOME){
		if(!debond){
			debond = true;
			api_bt_debond(BT_ID0,BT_RF);
			api_bt_debond(BT_ID0,BT_RFC);
		}
	}else{
		debond = false;
	}

	if(m_app_key.pressed_b & HW_KEY_HOME){
		hal_bt_enable(BT_ID0, BT_RF,0);
	}
}


void hw_user_vender_init(void)
{
    uint8_t id;
	
	#if API_USBD_BIT_ENABLE
	for(id=0; id<USBD_NUM; id++){
		m_usbd_types[id] = USBD_TYPE_SUPPORT;
        m_usbd_hid_types[id] = USBD_HID_SUPPORT;
	}
	#endif

    logd("call hw_user_vender_init ok\n" );

}
void user_vender_init(void)	
{
    uint8_t i;
    logd("call user_vender_init ok\n" );


    // api_gpio_dir(PB_00, PIN_OUT,PIN_PULLNONE);
    // app_rumble_set_duty(RUMBLE_L, 0X80, 1000);
    // app_rumble_set_duty(RUMBLE_R, 0X80, 1000);
	
	#if APP_RGB_ENABLE
    for(i=0; i<APP_RGB_NUMS; i++){
        app_rgb_set_blink(i, Color_White, BLINK_SLOW);
    }
	#endif
	
}
void user_vender_deinit(void)			//关机前deinit
{
}

void user_vender_handler(void)
{
	//rx uart
    static timer_t timer;

    #ifdef HW_UART_MAP
    app_fifo_t *fifop = api_uart_get_rx_fifo(1);
    uint8_t c;
	command_rx_t rx;
	static uint8_t s_cmd_buf[UART_CMD_MTU];
	static uint8_t s_cmd_len = 0;
    
    while(ERROR_SUCCESS == app_fifo_get(fifop, &c)){
        // logd("%x",c);
		// if(api_command_rx_byte(&rx, UART_CMD_MTU, c, s_cmd_buf, &s_cmd_len)){
		// 	logd("uart cmd %d:",rx.len); dumpd(rx.pcmd, rx.len);
		// 	command_rx_free(&rx);
		// }
    }
    #endif

    // // adc test 
    // static timer_t adc_times = 0;
    // if(m_systick - adc_times >= 10){
    //     adc_times = m_systick;
    //     logd("adc:%4d,%4d,%4d,%4d,%4d,%4d\n",
	// 	api_adc_value(0),api_adc_value(1),api_adc_value(2),api_adc_value(3),api_adc_value(4),api_adc_value(5));
    // }


    //use test
	if(m_systick - timer >= 3000){
		timer = m_systick;

		#if (API_USBD_BIT_ENABLE && (USBD_HID_SUPPORT & (BIT_ENUM(HID_TYPE_KB) | BIT_ENUM(HID_TYPE_MOUSE) | BIT_ENUM(HID_TYPE_CONSUMER)))) \ 
			|| (BT_ENABLE && (BLE_HID_SUPPORT & (BIT_ENUM(HID_TYPE_KB) | BIT_ENUM(HID_TYPE_MOUSE) | BIT_ENUM(HID_TYPE_CONSUMER))))
		bool ready = false;
		#define  TEST_USB_ID	0

		#if USBD_HID_SUPPORT
		usbd_dev_t *pdev = usbd_get_dev(TEST_USB_ID);
		trp_handle_t usb_handle = {TR_USBD, TEST_USB_ID, U16(DEF_DEV_TYPE_HID,DEF_HID_TYPE_KB)};
		ready |= pdev->ready; 
		#endif

		#if BLE_HID_SUPPORT
		api_bt_ctb_t* bt_ctbp = api_bt_get_ctb(BT_BLE);
		trp_handle_t ble_handle = {TR_BLE, BT_ID0, U16(DEF_DEV_TYPE_HID,DEF_HID_TYPE_KB)};
		ready |= BOOL_SET(bt_ctbp->sta == BT_STA_READY); 
		#endif
		
		if(ready){
            static kb_t kb={KB_REPORT_ID,0};
            static mouse_t mouse={MOUSE_REPORT_ID,0};

            if(kb.key[0]){
                kb.key[0] = 0;
				kb.key[1] = 0;
            }else{
                kb.key[0] = KB_CAP_LOCK;
				kb.key[1] = KB_A;
            }

			#if USBD_HID_SUPPORT
            api_transport_tx(&usb_handle,&kb,sizeof(kb));
			#endif
			#if BLE_HID_SUPPORT
			api_transport_tx(&ble_handle,&kb,sizeof(kb));
			#endif


            if(mouse.x >= 0){
                mouse.x = -10;
            }else{
                mouse.x = 10;
            }

			#if USBD_HID_SUPPORT
            usb_handle.index = U16(DEF_DEV_TYPE_HID,DEF_HID_TYPE_MOUSE);
            api_transport_tx(&usb_handle,&mouse,sizeof(mouse));
			#endif
			#if BLE_HID_SUPPORT
			ble_handle.index = U16(DEF_DEV_TYPE_HID,DEF_HID_TYPE_MOUSE);
			api_transport_tx(&ble_handle,&mouse,sizeof(mouse));

			ble_handle.index = U16(DEV_TYPE_VENDOR,DEF_HID_TYPE_MOUSE);
			api_transport_tx(&ble_handle,&mouse,sizeof(mouse));
			#endif
        }
		#endif


		#if BT0_SUPPORT & (BIT_ENUM(TR_RF) | BIT_ENUM(TR_RFC))
		api_bt_ctb_t* rf_ctbp;
		#if BT0_SUPPORT & BIT_ENUM(TR_RF)
		rf_ctbp = api_bt_get_ctb(BT_RF);
		#endif
		#if BT0_SUPPORT & BIT_ENUM(TR_RFC)
		rf_ctbp = api_bt_get_ctb(BT_RFC);
		#endif

		if(rf_ctbp->sta == BT_STA_READY){
			static uint32_t buf = 0;
			buf++;

			m_memory_buf[!m_memory_index][0] = CMD_HEART_BEAT;
			memcpy(&m_memory_buf[!m_memory_index][1], &buf, sizeof(buf));
			m_memory_len = sizeof(buf)+1;
			m_memory_index = !m_memory_index;
		}

		#endif

    }
	

	

}

#endif
