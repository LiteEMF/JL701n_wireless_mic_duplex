#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"

#include "le_trans_data.h"
#include "le_common.h"

#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"
#include "ble/ll_config.h"
#include "third_party/wireless_mic/wl_mic_api.h"
#include "update_loader_download.h"

#if LE_DEBUG_PRINT_EN
extern void printf_buf(u8 *buf, u32 len);
/* #define log_info          printf */
#define log_info(x, ...)  printf("[LE_TRANS]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif



// 广播周期 (unit:0.625ms)
#define STANDARD_ADV_INTERVAL_MIN          (160 * 1)//

#define STANDARD_CONN_PARAM_TABLE_CNT       (sizeof(standard_connection_param_table)/sizeof(struct conn_update_param_t))

static u8 first_pair_flag = 0;     //第一次配对标记


#define REPEAT_DIRECT_ADV_COUNT  (2)// *1.28s
#define REPEAT_DIRECT_ADV_TIMER  (100)//

//为了及时响应手机数据包，某些流程会忽略进入latency的控制
//如下是定义忽略进入的interval个数
#define LATENCY_SKIP_INTERVAL_MIN     (3)
#define LATENCY_SKIP_INTERVAL_KEEP    (15)
#define LATENCY_SKIP_INTERVAL_LONG    (0xffff)



struct pair_info_t {
    u16 head_tag;             //头标识
    u8  pair_flag: 2;         //配对信息是否有效
    u8  direct_adv_flag: 1;   //是否需要定向广播
    u8  direct_adv_cnt: 5;    //定向广播次数
    u8  peer_address_info[7]; //绑定的对方地址信息
    u16 tail_tag;//尾标识
};
extern struct pair_info_t  conn_pair_info;
extern u8 cur_peer_address_info[7];

extern u8 flag_user_private;
extern uint8_t connection_update_enable; ///0--disable, 1--enable
extern uint8_t connection_update_cnt;
extern void hogp_set_ble_work_state(ble_state_e state);
extern void hogp_conn_pair_vm_do(struct pair_info_t *info, u8 rw_flag);
extern int hogp_app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);
extern void hogp_can_send_now_wakeup(void);
extern volatile hci_con_handle_t con_handle;

uint8_t connection_update_waiting; //请求已被接收，等待主机更新参数

int standard_adv_interval_min_val = STANDARD_ADV_INTERVAL_MIN;
int ble_hid_timer_handle = 0;


static u16 cur_conn_latency; //

//标准参数表
static const struct conn_update_param_t standard_connection_param_table[] = {
	{6, 9,  100, 600}, //android
	{12, 12, 30, 400}, //ios
};


//hid device infomation
u8 standard_appearance[2] = {BLE_APPEARANCE_GENERIC_HID & 0xff, BLE_APPEARANCE_GENERIC_HID >> 8}; //

static const char Manufacturer_Name_String[] = "zhuhai_jieli";
static const char Model_Number_String[] = "hid_mouse";
static const char Serial_Number_String[] = "000000";
static const char Hardware_Revision_String[] = "0.0.1";
static const char Firmware_Revision_String[] = "0.0.1";
static const char Software_Revision_String[] = "0.0.1";
static const u8 System_ID[] = {0, 0, 0, 0, 0, 0, 0, 0};

//定义的产品信息,for test
#define  PNP_VID_SOURCE   0x02
#define  PNP_VID          0x05ac //0x05d6
#define  PNP_PID          0x022C //
#define  PNP_PID_VERSION  0x011b //1.1.11

static const u8 PnP_ID[] = {PNP_VID_SOURCE, PNP_VID & 0xFF, PNP_VID >> 8, PNP_PID & 0xFF, PNP_PID >> 8, PNP_PID_VERSION & 0xFF, PNP_PID_VERSION >> 8};
/* static const u8 PnP_ID[] = {0x02, 0x17, 0x27, 0x40, 0x00, 0x23, 0x00}; */
/* static const u8 PnP_ID[] = {0x02, 0xac, 0x05, 0x2c, 0x02, 0x1b, 0x01}; */

/* static const u8 hid_information[] = {0x11, 0x01, 0x00, 0x01}; */
static const u8 hid_information[] = {0x01, 0x01, 0x00, 0x03};

static  u8 *report_map; //描述符
static  u16 report_map_size;//描述符大小
#define HID_REPORT_MAP_DATA    report_map
#define HID_REPORT_MAP_SIZE    report_map_size

//report 发生变化，通过service change 通知主机重新获取
static u8 hid_battery_level = 88;

static u8(*get_vbat_percent_call)(void) = NULL;

