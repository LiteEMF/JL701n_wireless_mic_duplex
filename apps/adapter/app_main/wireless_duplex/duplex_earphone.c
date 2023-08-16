#include "adapter_idev.h"
#include "adapter_odev.h"
#include "adapter_adc.h"
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
#include "audio_config.h"
#include "tone_player.h"
#include "duplex_mode_switch.h"
#ifdef LITEEMF_ENABLED
#include "api/bt/api_bt.h"
#include "api/api_log.h"
#endif


#if (APP_MAIN == APP_WIRELESS_DUPLEX && WIRELESS_ROLE_SEL == APP_WIRELESS_SLAVE)

#define POWER_OFF_CNT 	     			6
#define MASTER_ENC_COMMAND_LEN			3
#define SLAVE_ENC_COMMAND_LEN			1
#define MAX_VOL				            15

static s32 cur_music_vol = -1;
static s32 cur_mic_vol = -1;
static u32 cur_time = 0;
static u8 flag_mute_dac = 0;
u8 wireless_conn_status = 0;
static u8 echo_cmd = WIRELESS_MIC_ECHO_OFF;
static u8 command = WIRELESS_MIC_DENOISE_OFF;

static struct adapter_media *wireless_mic_media = NULL;
extern struct audio_dac_hdl dac_hdl;

extern u8 get_charge_online_flag(void);
extern u8 app_common_device_event_deal(struct sys_event *event);
#if (WIRELESS_DENOISE_ENABLE)
const struct __stream_HighSampleRateSingleMicSystem_parm denoise_parm_1 = {
    .AggressFactor = 1.0f,
    .minSuppress = 0.5f,//0.5f,/*!< 最小压制,建议范围[0.04 - 0.5], 越大降得越少 */
    .init_noise_lvl = 0.0f,
    .frame_len = WIRELESS_CODING_FRAME_LEN,
};

const struct __stream_HighSampleRateSingleMicSystem_parm denoise_parm_2 = {
    .AggressFactor = 1.0f,
    .minSuppress = 0.25f,/*!< 最小压制,建议范围[0.04 - 0.5], 越大降得越少 */
    .init_noise_lvl = 0.0f,
    .frame_len = WIRELESS_CODING_FRAME_LEN,
};

const struct __stream_HighSampleRateSingleMicSystem_parm denoise_parm_3 = {
    .AggressFactor = 1.0f,
    .minSuppress = 0.1f,/*!< 最小压制,建议范围[0.04 - 0.5], 越大降得越少 */
    .init_noise_lvl = 0.0f,
    .frame_len = WIRELESS_CODING_FRAME_LEN,
};
#endif
#if WIRELESS_LLNS_ENABLE
const struct __stream_llns_parm llns_parm_1 = {
    .gainfloor = 0.5f,
    .suppress_level = 1.0f,
    .frame_len = WIRELESS_CODING_FRAME_LEN,
};
const struct __stream_llns_parm llns_parm_2 = {
    .gainfloor = 0.25f,
    .suppress_level = 1.0f,
    .frame_len = WIRELESS_CODING_FRAME_LEN,
};
const struct __stream_llns_parm llns_parm_3 = {
    .gainfloor = 0.1f,
    .suppress_level = 1.0f,
    .frame_len = WIRELESS_CODING_FRAME_LEN,
};
#endif
#if WIRELESS_PLATE_REVERB_ENABLE
Plate_reverb_parm plate_reverb_parm = {
    .wet = 40,                      //0-300%
    .dry = 80,                      //0-200%
    .pre_delay = 0,                 //0-40ms
    .highcutoff = 12200,                //0-20k 高频截止
    .diffusion = 43,                  //0-100%
    .decayfactor = 70,                //0-100%
    .highfrequencydamping = 26,       //0-100%
    .modulate = 1,                  // 0或1
    .roomsize = 100,                   //20%-100%
};
#endif
#if WIRELESS_ECHO_ENABLE
#ifndef CONFIG_EFFECT_CORE_V2_ENABLE
ECHO_PARM_SET echo_parm = {
    .delay = 120,//200,				//回声的延时时间 0-300ms
    .decayval = 35,//50,				// 0-70%
    .direct_sound_enable = 1,	//直达声使能  0/1
    .filt_enable = 1,			//发散滤波器使能
};
EF_REVERB_FIX_PARM echo_fix_parm = {
    .wetgain = 2048,			////湿声增益：[0:4096]
    .drygain = 4096,				////干声增益: [0:4096]
    .sr = WIRELESS_CODING_SAMPLERATE,		////采样率
    .max_ms = 200,				////所需要的最大延时，影响 need_buf 大小
};
#else
ECHO_PARM_SET echo_parm = {
    .delay = 120,                      //回声的延时时间 0-max_ms, 单位ms
    .decayval = 35,                   // 0-70%
    .filt_enable = 1,                //滤波器使能标志
    .lpf_cutoff = 5000,                 //0-20k
    .wetgain = 2048,                    //0-200%
    .drygain = 4096,                    //0-100%
};
EF_REVERB_FIX_PARM echo_fix_parm = {
    .sr = WIRELESS_CODING_SAMPLERATE,		////采样率
    .max_ms = 200,				////所需要的最大延时，影响 need_buf 大小
};
#endif
#endif

