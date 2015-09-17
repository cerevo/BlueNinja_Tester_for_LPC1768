#include <string.h>
#include "cmd_buf.h"

/* �R�}���h��M�o�b�t�@ */

//�R�}���h��M�o�b�t�@����֐�
void init_cmd_buf(CMD_BUF *buf)
{
    memset(buf->data, 0, CMD_BUF_LEN);
    buf->r_idx = 0;
    buf->w_idx = 0;
}

int put_cmd_buf(CMD_BUF *buf, uint8_t val)
{
    buf->data[buf->w_idx] = val;
    buf->w_idx = (buf->w_idx + 1) % CMD_BUF_LEN;
    if (buf->w_idx == buf->r_idx) {
        buf->r_idx = (buf->r_idx + 1) % CMD_BUF_LEN;
    }
    
    return val;
}

int get_cmd_buf(CMD_BUF *buf)
{
    uint8_t val;
    
    if (buf->w_idx == buf->r_idx) {
        return -1;  //Buffer EMPTY
    }
    val = buf->data[buf->r_idx];
    buf->r_idx = (buf->r_idx + 1) % CMD_BUF_LEN;
    
    return val;
}
//�����O�o�b�t�@��ǂ݂����Ĕz��Ɋi�[
int get_cmd_series_buf(CMD_BUF *buf, uint8_t *out_buf, uint8_t out_sz)
{
    int i;
    int val;
    for (i = 0; i < out_sz; i++) {
        val = get_cmd_buf(buf);
        if (val != -1) {
            out_buf[i] = val;
        } else {
            break;
        }
    }
    return i;
}