//report 发生变化，通过service change 通知主机重新获取
static u8 hid_report_change = 0;


static void (*le_hogp_output_callback)(u8 *buffer, u16 size) = NULL;
static void reset_passkey_cb(u32 *key);
void ble_profile_init(void);


static void ble_bt_evnet_post(u32 arg_type, u8 priv_event, u8 *args, u32 value)
{
	struct sys_event e;
	e.type = SYS_BT_EVENT;
	e.arg  = (void *)arg_type;
	e.u.bt.event = priv_event;
	if (args) {
		memcpy(e.u.bt.args, args, 7);
	}
	e.u.bt.value = value;
	sys_event_notify(&e);
}


static void ble_state_to_user(u8 state, u8 reason)
{
	ble_bt_evnet_post(SYS_BT_EVENT_BLE_STATUS, state, NULL, reason);
}
//检测是否需要定向广播
static void ble_check_need_pair_adv(void)
{
#if PAIR_DIREDT_ADV_EN
    log_info("%s\n", __FUNCTION__);
    if (conn_pair_info.pair_flag && conn_pair_info.direct_adv_flag) {
        conn_pair_info.direct_adv_cnt = REPEAT_DIRECT_ADV_COUNT;
    }
#endif
}



//@function 检测连接参数是否需要更新
static void standard_check_connetion_updata_deal(void)
{
    static struct conn_update_param_t param;
    if (connection_update_enable) {
        if (connection_update_cnt < STANDARD_CONN_PARAM_TABLE_CNT) {

            memcpy(&param, &standard_connection_param_table[connection_update_cnt], sizeof(struct conn_update_param_t)); //static ram
            log_info("update_request:-%d-%d-%d-%d-\n", param.interval_min, param.interval_max, param.latency, param.timeout);
            if (con_handle) {
                ble_op_conn_param_request(con_handle, (u32)&param);
            }
        }
    }
}

//回连状态，主动使能通知
static void resume_all_ccc_enable(u8 update_request)
{
	log_info("resume_all_ccc_enable\n");
	att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
	att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
	att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
	att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
	att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
	att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
	att_set_ccc_config(ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);

	hogp_set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
	if (update_request) {
		standard_check_connetion_updata_deal();
		ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_MIN); //
	}
}
//@function 接参数更新信息
static void standard_connection_update_complete_success(u8 *packet, u8 connected_init)
{
    int conn_interval, conn_latency, conn_timeout;

    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
	log_info("connected_init=%d",connected_init);
    if (connected_init) {
        if (conn_pair_info.pair_flag
            && (conn_latency && (0 == memcmp(conn_pair_info.peer_address_info, packet - 1, 7)))) { //
            //有配对信息，带latency连接，快速enable ccc 发数
            log_info("reconnect ok\n");
            resume_all_ccc_enable(0);
        }
    } else {
        //not creat connection,judge
            standard_check_connetion_updata_deal();
        if ((conn_interval == 12) && (conn_latency == 4)) {
            log_info("do update_again\n");
            connection_update_cnt = 1;
            standard_check_connetion_updata_deal();
        } else if ((conn_latency == 0)
                   && (connection_update_cnt == STANDARD_CONN_PARAM_TABLE_CNT)
                   && (standard_connection_param_table[0].latency != 0)) {
            log_info("latency is 0,update_again\n");
            connection_update_cnt = 0;
            standard_check_connetion_updata_deal();
        }
    }

    cur_conn_latency = conn_latency;

    if (ble_hid_timer_handle) {
        sys_s_hi_timer_modify(ble_hid_timer_handle, (u32)(conn_interval * 1.25));
    }
}


/* static void standard_send_request_connect_parameter(u8 table_index) */
/* { */
	/* struct conn_update_param_t *param = (void *)&standard_connection_param_table[table_index];//static ram */
		
    /* log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout); */
    /* if (con_handle) { */
        /* ble_op_conn_param_request(con_handle, param); */
    /* } */
/* } */

/* static void standard_check_connetion_updata_deal(void) */
/* { */
	/* if (connection_update_enable) { */
		/* if (connection_update_cnt < STANDARD_CONN_PARAM_TABLE_CNT) { */
			/* standard_send_request_connect_parameter(connection_update_cnt); */
		/* } */

	/* } */
/* } */