extern u32 timer_get_ms(void);
extern int adapter_wireless_media_get_packet_num(void);
extern int audio_dac_data_time(struct audio_dac_hdl *dac);
extern void adapter_wireless_enc_command_send(u8 command);
extern void audio_fade_in_fade_out(u8 left_gain, u8 right_gain, u8 fade);

#define EARPHONE_TONE_DEFAULT_VOL  12
static void earphone_tone_play_callback(void *priv)
{
    printf("earphone_tone_play_callback\n");
    //audio_fade_in_fade_out(0, 0, 0);
    //这里要加点延时，等声音输出完
    os_time_dly(15);
    cur_music_vol = -1;
    cur_mic_vol = -1;
}


static int earphone_tone_play(const char *list, u8 preemption)
{
    while (wl_mic_tone_get_dec_status() == TONE_START) {
        os_time_dly(5);
    }
    wl_mic_tone_play_stop();

    int err = wl_mic_tone_play(list, 0, earphone_tone_play_callback, NULL);
    if (0 == err)   {
        //audio_fade_in_fade_out(EARPHONE_TONE_DEFAULT_VOL, EARPHONE_TONE_DEFAULT_VOL, 1);
    }
    return err;
}





#if (WIRELESS_NOISEGATE_EN)
#ifndef CONFIG_EFFECT_CORE_V2_ENABLE
const NOISEGATE_PARM slave_noisegate_parm = {
    .attackTime = 300,
    .releaseTime = 5,
    .threshold = -45000,
    .low_th_gain = 0,
    .sampleRate = WIRELESS_CODING_SAMPLERATE,
    .channel = 1,
};
#else//701N跑下面的配置
const NoiseGateParam slave_noisegate_parm = {
    .attackTime = 300,
    .releaseTime = 5,
    .threshold = -45000,
    .low_th_gain = 0,
    .sampleRate = WIRELESS_CODING_SAMPLERATE,
    .channel = 1,
    .IndataInc = 0,
    .OutdataInc = 0,
};

#endif
#endif//WIRELESS_NOISEGATE_EN

static struct adapter_encoder_fmt slave_enc_upstream_parm = {
    .enc_type = ADAPTER_ENC_TYPE_WIRELESS,
    .channel_type = NULL,//区分左右声道,这里传的是指针，目的可以动态获取
    .command_len = SLAVE_ENC_COMMAND_LEN,
};
static const u16 slave_dvol_table[] = {
    0	, //0
    93	, //1
    111	, //2
    132	, //3
    158	, //4
    189	, //5
    226	, //6
    270	, //7
    323	, //8
    386	, //9
    462	, //10
    552	, //11
    660	, //12
    789	, //13
    943	, //14
    1127, //15
    1347, //16
    1610, //17
    1925, //18
    2301, //19
    2751, //20
    3288, //21
    3930, //22
    4698, //23
    5616, //24
    6713, //25
    8025, //26
    9592, //27
    11466,//28
    15200,//29
    16000,//30
    16384 //31
};

