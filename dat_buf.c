#include <string.h>
#include "dat_buf.h"

void init_dat_buf(DAT_BUF *buf)
{
    memset(buf->data, 0, DAT_BUF_LEN * sizeof(uint32_t));
    buf->r_idx = 0;
    buf->w_idx = 0;
}

int put_dat_buf(DAT_BUF *buf, uint32_t val)
{
    buf->data[buf->w_idx] = val;
    buf->w_idx = (buf->w_idx + 1) % DAT_BUF_LEN;
    if (buf->w_idx == buf->r_idx) {
        buf->r_idx = (buf->r_idx + 1) % DAT_BUF_LEN;
    }
    
    return val;
}

bool get_dat_buf(DAT_BUF *buf, uint32_t *out)
{
    if (buf->w_idx == buf->r_idx) {
        return false;  //Buffer EMPTY
    }
    *out = buf->data[buf->r_idx];
    buf->r_idx = (buf->r_idx + 1) % DAT_BUF_LEN;
    
    return true;
}

//リングバッファを読みだして配列に格納
int get_series_dat_buf(DAT_BUF *buf, uint32_t *out_buf, uint32_t out_sz)
{
    int i;
    uint32_t val;
    for (i = 0; i < out_sz; i++) {
        if (get_dat_buf(buf, &val)) {
            out_buf[i] = val;
        } else {
            break;
        }
    }
    return i;
}