//协议栈sm 消息回调处理
void standard_cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	sm_just_event_t *event = (void *)packet;
	u32 tmp32;
	switch (packet_type) {
		case HCI_EVENT_PACKET:
			switch (hci_event_packet_get_type(packet)) {
				case SM_EVENT_JUST_WORKS_REQUEST:
					sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
					log_info("Just Works Confirmed.\n");
					first_pair_flag = 1;
					memcpy(conn_pair_info.peer_address_info, &packet[4], 7);
					conn_pair_info.pair_flag = 0;
					ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_KEEP); //
					ble_state_to_user(BLE_PRIV_MSG_PAIR_CONFIRM, 0);
					break;
				case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
					log_info_hexdump(packet, size);
					memcpy(&tmp32, event->data, 4);
					log_info("Passkey display: %06u.\n", tmp32);
					ble_state_to_user(BLE_PRIV_MSG_PAIR_CONFIRM, 1);
					break;
			}
			break;
	}
}


static void check_report_map_change(void)
{
	#if 0 //部分手机不支持
	if (hid_report_change && first_pair_flag && att_get_ccc_config(ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE)) {
		log_info("###send services changed\n");
		hogp_app_send_user_data(ATT_CHARACTERISTIC_2a05_01_VALUE_HANDLE, change_handle_table, 4, ATT_OP_INDICATE);
		hid_report_change = 0;
	}
	#endif
}

static bool ble_pri_or_normal_sw(bool pri)
{
	static u32 info_pram = 0;
	if (info_pram == 0) {
		info_pram = config_vendor_le_bb;
	}

	if (pri) {
		config_vendor_le_bb = info_pram;
	} else {
		config_vendor_le_bb = 0;
	}
	printf("config_vendor_le_bb = %d",config_vendor_le_bb);
	//btctrler_hci_cmd_to_task(LMP_HCI_CMD,1,HCI_RESET);
	ll_reset();
}
void ble_disconnect_complete_sw(u8 status)
{
	ble_pri_or_normal_sw(status);
#if WIRELESS_24G_ENABLE
	if (status) {
		rf_set_24g_hackable_coded(WIRELESS_24G_CODE_ID);
	} else {
		rf_set_24g_hackable_coded(0);
	}
#endif
	ble_profile_init();
	ble_module_enable(1);
}

//参考识别手机系统
static void att_check_remote_result(u16 con_handle, remote_type_e remote_type)
{
	log_info("le_hogp %02x:remote_type= %02x\n", con_handle, remote_type);
	ble_bt_evnet_post(SYS_BT_EVENT_FORM_COMMON, COMMON_EVENT_BLE_REMOTE_TYPE, NULL, remote_type);
	//to do
}


void standard_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        /* case DAEMON_EVENT_HCI_PACKET_SENT: */
        /* break; */
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");
        case ATT_EVENT_CAN_SEND_NOW:
            hogp_can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                switch (packet[3]) {
                case BT_OP_SUCCESS:
                    //init
                    first_pair_flag = 0;
                    connection_update_cnt = 0;
                    connection_update_waiting = 0;
                    cur_conn_latency = 0;
                    connection_update_enable = 1;

                    con_handle = little_endian_read_16(packet, 4);
                    log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", con_handle);
                    
					server_profile_start(con_handle);
                    log_info_hexdump(packet + 7, 7);
                    memcpy(cur_peer_address_info, packet + 7, 7);
                    ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_KEEP); //
                    standard_connection_update_complete_success(packet + 8,1);

                    //清除PC端历史键值
                    /* extern void clear_mouse_packet_data(void); */
                    /* sys_timeout_add(NULL, clear_mouse_packet_data, 50); */
#if RCSP_BTMATE_EN
#if (0 == BT_CONNECTION_VERIFY)
                    JL_rcsp_auth_reset();
#endif
                    /* rcsp_dev_select(RCSP_BLE); */
                    rcsp_init();
#endif
                    /* att_server_set_exchange_mtu(con_handle); //主动请求交换MTU */
                    /* ble_vendor_interval_event_enable(con_handle, 1); //enable interval事件-->HCI_SUBEVENT_LE_VENDOR_INTERVAL_COMPLETE */
                    break;

                case BT_ERR_ADVERTISING_TIMEOUT:
                    //定向广播超时结束
                    log_info("BT_ERR_ADVERTISING_TIMEOUT\n");
                    hogp_set_ble_work_state(BLE_ST_IDLE);

                    bt_ble_adv_enable(1);

                    break;

                default:
                    break;
                }
            }
            break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                connection_update_waiting = 0;
                standard_connection_update_complete_success(packet,0);
                break;

            case HCI_SUBEVENT_LE_VENDOR_INTERVAL_COMPLETE:
                log_info("INTERVAL_EVENT:%04x,%04x\n", little_endian_read_16(packet, 4), little_endian_read_16(packet, 6));
                /* put_buf(packet, size + 1); */
                break;

            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
