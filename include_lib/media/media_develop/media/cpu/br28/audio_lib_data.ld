

        media_data_start = .;

        . = ALIGN(4);
        audio_sync_data_begin = .;
            *(.audio_sync_data)
        audio_sync_data_end = .;

        . = ALIGN(4);
		/* EQ_COEFF_BASE = . ; */
		/* . = EQ_COEFF_BASE + 4 * EQ_SECTION_NUM * 14; */

        . = ALIGN(4);
        *(.aec_data)
        . = ALIGN(4);
        *(.res_data)
        . = ALIGN(4);
        *(.ns_data)
        . = ALIGN(4);
        *(.der_data)
        . = ALIGN(4);
        *(.nlp_data)
        . = ALIGN(4);
        *(.qmf_data)
        . = ALIGN(4);
        *(.fft_data)
        . = ALIGN(4);
		*(.bt_audioplc_data)
    	*(.jla_decoder_data)
    	*(.jla_dec_data)
#if (TCFG_ENC_JLA_ENABLE || TCFG_DEC_JLA_ENABLE)
			*(.jla_codec_ari_c_const)
			*(.jla_codec_ari_c_data)
			*(.jla_codec_c_data)

#if (WIRELESS_MIC_CODING_SAMPLERATE == WIRELESS_MIC_DECODE_SAMPLERATE)
	        *(.jla_codec_t8k_code)
	        *(.jla_codec_t16k_code)
	        *(.jla_codec_t24k_code)
	        *(.jla_codec_t32k_code)
	        *(.jla_codec_t441k_code)
#else
	        *(.jla_codec_t16k_code)
	        *(.jla_codec_t441k_code)
#endif
#endif
		*(.sms_data)

        *(.pcm_data)
		*(.cvsd_data)
		*(.g729_data)
		*(.g726_data)
		*(.wtg_dec_data)
		*(.wtgv2_data)
		*(.wtgv2dec_data)
		*(.mp3_data)
		*(.mp3_dec_data)
		*(.msbc_data)
		*(.mty_data)
		*(.mp3tsy_dec_data)
		*(.sbc_data)
		*(.sine_data)
        *(.wav_dec_data)
		*(.wav_data)
		*(.wma_data)
		*(.wma_dec_data)
		*(.amr_data)
		*(.amr_dec_data)
		*(.midi_data)
		*(.ape_data)
		*(.ape_dec_data)
		*(.audio_decoder_data)
		*(.dts_data)
		*(.dts_dec_data)
		*(.flac_data)
		*(.flac_dec_data)
		*(.m4a_data)
		*(.m4a_dec_data)
        *(.mp3_encode_data)
		*(.media_device_data)
		*(.audio_encoder_data)
		*(.mixer_data)
		*(.stream_data)
		*(.dec_server_data)
		*(.rec_server_data)
		*(.auto_mute_data)
		*(.plc_data)
		*(.wireless_sync_data)

		*(.resample_cal_data)
		*(.bt_compressor_data)
		*(.bt_crossOver_data)
		*(.bt_limiter_data)
		*(.bt_compressor_sparse_data)
		*(.bt_limiter_sparse_data)
		*(.bt_crossOver_sparse_data)

		*(.iap_data)
		*(.audio_bfilt_data)
		*(.audio_buf_sync_data)
		*(.audio_dec_app_data)
		*(.audio_dec_app_sync_data)
		*(.audio_dig_vol_data)
		*(.audio_echo_reverb_data)
		*(.audio_echo_src_data)
		*(.audio_energy_detect_data)
		*(.audio_eq_data)
		*(.audio_drc_data)
		*(.audio_eq_drc_apply_data)
		*(.audio_equalloudness_data)
		*(.audio_howling_data)
		*(.audio_noisegate_data)
		*(.audio_output_dac_data)
		*(.audio_pemafrow_data)
		*(.audio_pitch_data)
		*(.audio_pitchspeed_data)
		*(.audio_surround_data)
		*(.audio_vbass_data)
		*(.audio_vocal_remove_data)
		*(.audio_vocal_tract_synthesis_data)
		*(.eq_config_data)
		*(.eq_config_protocol_data)
		*(.spectrum_eq_data)
		*(.spectrum_fft_data)
		*(.audio_localtws_data)
		*(.audio_dec_localtws_data)
		*(.audio_dec_a2dp_data)
		*(.audio_dec_esco_data)
		*(.audio_dec_file_data)
		*(.audio_dec_pcm_data)
		*(.audio_resample_data)
		*(.audio_splicing_data)
		*(.channel_switch_data)
		*(.sw_drc_data)
		*(.aptx_decoder_data)
		*(.ldac_decoder_data)
		*(.speex_encoder_data)
		*(.sbc_encoder_data)
        *(.sbc_eng_data)
		*(.opus_encoder_data)
		*(.prevent_task_fill_data)
		*(.audio_cfifo_data)
		*(.audio_track_data)
		*(.audio_adc_data)
		*(.audio_dac_data)
		*(.audio_linein_data)
		*(.audio_link_data)
		*(.audio_sample_sync_data)
		*(.audio_wireless_sync_data)
		*(.audio_spdif_data)
		*(.audio_src_data)
		*(.audio_src_base_data)
		*(.effectrs_sync_data)
		*(.eq_design_data)
		*(.hw_eq_data)
		*(.iic_data)
		*(.mic_cap_data)
		*(.sbc_hwaccel_data)

		*(.resample_fastcal_data)
		*(.aac_data)
		*(.alac_data)
        . = ALIGN(4);
		*(.dec2tws_mem)
        *(.midi_mem)
		*(.midi_ctrl_mem)
		*(.lfwordana_data)

		/* *(.crossOver_code) */
		/* *(.crossOver_const) */
		/* *(.bt_crossOver_const) */
		/* *(.bt_crossOver_code) */
		/* . = ALIGN(4); */

		/* *(.compressor_code) */
		/* *(.compressor_const) */
		/* *(.bt_compressor_const) */
		/* *(.bt_compressor_code) */
		/* . = ALIGN(4); */

		/* *(.limiter_code) */
		/* *(.limiter_const) */
		/* *(.bt_limiter_const) */
		/* *(.bt_limiter_code) */
		/* . = ALIGN(4); */

        *(.anc_data)
        *(.anc_user_data)
        *(.anc_box_data)
        *(.anc_btspp_data)
        *(.anc_debug_data)

        *(.dac_buff)
        *(.dec_mix_buff)
		*(.pitchshifter_data)
        media_data_end = .;