static const audio_dig_vol_param slave_digital_vol_parm = {
    .vol_start = ARRAY_SIZE(slave_dvol_table) - 1,
    .vol_max = ARRAY_SIZE(slave_dvol_table) - 1,
    .ch_total = 1,
    .fade_en = 1,
    .fade_points_step = 2,
    .fade_gain_step = 2,
    .vol_list = (void *)slave_dvol_table,
};

static const u16 master_dvol_table[] = {
    0	, //0
    93	, //1
    111	, //2
    132	, //3
    158	, //4
    189	, //5
    226	, //6
    270	, //7
    323	, //8
    386	, //9
    462	, //10
    552	, //11
    660	, //12
    789	, //13
    943	, //14
    1127, //15
    1347, //16
    1610, //17
    1925, //18
    2301, //19
    2751, //20
    3288, //21
    3930, //22
    4698, //23
    5616, //24
    6713, //25
    8025, //26
    9592, //27
    11466,//28
    15200,//29
    16000,//30
    16384 //31
};

static const audio_dig_vol_param downstream_digital_vol_parm = {
    .vol_start = ARRAY_SIZE(slave_dvol_table) - 1,
    .vol_max = ARRAY_SIZE(slave_dvol_table) - 1,
    .ch_total = 2,
    .fade_en = 1,
    .fade_points_step = 2,
    .fade_gain_step = 2,
    .vol_list = (void *)slave_dvol_table,
};

static const struct adapter_stream_fmt slave_audio_upstream_list[] = {
#if 0
    {
        .attr = ADAPTER_STREAM_ATTR_STREAM_DEMO,
        .value = {
            .demo = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
            },
        },
    },
#endif
//数字音量节点
    {
        .attr  = ADAPTER_STREAM_ATTR_DVOL,
        .value = {
            .digital_vol = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                .volume = &slave_digital_vol_parm,
            },
        },
    },
#if TCFG_EQ_ENABLE
#if TCFG_AEC_DCCS_EQ_ENABLE
    {
        .attr  = ADAPTER_STREAM_ATTR_DCCS,
        .value = {
            .dccs = {
                .parm = &dccs_parm,
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
            },
        },
    },
#endif
#if TCFG_PHONE_EQ_ENABLE
    {
        .attr  = ADAPTER_STREAM_ATTR_MICEQ,
        .value = {
            .miceq = {
                .samplerate = WIRELESS_CODING_SAMPLERATE,
                .ch_num = 1,
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
            },
        },
    },
#endif
#endif
#if WIRELESS_ECHO_ENABLE
    {
        .attr  = ADAPTER_STREAM_ATTR_ECHO,
        .value = {
            .echo = {
                .parm = &echo_parm,
                .fix_parm = &echo_fix_parm,
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
            },
        },
    },
#endif
#if WIRELESS_PLATE_REVERB_ENABLE
    {
        .attr  = ADAPTER_STREAM_ATTR_PLATE_REVERB,
        .value = {
            .plate_reverb = {
                .parm = &plate_reverb_parm,
                .samplerate = WIRELESS_CODING_SAMPLERATE,
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
            },
        },
    },
    {
        .attr = ADAPTER_STREAM_ATTR_CH_SW,	//reverb会把数据转成双声道，这里要重新转成单声道
        .value = {
            .ch_sw = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                .out_ch_type = AUDIO_CH_DIFF,
                .buf_len = 0,
            },
        },
    },
#endif
#if (WIRELESS_DENOISE_ENABLE)
    //降噪节点
    {
        .attr  = ADAPTER_STREAM_ATTR_DENOISE,
        .value = {
            .denoise = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                .sample_rate = 32000,
                .onoff = 1,
                .parm = {
                    .AggressFactor = 1.0f,
                    .minSuppress = 0.1f,
                    .init_noise_lvl = 0.0f,
                    .frame_len = WIRELESS_CODING_FRAME_LEN,
                },
            },
        },
    },
