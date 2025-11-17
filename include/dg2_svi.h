#ifndef DG2_SVI_H_
#define DG2_SVI_H_

#if 0

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include "dg2_curve.h"
#include "dg2_disp.h"
#include <stdint.h>

#define DG2_VP_CURVE_BUFFER 0x1000
#define DG2_CURVE_BUFFER_SIZE 0x800

typedef enum dg2_svi_vp {
    DG2_SVI_VP_SYSTEM_RESET = 0x04,
    DG2_SVI_VP_VER = 0x0F,
    DG2_SVI_VP_RTC = 0x10,
    DG2_SVI_VP_PIC_NOW = 0x14,
    DG2_SVI_VP_GUI_STATUS = 0x15,
    DG2_SVI_VP_LED_NOW = 0x31,
    DG2_SVI_VP_LED_CONFIG = 0x82,
    DG2_SVI_VP_PIC_SET = 0x84,
    DG2_SVI_VP_RTC_SET = 0x9C,
    DG2_SVI_VP_DYNAMIC_CURVE = 0x300,
} dg2_svi_vp;

typedef struct __attribute__((packed)) dg2_svi_version {
    uint8_t gui;
    uint8_t os;
} dg2_svi_version;

typedef enum dg2_svi_gui_status {
    DG2_SVI_GUI_STATUS_FREE = 0,
    DG2_SVI_GUI_STATUS_PROCESSING = 1
} dg2_svi_gui_status;

typedef struct __attribute__((packed)) dg2_svi_rtc {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t reserved;
} dg2_svi_rtc;

dg2_error dg2_disp_system_reset(dg2_disp *disp);

dg2_error dg2_disp_version_get(dg2_disp *disp, dg2_svi_version *version);

dg2_error dg2_disp_rtc_get(dg2_disp *disp, dg2_svi_rtc *rtc);
dg2_error dg2_disp_rtc_set(dg2_disp *disp, dg2_svi_rtc *rtc);

dg2_error dg2_disp_gui_status_get(dg2_disp *disp, dg2_svi_gui_status *gui_status);

dg2_error dg2_disp_page_get(dg2_disp *disp, uint16_t *page);
dg2_error dg2_disp_page_set(dg2_disp *disp, uint16_t page);

dg2_error dg2_disp_led_get_bright(dg2_disp *disp, uint8_t *bright);
dg2_error dg2_disp_led_set_bright(dg2_disp *disp, uint8_t bright);

dg2_error dg2_disp_curve_buffer_get(dg2_disp *disp, dg2_curve_ch curve_ch, uint16_t *storage_ptr, uint16_t *data_len);
dg2_error dg2_disp_curve_buffer_set(dg2_disp *disp, dg2_curve_ch curve_ch, uint16_t storage_ptr, uint16_t data_len);
dg2_error dg2_disp_curve_buffer_read(dg2_disp *disp, dg2_curve_ch curve_ch, int16_t *buff, uint8_t count);
dg2_error dg2_disp_curve_buffer_write(dg2_disp *disp, dg2_curve_ch curve_ch, int16_t *buff, uint8_t count);

dg2_error dg2_disp_curve_clear(dg2_disp *disp, dg2_curve_ch curve_ch);
dg2_error dg2_disp_curve_app(dg2_disp *disp, dg2_curve_ch curve_ch, int16_t data);
dg2_error dg2_disp_curves_write(dg2_disp *disp, dg2_curves *curves);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // 0

#endif /* DG2_SVI_H_ */
