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
#if API_AUDIO_ENABLE
#include "api/audio/api_audio.h"
#include "usb/device/uac_audio.h"
#include "usb_std_class_def.h"

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

/*****************************************************************************************************
**	static Function
******************************************************************************************************/

/*****************************************************************************************************
**  Function
******************************************************************************************************/



/*******************************************************************
** Parameters:		
** Returns:	
** Description:		
*******************************************************************/
void hal_audio_spk_set_vol(uint8_t id,uint16_t l_vol, uint16_t r_vol)
{
	uint16_t vol;
	logd("spk_set_vol=%d %d",l_vol,r_vol);
	// uac_mute_volume(SPK_FEATURE_UNIT_ID,l_vol,r_vol);
	vol = (l_vol<<8) | r_vol;
	adapter_process_event_notify(ADAPTER_EVENT_SET_MUSIC_VOL, vol);
}
void hal_audio_mic_set_vol(uint8_t id,uint16_t mic_vol)
{
	logd("mic_set_vol=%d",mic_vol);
	// uac_mute_volume(MIC_FEATURE_UNIT_ID,mic_vol,0);
	adapter_process_event_notify(ADAPTER_EVENT_SET_MIC_VOL, mic_vol);
}
void hal_audio_open_spk(uint8_t id,api_audio_t *paudio)
{
	uac_speaker_stream_open(paudio->spk_sampel.rate, paudio->spk_sampel.channel);
}
void hal_audio_close_spk(uint8_t id)
{
	uac_speaker_stream_close();
}
void hal_audio_open_mic(uint8_t id,api_audio_t *paudio)
{
	uac_mic_stream_open(paudio->mic_sampel.rate, API_AUDIO_MIC_SIZE(paudio), paudio->mic_sampel.channel);
}
void hal_audio_close_mic(uint8_t id)
{
	uac_mic_stream_close();
}
bool hal_audio_init(uint8_t id,api_audio_t *paudio)
{
	return true;
}

#endif