#endif
#if (WIRELESS_LLNS_ENABLE)
    {
        .attr  = ADAPTER_STREAM_ATTR_LLNS,
        .value = {
            .llns = {
                .samplerate  = WIRELESS_CODING_SAMPLERATE,
                .onoff = 0,
                .llns_parm = {
                    .gainfloor = 0.1f,
                    .suppress_level = 1.0f,
                    .frame_len = WIRELESS_CODING_FRAME_LEN,
                },
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                //其他关于encoder的参数配置
            },
        },
    },
#endif
#if (WIRELESS_NOISEGATE_EN)
    {
        .attr  = ADAPTER_STREAM_ATTR_NOISEGATE,
        .value = {
            .noisegate = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                .parm = &slave_noisegate_parm,
            },
        },
    },
#endif
    {

        .attr  = ADAPTER_STREAM_ATTR_ENCODE,
        .value = {
            .encoder = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                .parm = &slave_enc_upstream_parm,

                //其他关于encoder的参数配置
            },
        },
    },
};

static u16 slave_audio_downstream_get_in_sr(void *priv)
{
    return WIRELESS_DECODE_SAMPLERATE;
}

static u16 slave_audio_downstream_get_out_sr(void *priv)
{
    return WIRELESS_DECODE_SAMPLERATE;
}

static u32 slave_audio_downstream_get_max_delay_ms(void *priv)
{
    return 12;//20;
}

static u32 slave_audio_downstream_get_cur_delay_ms(void *priv)
{
    u32 packets = 0;//adapter_wireless_media_get_packet_num();
    if (wireless_mic_media) {
        packets = adapter_decoder_ioctrl(wireless_mic_media->downdecode, ADAPTER_DEC_IOCTRL_CMD_GET_RX_PACKET_NUM, 0, 0);//;adapter_wireless_media_get_packet_num();
    }
    u32 delay = (WIRELESS_MIC_ADC_POINT_UNIT * packets * 1000) / WIRELESS_DECODE_SAMPLERATE * 2;
#if NEW_AUDIO_W_MIC
    struct adapter_audio_stream *audio_stream_hdl = (struct adapter_audio_stream *)(adapter_decoder_ioctrl(wireless_mic_media->downdecode, ADAPTER_DEC_IOCTRL_CMD_GET_AUDIO_STREAM_HDL, 0, 0));
    delay += audio_sound_dac_get_data_time(audio_stream_hdl->dac1);
    /* printf("%d", delay); */
#else
    delay += audio_dac_data_time(&dac_hdl);
#endif
    //put_u32hex(delay);
    return delay;
}
#if WIRELESS_AUTO_MUTE
/*
注!!!：由于提示音不是走的同一路解码，所以需要在提示音播放前需要判断一下是否mute住了，
	   解mute之后再播放提示音,在播放完提示音后判断mute状态，再mute
 */
//用户在此修改自己功放mute脚控制
#define DAC_MUTE	//app_audio_mute(AUDIO_MUTE_DEFAULT)
#define DAC_UNMUTE	//app_audio_mute(AUDIO_UNMUTE_DEFAULT)
//auto_mute 事件处理,用户可以根据需求添加mute处理
void auto_mute_handler(u8 event, u8 ch)
{
    printf(">>>> ch:%d %s\n", ch, event ? ("MUTE") : ("UNMUTE"));
    flag_mute_dac = event;
    if (flag_mute_dac) {
        DAC_MUTE;
    } else {
        DAC_UNMUTE;
    }
}
static const audio_energy_detect_param auto_mute_parm = {
    .mute_energy = 5,
    .unmute_energy = 10,
    .mute_time_ms = 1000,
    .unmute_time_ms = 100,
    .count_cycle_ms = 10,
    .sample_rate = WIRELESS_DECODE_SAMPLERATE,
    .event_handler = auto_mute_handler,
#if TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_LR_DIFF
    .ch_total = 1,//app_audio_output_channel_get(),
#else
    .ch_total = 2,//app_audio_output_channel_get(),
#endif
    .dcc = 1,
};
#endif

