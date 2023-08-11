#include "adapter_idev.h"
#include "adapter_odev.h"
#include "adapter_idev_bt.h"
#include "adapter_odev_dac.h"
#include "adapter_process.h"
#include "adapter_media.h"
#include "application/audio_dig_vol.h"
#include "le_client_demo.h"
#include "system/malloc.h"
#include "key_event_deal.h"
#include "asm/pwm_led.h"
#include "ui_manage.h"
#include "adapter_idev_usb.h"
#include "usb/device/hid.h"
#include "adapter_wireless_command.h"

#if (APP_MAIN ==  APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_MASTER)

#define POWER_OFF_CNT 	     			6
#define MASTER_ENC_COMMAND_LEN			3
#define SLAVE_ENC_COMMAND_LEN			1


u8 wireless_conn_status = 0;
static u16 cur_music_vol = 0;
static u8 flag_mute_dac = 0;

extern u32 adapter_usb_mic_get_output_total_size(void *priv);
extern u32 adapter_usb_mic_get_output_size(void *priv);
extern u16 uac_get_cur_vol(const u8 id, u16 *l_vol, u16 *r_vol);

static const struct idev_usb_fmt usb_parm_list[] = {
    //HID
    {
        .attr  = IDEV_USB_ATTR_HID,
        .value =
        {
            .hid = {
                .report_map_num = 1,
                .report_map[0] = {
                    .map = NULL,
                    .size = 0,
                }
            },
        },
    },
    //END
    {
        .attr = IDEV_USB_ATTR_END,
    },
};


static u16 wireless_mic_upstream_in_sr(void *priv)
{
    return WIRELESS_DECODE_SAMPLERATE;
}
static u16 wireless_mic_upstream_out_sr(void *priv)
{
    return MIC_AUDIO_RATE;//USB mic接type C连接手机， 48000兼容性好， 所以这里用48000这个采样率
}


struct adapter_encoder_fmt master_enc_upstream_parm = {
    .enc_type = ADAPTER_ENC_TYPE_UAC_MIC,
};

static int dac_data_pro_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in)/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
{
    if (flag_mute_dac) {
        memset(in->data, 0x00, in->data_len);
    }
    return 0;
}
static const struct adapter_stream_fmt master_audio_upstream_list[] = {
#if (WIRELESS_EARPHONE_MIC_EN)
    //dac 节点
    {
        .attr  = ADAPTER_STREAM_ATTR_DAC,
        .value = {
#if NEW_AUDIO_W_MIC
            .dac1 = {
                .data_handle = dac_data_pro_handler,//如果要关注处理前的数据可以在这里注册回调
                .attr = {
                    .write_mode = WRITE_MODE_BLOCK,
                    .sample_rate = WIRELESS_DECODE_SAMPLERATE,
                    .delay_ms = 12,
                    .protect_time = 0,
                    .start_delay = 6,
                    .underrun_time = 1,
                },
            }
#else
            .dac = {
                .data_handle = dac_data_pro_handler,//如果要关注处理前的数据可以在这里注册回调
                //其他关于dac的参数配置
                .get_delay_time = NULL,
                .attr = {
                    .write_mode = WRITE_MODE_BLOCK,
                    .delay_time = 12,//10,//dac延时设置
                    .protect_time = 0,
                    .start_delay = 6,
                    .underrun_time = 1,
                },
            },
#endif
        },
    },
#else
    //SYNC节点
    {
        .attr  = ADAPTER_STREAM_ATTR_SYNC,
        .value = {
            .sync = {
                //其他关于sync的参数配置
                .always = 1,
                .ch_num = 1,//mic 是单声道
                .ibuf_len = 0,
                .obuf_len = 0,
                .begin_per = 40,
                .top_per = 60,
                .bottom_per = 30,
                .inc_step = 10,
                .dec_step = 10,
                .max_step = 100,
                .get_in_sr = wireless_mic_upstream_in_sr,
                .get_out_sr = wireless_mic_upstream_out_sr,
                .get_total = adapter_usb_mic_get_output_total_size,
                .get_size = adapter_usb_mic_get_output_size,

                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
            },
        },
    },
    {
        .attr  = ADAPTER_STREAM_ATTR_ENCODE,
        .value = {
            .encoder = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                .parm = &master_enc_upstream_parm,

                //其他关于encoder的参数配置
            },
        },
    },
#endif
};

