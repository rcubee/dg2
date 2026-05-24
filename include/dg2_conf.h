#ifndef DG2_CONF_H_
#define DG2_CONF_H_

#define DG2_ASYNC_ENABLE 1
#define DG2_SYNC_ENABLE 1

#if !(defined(DG2_ASYNC_ENABLE) || defined(DG2_SYNC_ENABLE))
#error "Atleast one of specified macros must be defined: DG2_ASYNC_ENABLE or DG2_SYNC_ENABLE."
#endif

#define DG2_DISP_RX_BUFF_CAPACITY (256U)
#define DG2_DISP_TX_BUFF_CAPACITY (256U)

#endif // DG2_CONF_H_