static int dac_data_pro_handler(struct audio_stream_entry *entry,  struct audio_data_frame *in)/*!< 该数据流数据处理前数据回调,可以实现数据检测及做特殊数字处理 */
{
    if (flag_mute_dac) {
        memset(in->data, 0x00, in->data_len);
    }
    return 0;
}

static const struct adapter_stream_fmt slave_audio_downstream_list[] = {

#if 0
    {
        .attr  = ADAPTER_STREAM_ATTR_SYNC,
        .value = {
            .sync = {
                //其他关于sync的参数配置
                .always = 1,
                .ch_num = 2,
                .ibuf_len = 0,
                .obuf_len = 0,
                .begin_per = 60,
                .top_per = 70,
                .bottom_per = 50,
                .inc_step = 10,
                .dec_step = 10,
                .max_step = 100,
                .get_in_sr = slave_audio_downstream_get_in_sr,
                .get_out_sr = slave_audio_downstream_get_out_sr,
                .get_total = slave_audio_downstream_get_max_delay_ms,
                .get_size = slave_audio_downstream_get_cur_delay_ms,

                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
            },
        },
    },
#endif
#if TCFG_EQ_ENABLE&&TCFG_MUSIC_EQ_ENABLE
    {
        .attr  = ADAPTER_STREAM_ATTR_MUSICEQ,
        .value = {
            .musiceq = {
                .samplerate = WIRELESS_DECODE_SAMPLERATE,
#if TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_LR_DIFF
                .ch_num = 1,
#else
                .ch_num = 2,
#endif
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
            },
        },
    },
#endif
#if WIRELESS_AUTO_MUTE
    {
        .attr = ADAPTER_STREAM_ATTR_AUTO_MUTE,
        .value = {
            .auto_mute = {
                .parm = &auto_mute_parm,
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
            }
        },
    },
#endif
//数字音量节点

    {
        .attr  = ADAPTER_STREAM_ATTR_DVOL,
        .value = {
            .digital_vol = {
                .data_handle = NULL,//如果要关注处理前的数据可以在这里注册回调
                .volume = &downstream_digital_vol_parm,
            },
        },
    },
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
};

static int adapter_uac_vol_switch(int vol)
{
    u16 valsum = vol * (MAX_VOL + 1) / 100;

    if (valsum > MAX_VOL) {
        valsum = MAX_VOL;
    }
    return valsum;
}

static int slave_downstream_command_parse(void *priv, u8 *data)
{
    static u16 music_vol = 0;
    static u16 mic_vol = 0;

    if ((timer_get_ms() - cur_time) > 60) {
        cur_time = timer_get_ms();
        //printf("-%d,%d",data[0],data[1]);
        /* data[0] = adapter_uac_vol_switch(data[0]); */
        /* data[1] = adapter_uac_vol_switch(data[1]); */
        //printf("=%d,%d",data[0],data[1]);
        music_vol = (data[0] << 8) | data[1];
        mic_vol = data[2] & 0xffff;
        if (cur_music_vol != music_vol) {
            adapter_process_event_notify(ADAPTER_EVENT_SET_MUSIC_VOL, music_vol);
        }
        if (cur_mic_vol != mic_vol) {
            adapter_process_event_notify(ADAPTER_EVENT_SET_MIC_VOL, mic_vol);
        }
    }

    return MASTER_ENC_COMMAND_LEN;
}