#if RCSP_BTMATE_EN
            rcsp_exit();
#endif
            con_handle = 0;

            ble_op_att_send_init(con_handle, 0, 0, 0);
            /* bt_ble_adv_enable(1); */
            hogp_set_ble_work_state(BLE_ST_DISCONN);
            if (UPDATE_MODULE_IS_SUPPORT(UPDATE_BLE_TEST_EN)) {
                if (!ble_update_get_ready_jump_flag()) {
                    if (packet[5] == 0x08) {
                        //超时断开,检测是否配对广播
                        ble_check_need_pair_adv();
                    }
                    bt_ble_adv_enable(1);
                } else {
                    log_info("no open adv\n");
                }
            }
			if (flag_user_private) {	//standard2private
				ble_disconnect_complete_sw(flag_user_private);
			}
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %u\n", mtu);
            ble_op_att_set_send_mtu(mtu);
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!\n");
                standard_check_connetion_updata_deal();
            } else {
                connection_update_waiting = 1;
                connection_update_cnt = STANDARD_CONN_PARAM_TABLE_CNT;
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            if (!packet[2]) {
                if (first_pair_flag) {
                    //只在配对时启动检查
                    att_server_set_check_remote(con_handle, att_check_remote_result);
#if PAIR_DIREDT_ADV_EN
                    conn_pair_info.pair_flag = 1;
                    hogp_conn_pair_vm_do(&conn_pair_info, 1);
#endif
                } else {
                    if ((!conn_pair_info.pair_flag) || memcmp(cur_peer_address_info, conn_pair_info.peer_address_info, 7)) {
                        log_info("record reconnect peer\n");
                        put_buf(cur_peer_address_info, 7);
                        put_buf(conn_pair_info.peer_address_info, 7);
                        memcpy(conn_pair_info.peer_address_info, cur_peer_address_info, 7);
                        conn_pair_info.pair_flag = 1;
                        hogp_conn_pair_vm_do(&conn_pair_info, 1);
                    }
                    resume_all_ccc_enable(1);

                    //回连时,从配对表中获取
                    u8 tmp_buf[6];
                    u8 remote_type = 0;
                    swapX(&cur_peer_address_info[1], tmp_buf, 6);
                    ble_list_get_remote_type(tmp_buf, cur_peer_address_info[0], &remote_type);
                    log_info("list's remote_type:%d\n", remote_type);
                    att_check_remote_result(con_handle, remote_type);
                }
                check_report_map_change();
                ble_state_to_user(BLE_PRIV_PAIR_ENCRYPTION_CHANGE, first_pair_flag);
            }
            break;
        }
        break;
    }
}


