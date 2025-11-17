#include "dg2_svi.h"

/*
 * 5.1 System Variable Interface
 */

#if 0

dg2_error dg2_disp_system_reset(dg2_disp *disp)
{
    DG2_ASSERT(disp);

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, DG2_SVI_VP_SYSTEM_RESET);
    dg2_pkt_insert_word(&pkt, 0x55AA5AA5);

    return dg2_disp_pkt_exchange_ok(disp, &pkt);
}

dg2_error dg2_disp_version_get(dg2_disp *disp, dg2_svi_version *version)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(version);

    return dg2_disp_vp_read_mul(disp, DG2_SVI_VP_VER, (int16_t*)version, sizeof(dg2_svi_version) / sizeof(uint16_t));
}

dg2_error dg2_disp_page_get(dg2_disp *disp, uint16_t *page)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(page);

    return dg2_disp_vp_read(disp, DG2_SVI_VP_PIC_NOW, (int16_t*)page);
}

dg2_error dg2_disp_page_set(dg2_disp *disp, uint16_t page)
{
    DG2_ASSERT(disp);

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, DG2_SVI_VP_PIC_SET);
    dg2_pkt_insert_halfword(&pkt, 0x5A01);
    dg2_pkt_insert_halfword(&pkt, page);

    return dg2_disp_pkt_exchange_ok(disp, &pkt);
}

dg2_error dg2_disp_rtc_get(dg2_disp *disp, dg2_svi_rtc *rtc)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(rtc);

    return dg2_disp_vp_read_bytes(disp, DG2_SVI_VP_RTC, (uint8_t*)rtc, sizeof(dg2_svi_rtc));
}

dg2_error dg2_disp_rtc_set(dg2_disp *disp, dg2_svi_rtc *rtc)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(rtc);

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, DG2_SVI_VP_RTC_SET);
    dg2_pkt_insert_halfword(&pkt, 0x5AA5);

    dg2_pkt_insert_byte(&pkt, rtc->year);
    dg2_pkt_insert_byte(&pkt, rtc->month);
    dg2_pkt_insert_byte(&pkt, rtc->day);
    dg2_pkt_insert_byte(&pkt, rtc->hour);
    dg2_pkt_insert_byte(&pkt, rtc->minute);
    dg2_pkt_insert_byte(&pkt, rtc->second);

    return dg2_disp_pkt_exchange_ok(disp, &pkt);
}

dg2_error dg2_disp_gui_status_get(dg2_disp *disp, dg2_svi_gui_status *gui_status)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(gui_status);

    dg2_error error;
    int16_t tmp;
    if ((error = dg2_disp_vp_read(disp, DG2_SVI_VP_GUI_STATUS, &tmp)) != DG2_OK) {
        return error;
    }

    *gui_status = tmp;

    return DG2_OK;
}

dg2_error dg2_disp_led_get_bright(dg2_disp *disp, uint8_t *bright)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(bright);

    dg2_error error;
    uint16_t tmp;
    if ((error = dg2_disp_vp_read(disp, DG2_SVI_VP_LED_NOW, (int16_t*)&tmp))) {
        return error;
    }

    *bright = ((uint8_t*)&tmp)[0];

    return DG2_OK;
}

dg2_error dg2_disp_led_set_bright(dg2_disp *disp, uint8_t bright)
{
    DG2_ASSERT(disp);

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, DG2_SVI_VP_LED_CONFIG);
    dg2_pkt_insert_byte(&pkt, bright);

    return dg2_disp_pkt_exchange_ok(disp, &pkt);
}

/*
 * Curves
 */

dg2_error dg2_disp_curve_buffer_get(dg2_disp *disp, dg2_curve_ch curve_ch, uint16_t *storage_ptr, uint16_t *data_len)
{
    DG2_ASSERT(disp);

    dg2_error error;
    uint16_t tmp[2];
    if ((error = dg2_disp_vp_read_mul(disp, DG2_SVI_VP_DYNAMIC_CURVE + 2 * curve_ch, (int16_t*)tmp, DG2_ARRAY_SIZE(tmp))) != DG2_OK) {
        return error;
    }

    if (storage_ptr) {
        *storage_ptr = tmp[0];
    }

    if (data_len) {
        *data_len = tmp[1];
    }

    return DG2_OK;
}