static const struct adapter_decoder_fmt slave_audio_upstream = {
    .dec_type = ADAPTER_DEC_TYPE_MIC,
    .list	  = slave_audio_upstream_list,
    .list_num = sizeof(slave_audio_upstream_list) / sizeof(slave_audio_upstream_list[0]),
};
static const struct adapter_decoder_fmt slave_audio_downstream = {
    .dec_type = ADAPTER_DEC_TYPE_JLA,
    .list	  = slave_audio_downstream_list,
    .list_num = sizeof(slave_audio_downstream_list) / sizeof(slave_audio_downstream_list[0]),
#if TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR
    .dec_output_type = AUDIO_CH_LR,
#else
    .dec_output_type = AUDIO_CH_DIFF,
#endif
    .cmd_parse_cbk = slave_downstream_command_parse,

};
static const struct adapter_media_fmt slave_media_list[] = {
    [0] = {
#if WIRELESS_UPSTREAM_ENABLE
        .upstream =	&slave_audio_upstream,
#else
        .upstream =	NULL,
#endif
#if WIRELESS_DOWNSTREAM_ENABLE
        .downstream = &slave_audio_downstream,
#else
        .downstream = NULL,
#endif
    },
};

static const struct adapter_media_config slave_media_config = {
    .list = slave_media_list,
    .list_num = sizeof(slave_media_list) / sizeof(slave_media_list[0]),
};

//bt idev config
struct _idev_bt_parm idev_bt_parm_list = {
    .mode = BIT(IDEV_BLE),
};

u8 flag_mute_exdac = 0;
static int earphone_key_event_handler(struct sys_event *event)
{
    int ret = 0;
    struct key_event *key = &event->u.key;

    u16 key_event = event->u.key.event;

    static u8 key_poweroff_cnt = 0;
    static u8 flag_poweroff = 0;

    printf("slave key_event:%d %d %d\n", key_event, key->value, key->event);
    switch (key_event) {
#if (WIRELESS_LLNS_ENABLE)

    case KEY_WIRELESS_MIC_DENOISE_SET:
#if !ALWAYS_RUN_STREAM	//未连接也可以设置降噪档位
        if (!wireless_conn_status) {
            printf("no conn,break");
            break;
        }
#endif
        command++;
        if (command > WIRELESS_MIC_DENOISE_LEVEL_MAX) {
            command = WIRELESS_MIC_DENOISE_OFF;
        }
        printf("KEY_WIRELESS_MIC_LLNS_SET %d\n", command);
        if (wireless_mic_media) {
            switch (command) {
            case 1:
                //关闭降噪
                /* adapter_media_stop(wireless_mic_media); */
                /* adapter_media_start(wireless_mic_media); */
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_LLNS_SWITCH, 1, (int *)0);
                break;
            case 2:
                //轻度降噪
                /* adapter_media_stop(wireless_mic_media); */
                /* adapter_media_start(wireless_mic_media); */
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_LLNS_SWITCH, 1, (int *)1);
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_LLNS_SET_PARM, 1, (int *)&llns_parm_1);
                break;
            case 3:
                //中度降噪
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_LLNS_SWITCH, 1, (int *)1);
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_LLNS_SET_PARM, 1, (int *)&llns_parm_2);
                break;
            case 4:
                //深度降噪
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_LLNS_SWITCH, 1, (int *)1);
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_LLNS_SET_PARM, 1, (int *)&llns_parm_3);
                break;
            default:
                break;
            }
        }
        break;
#endif


#if (WIRELESS_DENOISE_ENABLE)

    case KEY_WIRELESS_MIC_DENOISE_SET:
        //主机响应实际按键事件， 通过命令转发到从机， 由从机实现降噪
        //主机发出按键降噪设置命令, 1:关闭降噪， 2:轻度降噪，3:中度降噪，4:深度降噪
        //主机的降噪等级可以通过VM记忆， 根据方案需求自行添加， 记忆command即可，每次开机读取默认值
#if !ALWAYS_RUN_STREAM
        if (!wireless_conn_status) {
            printf("no conn,break");
            break;
        }
