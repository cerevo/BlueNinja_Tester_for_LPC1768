#ifndef _DAT_BUF_H_
#define _DAT_BUF_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#define DAT_BUF_LEN (6)
    
typedef struct {
    uint32_t data[DAT_BUF_LEN];
    uint8_t r_idx;
    uint8_t w_idx;
}   DAT_BUF;

void init_dat_buf(DAT_BUF *buf);
int put_dat_buf(DAT_BUF *buf, uint32_t val);
bool get_dat_buf(DAT_BUF *buf, uint32_t *out);
int get_series_dat_buf(DAT_BUF *buf, uint32_t *out_buf, uint32_t out_sz);

#ifdef __cplusplus
}
#endif

#endif
