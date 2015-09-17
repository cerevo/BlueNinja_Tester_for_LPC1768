#ifndef _CMD_BUF_H_
#define _CMD_BUF_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_BUF_LEN (5)

typedef struct {
    uint8_t data[CMD_BUF_LEN];
    uint8_t r_idx;
    uint8_t w_idx;
}   CMD_BUF;

void init_cmd_buf(CMD_BUF *buf);
int put_cmd_buf(CMD_BUF *buf, uint8_t val);
int get_cmd_buf(CMD_BUF *buf);
int get_cmd_series_buf(CMD_BUF *buf, uint8_t *out_buf, uint8_t out_sz);

#ifdef __cplusplus
}
#endif

#endif