#endif
        command++;
        if (command > WIRELESS_MIC_DENOISE_LEVEL_MAX) {
            command = WIRELESS_MIC_DENOISE_OFF;
        }
        printf("KEY_WIRELESS_MIC_DENOISE_SET %d\n", command);

        if (wireless_mic_media) {
            switch (command) {
            case 1:
                //关闭降噪
                /* adapter_media_stop(wireless_mic_media); */
                /* adapter_media_start(wireless_mic_media); */
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_DENOISE_SWITCH, 1, (int *)0);
                break;
            case 2:
                //轻度降噪
                /* adapter_media_stop(wireless_mic_media); */
                /* adapter_media_start(wireless_mic_media); */
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_DENOISE_SWITCH, 1, (int *)1);
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_DENOISE_SET_PARM, 1, (int *)&denoise_parm_1);
                break;
            case 3:
                //中度降噪
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_DENOISE_SWITCH, 1, (int *)1);
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_DENOISE_SET_PARM, 1, (int *)&denoise_parm_2);
                break;
            case 4:
                //深度降噪
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_DENOISE_SWITCH, 1, (int *)1);
                adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_DENOISE_SET_PARM, 1, (int *)&denoise_parm_3);
                break;
            default:
                break;
            }
        }
        break;
#endif//#if (WIRELESS_DENOISE_ENABLE)
#if WIRELESS_ECHO_ENABLE||WIRELESS_PLATE_REVERB_ENABLE

    case KEY_WIRELESS_MIC_ECHO_SET:
        if (!wireless_conn_status) {
            printf("no conn,break");
            break;
        }
        echo_cmd++;
        if (echo_cmd > WIRELESS_MIC_ECHO_ON) {
            echo_cmd = WIRELESS_MIC_ECHO_OFF;
        }
        printf("KEY_WIRELESS_MIC_ECHO %d\n", echo_cmd);

        if (wireless_mic_media) {
            adapter_decoder_ioctrl(wireless_mic_media->updecode, ADAPTER_DEC_IOCTRL_CMD_ECHO_EN, 1, (int *)(echo_cmd));
        }
        break;
#endif//
    case KEY_MUSIC_PREV:
        printf("KEY_MUSIC_PREV\n");
        adapter_wireless_enc_command_send(WIRELESS_TRANSPORT_CMD_PREV);
        break;
    case KEY_MUSIC_NEXT:
        printf("KEY_MUSIC_NEXT\n");
        adapter_wireless_enc_command_send(WIRELESS_TRANSPORT_CMD_NEXT);
        break;
    case KEY_MUSIC_PP:
        printf("KEY_MUSIC_PP\n");
        adapter_wireless_enc_command_send(WIRELESS_TRANSPORT_CMD_PP);
        break;
    case KEY_VOL_DOWN:
        printf("KEY_VOL_DOWN\n");
        adapter_wireless_enc_command_send(WIRELESS_TRANSPORT_CMD_VOL_DOWN);
        break;
    case KEY_VOL_UP:
        printf("KEY_VOL_UP\n");
        adapter_wireless_enc_command_send(WIRELESS_TRANSPORT_CMD_VOL_UP);
        break;
    case KEY_SW_SAMETIME_OUTPUT:
        //开关同时输出到dac
        flag_mute_exdac = !flag_mute_exdac;
        printf("flag_mute_exdac = %d", flag_mute_exdac);
        break;
    case KEY_MODE_SW:
#if TCFG_DUPLEX_EARPHONE_MODE_SW
        duplex_earphone_mode_switch(EARPHONE_MODE);
#endif
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

