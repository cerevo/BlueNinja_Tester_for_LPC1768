#include <stdlib.h>
#include <stdint.h>

/*** ���[�J���֐� ***/
static int uint32_sort(const void *a, const void *b)
{
    if (*(uint32_t*)a > *(uint32_t*)b) {
        return 1;
    } else if (*(uint32_t*)a < *(uint32_t*)b) {
        return -1;
    } else {
        return 0;
    }
}
/* �����l��Ԃ� */
uint32_t get_median(uint32_t *buf, uint8_t sz)
{
    qsort((void*)buf, sz, sizeof(uint32_t), uint32_sort);
    
    return buf[sz / 2];
}