dg2_error dg2_disp_curve_buffer_set(dg2_disp *disp, dg2_curve_ch curve_ch, uint16_t storage_ptr, uint16_t data_len)
{
    DG2_ASSERT(disp);

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, DG2_SVI_VP_DYNAMIC_CURVE + 2 * curve_ch);
    dg2_pkt_insert_halfword(&pkt, storage_ptr);
    dg2_pkt_insert_halfword(&pkt, data_len);

    return dg2_disp_pkt_exchange_ok(disp, &pkt);
}

dg2_error dg2_disp_curve_buffer_read(dg2_disp *disp, dg2_curve_ch curve_ch, int16_t *buff, uint8_t count)
{
    DG2_ASSERT(disp);

    return dg2_disp_vp_read_mul(disp, DG2_VP_CURVE_BUFFER + DG2_CURVE_BUFFER_SIZE * curve_ch, (int16_t*)buff, count);
}

dg2_error dg2_disp_curve_buffer_write(dg2_disp *disp, dg2_curve_ch curve_ch, int16_t *buff, uint8_t count)
{
    DG2_ASSERT(disp);

    return dg2_disp_vp_write_mul(disp, DG2_VP_CURVE_BUFFER + DG2_CURVE_BUFFER_SIZE * curve_ch, (int16_t*)buff, count);
}

dg2_error dg2_disp_curve_clear(dg2_disp *disp, dg2_curve_ch curve_ch)
{
   DG2_ASSERT(disp);

   dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, DG2_SVI_VP_DYNAMIC_CURVE + 0x01 + 2 * curve_ch);
   dg2_pkt_insert_halfword(&pkt, 0x0000);

   return dg2_disp_pkt_exchange_ok(disp, &pkt);
}

dg2_error dg2_disp_curve_app(dg2_disp *disp, dg2_curve_ch curve_ch, int16_t data)
{
    DG2_ASSERT(disp);

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, DG2_SVI_VP_DYNAMIC_CURVE + 0x10);
    dg2_pkt_insert_halfword(&pkt, 0x5AA5);
    dg2_pkt_insert_byte(&pkt, 0x01);
    dg2_pkt_insert_byte(&pkt, 0x00);
    dg2_pkt_insert_byte(&pkt, curve_ch);
    dg2_pkt_insert_byte(&pkt, 0x01);
    dg2_pkt_insert_halfword(&pkt, data);

    return dg2_disp_pkt_exchange_ok(disp, &pkt);
}

dg2_error dg2_disp_curves_write(dg2_disp *disp, dg2_curves *curves)
{
    DG2_ASSERT(disp);
    DG2_ASSERT(curves);

    uint8_t curve_count = 0;

    for (uint8_t i = 0; i < curves->curve_count; ++i) {
        dg2_curve *curve = curves->curves + i;

        if (!curve->data_len) {
            continue;
        }

        curve_count++;
    }

    if (!curve_count) {
        return DG2_OK;
    }

    dg2_pkt pkt = dg2_disp_pkt_init(disp, DG2_CMD_WRITE, DG2_SVI_VP_DYNAMIC_CURVE + 0x10);
    dg2_pkt_insert_halfword(&pkt, 0x5AA5);
    dg2_pkt_insert_byte(&pkt, curve_count);
    dg2_pkt_insert_byte(&pkt, 0x00);

    for (uint8_t i = 0; i < curves->curve_count; ++i) {
        dg2_curve *curve = curves->curves + i;

        if (!curve->data_len) {
            continue;
        }

        dg2_pkt_insert_byte(&pkt, curve->ch);
        dg2_pkt_insert_byte(&pkt, curve->data_len);
        dg2_pkt_insert_halfwords(&pkt, curve->buff, curve->data_len);

        curve->data_len = 0;
    }

    return dg2_disp_pkt_exchange_ok(disp, &pkt);
}

#endif // 0
