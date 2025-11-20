#ifndef DG2_CRC_H_
#define DG2_CRC_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <stdint.h>
#include <stddef.h>

typedef uint16_t (*dg2_cb_crc)(uint8_t *data, size_t size);

uint16_t dg2_crc(uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* DG2_CRC_H_ */