static int earphone_event_handle_callback(struct sys_event *event)
{
    //处理用户关注的事件
    int ret = 0;
    switch (event->type) {
    case SYS_KEY_EVENT:
        ret = earphone_key_event_handler(event);
        break;
    case SYS_DEVICE_EVENT:
        app_common_device_event_deal(event);

        switch ((u32)event->arg) {
        case DEVICE_EVENT_FROM_ADAPTER:
            switch (event->u.dev.event) {
            case ADAPTER_EVENT_CONNECT :
                printf("ADAPTER_EVENT_CONNECT\n");
                wireless_conn_status = 1;
                earphone_tone_play(TONE_BT_CONN, 1);
                ui_update_status(STATUS_BT_CONN);
                sys_auto_shut_down_disable();
                break;
            case ADAPTER_EVENT_DISCONN :
                printf("ADAPTER_EVENT_DISCONN\n");
                wireless_conn_status = 0;
                earphone_tone_play(TONE_BT_DISCONN, 1);
                ui_update_status(STATUS_BT_DISCONN);
                sys_auto_shut_down_enable();
                break;
            case ADAPTER_EVENT_IDEV_MEDIA_CLOSE :
            case ADAPTER_EVENT_ODEV_MEDIA_CLOSE :
                cur_music_vol = -1;
                cur_mic_vol = -1;
                break;
            case ADAPTER_EVENT_ODEV_MEDIA_OPEN :
            case ADAPTER_EVENT_IDEV_MEDIA_OPEN :
                break;
            case ADAPTER_EVENT_POWEROFF:
                ret = 1;
                break;
            case ADAPTER_EVENT_SET_MUSIC_VOL :
                cur_music_vol = event->u.dev.value;
                printf("vol_l=%d,vol_r=%d", cur_music_vol >> 8, cur_music_vol & 0xff);
                //audio_fade_in_fade_out(cur_music_vol >> 8, cur_music_vol & 0xff, 1);
                adapter_decoder_set_vol(wireless_mic_media->downdecode, BIT(0), cur_music_vol >> 8);
                adapter_decoder_set_vol(wireless_mic_media->downdecode, BIT(1), cur_music_vol & 0xff);
                break;
            case ADAPTER_EVENT_SET_MIC_VOL :
                cur_mic_vol = event->u.dev.value;
                printf("cur_mic_vol = %d", cur_mic_vol);
                adapter_decoder_set_vol(wireless_mic_media->updecode, BIT(0), (u8)cur_mic_vol);
                break;
            }
        }

        break;
    default:
        break;

    }
    return ret;
}

extern struct adc_platform_data adc_data;
extern struct audio_adc_hdl adc_hdl;
void app_main_run(void)
{
    clk_set("sys", (48 * 1000000L));
    ui_update_status(STATUS_POWERON);
    audio_adc_init(&adc_hdl, &adc_data);
    earphone_tone_play(TONE_POWER_ON, 1);
    while (wl_mic_tone_get_dec_status() == TONE_START) {
        os_time_dly(1);
    }

    #ifdef LITEEMF_ENABLED
    extern void liteemf_app_start(void);
    liteemf_app_start();
    #endif

    while (1) {
        //初始化
        adapter_adc_init();
        struct idev *idev = adapter_idev_open(ADAPTER_IDEV_BT, (void *)&idev_bt_parm_list);
        struct odev *odev = adapter_odev_open(ADAPTER_ODEV_DAC, NULL);
        struct adapter_media *media = adapter_media_open((struct adapter_media_config *)&slave_media_config);
        printf("earphone ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        ASSERT(idev);
        ASSERT(odev);
        ASSERT(media);
        struct adapter_pro *pro = adapter_process_open(idev, odev, media, earphone_event_handle_callback);//用户想拦截处理的事件

        ASSERT(pro, "adapter_process_open fail!!\n");
        wireless_mic_media = media;

        //执行(包括事件解析、事件执行、媒体启动/停止, HID等事件转发)
        adapter_process_run(pro);

        wireless_mic_media = NULL;
        //退出/关闭
        adapter_process_close(&pro);
        adapter_media_close(&media);
        adapter_idev_close(idev);
        adapter_odev_close(odev);

        ui_update_status(STATUS_POWEROFF);
        ///run idle off poweroff
        printf("enter poweroff !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        earphone_tone_play(TONE_POWER_OFF, 1);
        while (wl_mic_tone_get_dec_status() == TONE_START) {
            os_time_dly(1);
        }
        if (get_charge_online_flag()) {
            printf("charge_online,cpu reset");
            cpu_reset();
        } else {
            power_set_soft_poweroff();
        }
    }
}


#endif