static void master_downstream_enc_command_callback(u8 *buf, u8 len)
{
    u16 vol_l, vol_r, vol_mic;
#if (WIRELESS_EARPHONE_MIC_EN)
    vol_l = 100;
    vol_r = 100;
    vol_mic = 100;
#else
    uac_get_cur_vol(0, &vol_l, &vol_r);
    vol_mic = uac_get_mic_vol(0);
#endif

    buf[0] = (u8)vol_l;
    buf[1] = (u8)vol_r;
    buf[2] = (u8)vol_mic;

}
struct adapter_encoder_fmt master_enc_downstream_parm = {
    .enc_type = ADAPTER_ENC_TYPE_WIRELESS,
    .channel_type = NULL,//区分左右声道,这里传的是指针，目的可以动态获取
    .command_len = MASTER_ENC_COMMAND_LEN,
    .command_callback = master_downstream_enc_command_callback,
};
static const struct adapter_stream_fmt master_audio_downstream_list[] = {
    {
        .attr  = ADAPTER_STREAM_ATTR_ENCODE,
        .value = {
            .encoder = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                .parm = &master_enc_downstream_parm,

            },
        },
    },
};
static int master_audio_command_parse(void *priv, u8 *data)
{
    /*!< 这里回调是音频传输过程附带过来的命令数据 */
    u8 command = data[0];
    if (command) {
        if (command == WIRELESS_TRANSPORT_CMD_PP) {
            app_task_put_key_msg(KEY_MUSIC_PP, (int)command);
        } else if (command == WIRELESS_TRANSPORT_CMD_PREV) {
            app_task_put_key_msg(KEY_MUSIC_PREV, (int)command);
        } else if (command == WIRELESS_TRANSPORT_CMD_NEXT) {
            app_task_put_key_msg(KEY_MUSIC_NEXT, (int)command);
        } else if (command == WIRELESS_TRANSPORT_CMD_VOL_UP) {
            app_task_put_key_msg(KEY_VOL_UP, (int)command);
        } else if (command == WIRELESS_TRANSPORT_CMD_VOL_DOWN) {
            app_task_put_key_msg(KEY_VOL_DOWN, (int)command);
        }
    }
    return SLAVE_ENC_COMMAND_LEN;
}
static const struct adapter_decoder_fmt master_audio_upstream = {
    .dec_type = ADAPTER_DEC_TYPE_JLA,
    .list	  = master_audio_upstream_list,
    .list_num = sizeof(master_audio_upstream_list) / sizeof(master_audio_upstream_list[0]),
#if (WIRELESS_EARPHONE_MIC_EN == 1)
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_LR_DIFF)
    .dec_output_type = AUDIO_CH_DIFF,
#else
    .dec_output_type = AUDIO_CH_LR,
#endif
#else
    .dec_output_type = AUDIO_CH_DIFF,
#endif
    .cmd_parse_cbk = master_audio_command_parse,
};
static const struct adapter_decoder_fmt master_audio_downstream = {
#if (WIRELESS_EARPHONE_MIC_EN)
    .dec_type = ADAPTER_DEC_TYPE_MIC,
#else
    .dec_type = ADAPTER_DEC_TYPE_UAC_SPK,
#endif
    .list	  = master_audio_downstream_list,
    .list_num = sizeof(master_audio_downstream_list) / sizeof(master_audio_downstream_list[0]),
};
static const struct adapter_media_fmt master_media_list[] = {
    [0] = {
        .upstream =	&master_audio_upstream,
        .downstream = &master_audio_downstream,
    },
};

static const struct adapter_media_config master_media_config = {
    .list = master_media_list,
    .list_num = sizeof(master_media_list) / sizeof(master_media_list[0]),
};

//bt odev config
static const u8 dongle_remoter_name1[] = "S2_MIC_0";
static const client_match_cfg_t match_dev01 = {
    .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
    .compare_data_len = sizeof(dongle_remoter_name1) - 1, //去结束符
    .compare_data = dongle_remoter_name1,
    .bonding_flag = 0,
};

static client_conn_cfg_t ble_conn_config = {
    .match_dev_cfg[0] = &match_dev01,      //匹配指定的名字
    .match_dev_cfg[1] = NULL,
    .match_dev_cfg[2] = NULL,
    .search_uuid_cnt = 0,
    .security_en = 0,       //加密配对
};

struct _odev_bt_parm odev_bt_parm_list = {
    .mode = BIT(ODEV_BLE), //only support BLE

    //for BLE config
    .ble_parm.cfg_t = &ble_conn_config,
};

