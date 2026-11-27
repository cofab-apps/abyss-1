/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include "plat.h"
#define APP_NAME "Seamonster Choir"
#define APP_SPLASH_RGB RGB(84, 231, 204)
#define APP_BLOCK 128u
#define APP_CANVAS_PX (240u * 68u)
#define APP_TOUCH
#define APP_SYSEX
int app_sysex(const uint8_t *p, uint32_t n);
/* Panel calibration and favourite use separate A/B sector pairs. */
enum { OBJ_PANEL, OBJ_FAVOURITE, OBJ_NOBJ };
#define APP_STORE_MAP {0xCE000u, 0xD8000u}
void app_init(void);
void app_ui_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_frame(void);
void app_input_only(void);
void app_panic(void);
void app_say(const char *a, const char *b);
