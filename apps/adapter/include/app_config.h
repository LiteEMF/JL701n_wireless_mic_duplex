#ifndef APP_CONFIG_H
#define APP_CONFIG_H





//宏定义
#include "macro.h"

//用户app功能配置
#include "user_config.h"


//用户选择earphone 或者 dongle
#ifndef CONFIG_DUPLEX_ROLE_SEL		//APP_WIRELESS_MASTER APP_WIRELESS_SLAVE
#define CONFIG_DUPLEX_ROLE_SEL		APP_WIRELESS_MASTER
#endif

//用户板级功能配置
#include "board_config.h"

//用户默认宏配置
#include "macro_default.h"

//宏编译预警
// #include "macro_check.h"

#endif //APP_CONFIG_H