static int dongle_key_event_handler(struct sys_event *event)
{
    int ret = 0;
    struct key_event *key = &event->u.key;

    u16 key_event = event->u.key.event;

    static u8 key_poweroff_cnt = 0;
    static u8 flag_poweroff = 0;

    printf("master key_event:%d %d %d\n", key_event, key->value, key->event);
    switch (key_event) {

    case KEY_MUSIC_PREV:
        printf("KEY_MUSIC_PREV\n");
        hid_key_handler(0, USB_AUDIO_PREFILE);
        break;
    case KEY_MUSIC_NEXT:
        printf("KEY_MUSIC_NEXT\n");
        hid_key_handler(0, USB_AUDIO_NEXTFILE);
        break;
    case KEY_MUSIC_PP:
        printf("KEY_MUSIC_PP\n");
        hid_key_handler(0, USB_AUDIO_PP);
        break;
    case KEY_VOL_DOWN:
        printf("KEY_VOL_DOWN\n");
        hid_key_handler(0, USB_AUDIO_VOLDOWN);
        break;
    case KEY_VOL_UP:
        printf("KEY_VOL_UP\n");
        hid_key_handler(0, USB_AUDIO_VOLUP);
        break;
    case  KEY_POWEROFF:
        printf("KEY POWEROFF\n");
        key_poweroff_cnt = 0;
        flag_poweroff = 1;
        break;
    case  KEY_POWEROFF_HOLD:
        printf("KEY POWEROFF_HOLD\n");
        if (flag_poweroff) {
            if (++key_poweroff_cnt >= POWER_OFF_CNT) {
                key_poweroff_cnt = 0;
                ret = 1;
            }
        }
        break;
    case  KEY_ENTER_PAIR:
#if WIRELESS_PAIR_BONDING
        extern void clear_bonding_info(void);
        clear_bonding_info();
#endif
        break;
    case  KEY_NULL:
        break;
    }
    return ret;
}

static int dongle_event_handle_callback(struct sys_event *event)
{
    //处理用户关注的事件
    int ret = 0;
    switch (event->type) {
    case SYS_KEY_EVENT:
        ret = dongle_key_event_handler(event);
        break;
    case SYS_DEVICE_EVENT:
        app_common_device_event_deal(event);
        switch ((u32)event->arg) {
        case DEVICE_EVENT_FROM_ADAPTER:
            switch (event->u.dev.event) {
            case ADAPTER_EVENT_CONNECT :
                printf("ADAPTER_EVENT_CONNECT");
                wireless_conn_status = 1;
                break;
            case ADAPTER_EVENT_DISCONN :
                printf("ADAPTER_EVENT_DISCONN");
                wireless_conn_status = 0;
                break;
            case ADAPTER_EVENT_IDEV_MEDIA_CLOSE :
            case ADAPTER_EVENT_ODEV_MEDIA_CLOSE :
                break;
            case ADAPTER_EVENT_POWEROFF:
                ret = 1;
                break;
            }
        }
        break;
    default:
        break;

    }
    return ret;
}

void app_main_run(void)
{
    ui_update_status(STATUS_POWERON);

    while (1) {
        //初始化
#if (WIRELESS_EARPHONE_MIC_EN)
        struct idev *idev = adapter_idev_open(ADAPTER_IDEV_MIC, NULL);
        struct odev *odev = adapter_odev_open(ADAPTER_ODEV_BT, (void *)&odev_bt_parm_list);
#else
        struct idev *idev = adapter_idev_open(ADAPTER_IDEV_USB, (void *)&usb_parm_list);
        struct odev *odev = adapter_odev_open(ADAPTER_ODEV_BT, (void *)&odev_bt_parm_list);
#endif//WIRELESS_EARPHONE_MIC_EN
        struct adapter_media *media = adapter_media_open((struct adapter_media_config *)&master_media_config);
        printf("dongle~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

        ASSERT(idev);
        ASSERT(odev);
        ASSERT(media);
        struct adapter_pro *pro = adapter_process_open(idev, odev, media, dongle_event_handle_callback);//event_handle_callback 用户想拦截处理的事件

        ASSERT(pro, "adapter_process_open fail!!\n");

        //执行(包括事件解析、事件执行、媒体启动/停止, HID等事件转发)
        adapter_process_run(pro);

        //退出/关闭
        adapter_process_close(&pro);
        adapter_media_close(&media);
        adapter_idev_close(idev);
        adapter_odev_close(odev);

        ui_update_status(STATUS_POWEROFF);
        ///run idle off poweroff
        printf("enter poweroff !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        if (get_charge_online_flag()) {
            printf("charge_online,cpu reset");
            cpu_reset();
        } else {
            power_set_soft_poweroff();
        }
    }
}
#endif

