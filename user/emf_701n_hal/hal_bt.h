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

#ifndef	_hal_bt_h
#define	_hal_bt_h
#include "hw_config.h"
#include "system/includes.h"
#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************************************************
**	Hardware  Defined
********************************************************************************************************************/
#define CFG_RF_24G_CODE_ID  	(0x28)

//选择物理层
#ifndef SELECT_BLE_RF_PHY
#define  SELECT_BLE_RF_PHY                 CONN_SET_1M_PHY//1M:CONN_SET_1M_PHY 2M:CONN_SET_2M_PHY CODED:CONN_SET_CODED_PHY
#endif
//选择CODED类型:S2 or S8
#ifndef SELECT_BLE_RF_CODED_S2_OR_S8
#define  SELECT_BLE_RF_CODED_S2_OR_S8      CONN_SET_PHY_OPTIONS_S2//S2:CONN_SET_PHY_OPTIONS_S2 S8:CONN_SET_PHY_OPTIONS_S8
#endif

/*******************************************************************************************************************
**	Parameters
********************************************************************************************************************/

/*******************************************************************************************************************
**	event
********************************************************************************************************************/


/*******************************************************************************************************************
**	Functions
********************************************************************************************************************/
extern int sys_bt_event_handler(struct sys_event *event);
extern bool hal_bt_select_phy(uint16_t handle);

#ifdef __cplusplus
}
#endif
#endif