uint16_t standard_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{

    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;


    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
		u8* gap_device_name = (u8 *)(bt_get_local_name());
        att_value_len = strlen(gap_device_name);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_device_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s \n", gap_device_name);
        }
        break;

    case ATT_CHARACTERISTIC_2a01_01_VALUE_HANDLE:
        att_value_len = sizeof(standard_appearance);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &standard_appearance[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a04_01_VALUE_HANDLE:
        att_value_len = 8;//fixed
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            log_info("\n------get standard_connection_param_table\n");
            memcpy(buffer, &standard_connection_param_table[0], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a29_01_VALUE_HANDLE:
        att_value_len = strlen(Manufacturer_Name_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Manufacturer_Name_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a24_01_VALUE_HANDLE:
        att_value_len = strlen(Model_Number_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Model_Number_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a25_01_VALUE_HANDLE:
        att_value_len = strlen(Serial_Number_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Serial_Number_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a27_01_VALUE_HANDLE:
        att_value_len = strlen(Hardware_Revision_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Hardware_Revision_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a26_01_VALUE_HANDLE:
        att_value_len = strlen(Firmware_Revision_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Firmware_Revision_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a28_01_VALUE_HANDLE:
        att_value_len = strlen(Software_Revision_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Software_Revision_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a23_01_VALUE_HANDLE:
        att_value_len = sizeof(System_ID);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &System_ID[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a50_01_VALUE_HANDLE:
        log_info("read PnP_ID\n");
        att_value_len = sizeof(PnP_ID);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &PnP_ID[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a19_01_VALUE_HANDLE:
        att_value_len = 1;
        if (buffer) {
            if (get_vbat_percent_call) {
                hid_battery_level = get_vbat_percent_call();
            }
            buffer[0] = hid_battery_level;
        }
        break;

    case ATT_CHARACTERISTIC_2a4b_01_VALUE_HANDLE:
        att_value_len = HID_REPORT_MAP_SIZE;
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &HID_REPORT_MAP_DATA[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a4a_01_VALUE_HANDLE:
        att_value_len = sizeof(hid_information);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &hid_information[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;

    default:
        break;
    }

    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;

}



void __attribute__((weak)) ble_hid_transfer_channel_recieve(u8 *packet, u16 size)
{
	log_info("transfer_rx(%d):", size);
	log_info_hexdump(packet, size);
	//ble_hid_transfer_channel_send(packet,size);//for test
}
int standard_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

    log_info("write_callback, handle= 0x%04x,size = %d\n", handle, buffer_size);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_2a4d_03_VALUE_HANDLE:
        put_buf(buffer, buffer_size);           //键盘led灯状态
        if (le_hogp_output_callback) {
            le_hogp_output_callback(buffer, buffer_size);
        }
        break;

    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
        att_set_ccc_config(handle, buffer[0]);
        if (buffer[0]) {
            check_report_map_change();
        }
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae02_02_CLIENT_CONFIGURATION_HANDLE:
#if (0 == BT_CONNECTION_VERIFY)
        JL_rcsp_auth_reset();       //hid设备试能nofity的时候reset auth保证APP可以重新连接
#endif

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
        ble_bt_evnet_post(SYS_BT_EVENT_FORM_COMMON, COMMON_EVENT_SHUTDOWN_DISABLE, NULL, 0);
        /* auto_shutdown_disable(); */
#endif
#endif

    case ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE:
        hogp_set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_MIN); //
    case ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE:
        if ((cur_conn_latency == 0)
            && (connection_update_waiting == 0)
            && (connection_update_cnt == STANDARD_CONN_PARAM_TABLE_CNT)
            && (standard_connection_param_table[0].latency != 0)) {
            connection_update_cnt = 0;//update again
        }
        standard_check_connetion_updata_deal();
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae01_02_VALUE_HANDLE:
        log_info("rcsp_read:%x 0x%x\n", buffer_size, app_recieve_callback);
        connection_update_enable = 0;
        ble_op_latency_skip(con_handle, LATENCY_SKIP_INTERVAL_LONG); //
        if (app_recieve_callback) {
            app_recieve_callback(0, buffer, buffer_size);
        }
        break;
#endif

    case ATT_CHARACTERISTIC_ae41_01_VALUE_HANDLE:
        ble_hid_transfer_channel_recieve(buffer, buffer_size);
        break;

    default:
        break;
    }
    return 0;
}


static const u16 report_id_handle_table[] = {
    0,
    HID_REPORT_ID_01_SEND_HANDLE,
    HID_REPORT_ID_02_SEND_HANDLE,
    HID_REPORT_ID_03_SEND_HANDLE,
    HID_REPORT_ID_04_SEND_HANDLE,
    HID_REPORT_ID_05_SEND_HANDLE,
    HID_REPORT_ID_06_SEND_HANDLE,
};

int ble_hid_data_send(u8 report_id, u8 *data, u16 len)
{
    if (report_id == 0 || report_id > 6) {
        log_info("report_id %d,err!!!\n", report_id);
        return -1;
    }
    return hogp_app_send_user_data(report_id_handle_table[report_id], data, len, ATT_OP_AUTO_READ_CCC);
}

void ble_hid_key_deal_test(u16 key_msg)
{
    if (key_msg) {
        ble_hid_data_send(1, &key_msg, 2);
        key_msg = 0;//key release
        ble_hid_data_send(1, &key_msg, 2);
    }
}

int ble_hid_transfer_channel_send(u8 *packet, u16 size)
{
    /* log_info("transfer_tx(%d):", size); */
    /* log_info_hexdump(packet, size); */
    return hogp_app_send_user_data(ATT_CHARACTERISTIC_ae42_01_VALUE_HANDLE, packet, size, ATT_OP_AUTO_READ_CCC);
}

//status: 1--私有ble	0---标准ble
void user_ble_pri_or_normal_sw(u8 status)
{
	flag_user_private = status;
	connection_update_cnt = 0;
	log_info("status=%d",status);
	if (!bt_ble_is_connected()) {	//没有连接，直接切换
		ble_module_enable(0);
		ble_disconnect_complete_sw(status);
	} else {
		ble_module_enable(0);		//连接中，再断开事件那边切换
	}
}


void le_hogp_set_icon(u16 class_type)
{
    memcpy(standard_appearance, &class_type, 2);
}

void le_hogp_set_ReportMap(u8 *map, u16 size)
{
    report_map = map;
    report_map_size = size;
    hid_report_change = 1;
}









