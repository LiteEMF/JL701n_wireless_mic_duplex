// *INDENT-OFF*
#include "app_config.h"

#ifdef __SHELL__

##!/bin/sh

${OBJDUMP} -D -address-mask=0x7ffffff -print-imm-hex -print-dbg -mcpu=r3 $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data  $1.elf data.bin
${OBJCOPY} -O binary -j .moveable_slot $1.elf mov_slot.bin
${OBJCOPY} -O binary -j .data_code $1.elf data_code.bin
${OBJCOPY} -O binary -j .overlay_aec $1.elf aec.bin
${OBJCOPY} -O binary -j .overlay_aac $1.elf aac.bin
${OBJCOPY} -O binary -j .psr_data_code $1.elf psr_data_code.bin

${OBJDUMP} -section-headers -address-mask=0x7ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 >  symbol_tbl.txt

cat text.bin data.bin mov_slot.bin data_code.bin aec.bin aac.bin psr_data_code.bin > app.bin

/* if [ -f version ]; then */
    /* host-client -project ${NICKNAME}$2 -f app.bin version $1.elf p11_code.bin br28loader.bin br28loader.uart uboot.boot uboot.boot_debug ota.bin ota_debug.bin isd_config.ini */
/* else */
    /* host-client -project ${NICKNAME}$2 -f app.bin $1.elf  p11_code.bin br28loader.bin br28loader.uart uboot.boot uboot.boot_debug ota.bin ota_debug.bin isd_config.ini */

/* fi */
#if defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_REVERB)&& TCFG_AUDIO_MIC_EFFECT_POST_EQ_ENABLE
cp ./effect_file/mic_effect_reverb_full.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_REVERB)
cp ./effect_file/mic_effect_reverb.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_ECHO)&& TCFG_AUDIO_MIC_EFFECT_POST_EQ_ENABLE
cp ./effect_file/mic_effect_echo_full.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_ECHO)
cp ./effect_file/mic_effect_echo.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_REVERB_ECHO)
cp ./effect_file/mic_effect_full.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_MEGAPHONE)
cp ./effect_file/mic_effect_megaphone.bin eq_cfg_hw.bin
#elif defined(SOUND_TRACK_2_P_X_CH_CONFIG) &&SOUND_TRACK_2_P_X_CH_CONFIG&& (TWO_POINT_X_SPECIAL_CONFIG == 0)
cp ./effect_file/music_2to1_2to2.bin eq_cfg_hw.bin
#elif defined(SOUND_TRACK_2_P_X_CH_CONFIG) &&SOUND_TRACK_2_P_X_CH_CONFIG && TWO_POINT_X_SPECIAL_CONFIG
cp ./effect_file/music_2to1_2to2_special.bin eq_cfg_hw.bin
#elif defined(TCFG_DYNAMIC_EQ_ENABLE)&& TCFG_DYNAMIC_EQ_ENABLE && defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
cp ./effect_file/music_advance_linein.bin eq_cfg_hw.bin
#elif defined(TCFG_DYNAMIC_EQ_ENABLE)&& TCFG_DYNAMIC_EQ_ENABLE
cp ./effect_file/music_advance.bin eq_cfg_hw.bin
#elif defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
cp ./effect_file/music_base_linein.bin eq_cfg_hw.bin
#else
cp ./effect_file/music_base.bin eq_cfg_hw.bin
#endif


/opt/utils/strip-ini -i isd_config.ini -o isd_config.ini

if [ -f version ]; then
    files="app.bin version $1.elf p11_code.bin br28loader.bin br28loader.uart uboot.boot uboot.boot_debug ota.bin ota_debug.bin isd_config.ini isd_download.exe ufw_maker.exe fw_add.exe"
else
    files="app.bin $1.elf p11_code.bin br28loader.bin br28loader.uart uboot.boot uboot.boot_debug ota.bin ota_debug.bin isd_config.ini isd_download.exe ufw_maker.exe fw_add.exe"

fi

host-client -project ${NICKNAME}$2_${APP_CASE} -f ${files}

#else

@echo off
Setlocal enabledelayedexpansion
@echo ********************************************************************************
@echo           SDK BR28
@echo ********************************************************************************
@echo %date%


cd /d %~dp0

set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set ELFFILE=sdk.elf
set bankfiles=
set LZ4_PACKET=.\lz4_packet.exe

REM %OBJDUMP% -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
%OBJCOPY% -O binary -j .text %ELFFILE% text.bin
%OBJCOPY% -O binary -j .data %ELFFILE% data.bin
%OBJCOPY% -O binary -j .moveable_slot %ELFFILE% mov_slot.bin
%OBJCOPY% -O binary -j .data_code %ELFFILE% data_code.bin
%OBJCOPY% -O binary -j .overlay_aec %ELFFILE% aec.bin
%OBJCOPY% -O binary -j .overlay_aac %ELFFILE% aac.bin
%OBJCOPY% -O binary -j .overlay_aptx %ELFFILE% aptx.bin
%OBJCOPY% -O binary -j .psr_data_code %ELFFILE% psr_data_code.bin

%OBJCOPY% -O binary -j .common %ELFFILE% common.bin


%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
REM %OBJDUMP% -t %ELFFILE% >  symbol_tbl.txt

copy /b text.bin + data.bin + mov_slot.bin + data_code.bin + aec.bin + aac.bin + psr_data_code.bin app.bin


del !bankfiles! common.bin text.bin data.bin bank.bin


#if defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_REVERB) && TCFG_AUDIO_MIC_EFFECT_POST_EQ_ENABLE
copy .\effect_file\mic_effect_reverb_full.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_REVERB)
copy .\effect_file\mic_effect_reverb.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_ECHO) && TCFG_AUDIO_MIC_EFFECT_POST_EQ_ENABLE
copy .\effect_file\mic_effect_echo_full.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_ECHO)
copy .\effect_file\mic_effect_echo.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_REVERB_ECHO)
copy .\effect_file\mic_effect_full.bin eq_cfg_hw.bin
#elif defined(TCFG_MIC_EFFECT_ENABLE)&& TCFG_MIC_EFFECT_ENABLE && (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_MEGAPHONE)
copy .\effect_file\mic_effect_megaphone.bin eq_cfg_hw.bin
#elif defined(SOUND_TRACK_2_P_X_CH_CONFIG) &&SOUND_TRACK_2_P_X_CH_CONFIG&& (TWO_POINT_X_SPECIAL_CONFIG == 0)
copy .\effect_file\music_2to1_2to2.bin eq_cfg_hw.bin
#elif defined(SOUND_TRACK_2_P_X_CH_CONFIG) &&SOUND_TRACK_2_P_X_CH_CONFIG && TWO_POINT_X_SPECIAL_CONFIG
copy .\effect_file\music_2to1_2to2_special.bin eq_cfg_hw.bin
#elif defined(TCFG_DYNAMIC_EQ_ENABLE)&& TCFG_DYNAMIC_EQ_ENABLE && defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
copy .\effect_file\music_advance_linein.bin eq_cfg_hw.bin
#elif defined(TCFG_DYNAMIC_EQ_ENABLE)&& TCFG_DYNAMIC_EQ_ENABLE
copy .\effect_file\music_advance.bin eq_cfg_hw.bin
#elif defined(LINEIN_MODE_SOLE_EQ_EN) && LINEIN_MODE_SOLE_EQ_EN
copy .\effect_file\music_base_linein.bin eq_cfg_hw.bin
#else
copy .\effect_file\music_base.bin eq_cfg_hw.bin
#endif

call download/adapter/download.bat

#endif
