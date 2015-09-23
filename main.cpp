#include "mbed.h"
#include <ctype.h>
#include "cmd_buf.h"
#include "dat_buf.h"
#include "median.h"

Serial uart_pc(P0_2, P0_3);
Serial uart_tz(P0_25, P0_26);

DigitalOut led1(P0_1);
DigitalOut led2(P0_10);
DigitalOut led3(P0_11);

DigitalOut PowerSW_EN(P2_12);
DigitalOut ResetSW_EN(P0_8);
DigitalOut TZ_ELASE(P0_9);
DigitalOut ADC24_SYNC(P2_5);
DigitalOut ADV_EN(P2_11);
DigitalOut CHG_EN(P1_10);
DigitalOut USB_EN(P1_9);

DigitalOut GPIO_7(P2_0);
DigitalOut GPIO_8(P2_1);
DigitalOut GPIO_9(P2_2);
DigitalOut GPIO_16(P0_7);
DigitalOut GPIO_17(P0_6);
DigitalOut GPIO_18(P0_5);
DigitalOut GPIO_19(P0_4);
DigitalOut GPIO_20(P2_6);
DigitalOut GPIO_21(P2_7);
DigitalOut GPIO_22(P2_8);
DigitalOut GPIO_23(P2_9);

AnalogIn ADC_DIV2_VSYS(P0_23);
AnalogIn ADC_DIV2_D3V3(P0_24);
AnalogIn ADC_DIV2_BAT(P1_30);
AnalogIn ADC_DIV2_5V(P1_31);

DigitalIn OVER_CURR(P1_14);  //(仮)

I2CSlave I2CS_PINGPONG(P0_27, P0_28);

Timer TimAdcInterval;

/** 型の定義 **/
/* イベント */
typedef enum {
    EVT_NONE,
    EVT_CMD_P,      //電源Enableコマンド受信
    EVT_CMD_R,      //ブレイクアウトボードリセットコマンド受信
    EVT_CMD_E,      //ファームウェアイレースコマンド受信
    EVT_CMD_O,      //DOコマンド受信
    EVT_CMD_A,      //ADC Enableコマンド受信
    EVT_CMD_V,      //T電源電圧測定コマンド受信
    EVT_CMD_U,      //USB VBUSコマンド受信
    EVT_CMD_B,      //バッテリー有効化コマンド受信
    EVT_CMD_C,      //電流チェックコマンド受信
    EVT_CMD_RELAY,  //コマンドをTZ1へ中継
}   CMD_EVT;

/** 変数宣言 **/
static CMD_BUF recv_buff;   //受信文字列バッファ(リングバッファ)
static uint8_t cmd_buf[5];  //受信コマンドバッファ
static DAT_BUF adc_TZ_VSYS_buff;
static DAT_BUF adc_TZ_D3V3_buff;
static DAT_BUF adc_TZ_BATT_buff;

/** 各コマンドの処理 **/
/* P000/P001: PowerSW_ENを設定する */
static void cmd_Pxxx(int xxx)
{
    PowerSW_EN = xxx;
}

/* R000/R001: ResetSW_ENを設定する */
static void cmd_Rxxx(int xxx)
{
    ResetSW_EN = xxx;
}

/* E000/E001: TZ_ELASEを設定する */
static void cmd_Exxx(int xxx)
{
    TZ_ELASE = xxx;
}

/* OLxx: GPIOをLoに設定/OHxx: GPIOをHiに設定 */
static void cmd_Ooxx(char o, int xx)
{
    int hi_lo;
    
    // Lo/Hi
    switch (o) {
        case 'L':
            hi_lo = 0;
            break;
        case 'H':
            hi_lo = 1;
            break;
        default:
            return;
    }
    
    // DOの選択
    switch (xx) {
        case 7:
            GPIO_7 = hi_lo;
            break;
        case 8:
            GPIO_8 = hi_lo;
            break;
        case 9:
            GPIO_9 = hi_lo;
            break;
        case 16:
            GPIO_16 = hi_lo;
            break;
        case 17:
            GPIO_17 = hi_lo;
            break;
        case 18:
            GPIO_18 = hi_lo;
            break;
        case 19:
            GPIO_19 = hi_lo;
            break;
        case 20:
            GPIO_20 = hi_lo;
            break;
        case 21:
            GPIO_21 = hi_lo;
            break;
        case 22:
            GPIO_22 = hi_lo;
            break;
        case 23:
            GPIO_23 = hi_lo;
            break;
    }
}

/* Vxxx: 電源電圧測定 */
static int cmd_Vxxx(int xxx)
{
    uint32_t val[5];
    switch (xxx) {
        case 1: //TZ_VSYS
            if (get_series_dat_buf(&adc_TZ_VSYS_buff, val, 5) != 5) {
                return -1;  //バッファに5件溜まってない
            }
            break;
        case 2: //TZ_D3V3
            if (get_series_dat_buf(&adc_TZ_D3V3_buff, val, 5) != 5) {
                return -1;  //バッファに5件溜まってない
            }
            break;
        case 3: //TZ_BATT
            if (get_series_dat_buf(&adc_TZ_BATT_buff, val, 5) != 5) {
                return -1;  //バッファに5件溜まってない
            }
            break;
    }   
    return get_median(val, 5);
}


/* U000/U001: USB VBUS ON/OFF*/
static void cmd_Uxxx(int xxx)
{
    USB_EN = xxx;
}

/* A000/A001: ADV_ENを設定する */
static void cmd_Axxx(int xxx)
{
    ADV_EN = xxx;
}

/* B000/B001: CHG_ENを設定する */
static void cmd_Bxxx(int xxx)
{
    CHG_EN = xxx;
}

/* C000: OVER_CURRENTを読む */
static int cmd_C000(void)
{
    return OVER_CURR;
}

/** コマンド受信 **/
static bool recvCommand(CMD_BUF *buff, CMD_EVT *evt, int *param1, char *param2)
{
    int c;
    
    if (param1 == NULL) {
        return false;
    }
    
    *evt = EVT_NONE;
    if (uart_pc.readable()) {
        c = uart_pc.getc();
        /* コマンド終端 */
        if (c == '\r') {
            memset(cmd_buf, 0, sizeof(cmd_buf));
            if (get_cmd_series_buf(buff, cmd_buf, 4) != 4) {
                return -1;
            }
            
            //コマンド判定
            switch (cmd_buf[0]) {
                case 'P':
                    *evt = EVT_CMD_P;
                    *param1 = strtol((char *)&cmd_buf[1], NULL, 10);
                    if ((*param1 != 0) && (*param1 != 1)) {
                        return false;
                    }
                    break;
                case 'R':
                    *evt = EVT_CMD_R;
                    *param1 = strtol((char *)&cmd_buf[1], NULL, 10);
                    if ((*param1 != 0) && (*param1 != 1)) {
                        return false;
                    }
                    break;
                case 'E':
                    *evt = EVT_CMD_E;
                    *param1 = strtol((char *)&cmd_buf[1], NULL, 10);
                    if ((*param1 != 0) && (*param1 != 1)) {
                        return false;
                    }
                    break;
                case 'O':
                    *evt = EVT_CMD_O;
                    *param1 = strtol((char *)&cmd_buf[2], NULL, 10);
                    switch (*param1)  {
                        case 7:
                        case 8:
                        case 9:
                        case 16:
                        case 17:
                        case 18:
                        case 19:
                        case 20:
                        case 21:
                        case 22:
                        case 23:
                            break;
                        default:
                            return false;
                    }
                    *param2 = cmd_buf[1];
                    if ((*param2 != 'L') && (*param2 != 'H')) {
                        //invalid parameter
                        return false;
                    }
                    break;
                case 'A':
                    *evt = EVT_CMD_A;
                    *param1 = strtol((char *)&cmd_buf[1], NULL, 10);
                    if ((*param1 != 0) && (*param1 != 1)) {
                        return false;
                    }
                    break;
                case 'V':
                    *evt = EVT_CMD_V;
                    *param1 = strtol((char *)&cmd_buf[1], NULL, 10);
                    if ((*param1 < 1) || (*param1 > 3)) {
                        return false;
                    }
                    break;
                case 'U':
                    *evt = EVT_CMD_U;
                    *param1 = strtol((char *)&cmd_buf[1], NULL, 10);
                    if ((*param1 != 0) && (*param1 != 1)) {
                        return false;
                    }
                    break;
                case 'B':
                    *evt = EVT_CMD_B;
                    *param1 = strtol((char *)&cmd_buf[1], NULL, 10);
                    if ((*param1 != 0) && (*param1 != 1)) {
                        return false;
                    }
                    break;
                case 'C':
                    *evt = EVT_CMD_C;
                    *param1 = strtol((char *)&cmd_buf[1], NULL, 10);
                    if (*param1 != 0) {
                        return false;
                    }
                    break;
                default:
                    *evt = EVT_CMD_RELAY;
                    break;
            }
        } else {
            /* 受信バッファへ追加 */
            put_cmd_buf(buff, c);
        }
    }
    return true;    
}

/*** メイン ***/
static const int ADC_INTERVAL = 100;
int main() {
    CMD_EVT evt;
    int  param1;
    char param2;
    int  ret;
    
    char pingpong_buf[10];
    
    uart_pc.baud(9600);
    uart_pc.printf("*** TZ1 TESTER ***\r\n");
    
    uart_tz.baud(9600);
    
    led1 = 0;
    led2 = 1;
    led3 = 1;
    
    I2CS_PINGPONG.address(0x90);
    
    TimAdcInterval.start();
    
    while(1) {
        /* コマンド受信 */
        if (recvCommand(&recv_buff, &evt, &param1, &param2)) {
            switch (evt) {
                case EVT_CMD_P:
                    cmd_Pxxx(param1);
                    uart_pc.printf("DONE\r\n");
                    break;
                case EVT_CMD_R:
                    cmd_Rxxx(param1);
                    uart_pc.printf("DONE\r\n");
                    break;
                case EVT_CMD_E:
                    cmd_Exxx(param1);
                    uart_pc.printf("DONE\r\n");
                    break;
                case EVT_CMD_O:
                    cmd_Ooxx(param2, param1);
                    uart_pc.printf("DONE\r\n");
                    break;
                case EVT_CMD_A:
                    cmd_Axxx(param1);
                    uart_pc.printf("DONE\r\n");
                    break;
                case EVT_CMD_V:
                    ret = cmd_Vxxx(param1);
                    if (ret != -1) {
                        uart_pc.printf("{\"cmd\":\"V%03d\",\"volt\":%d}\r\n", param1, ret);
                    }
                    break;
                case EVT_CMD_U:
                    cmd_Uxxx(param1);
                    uart_pc.printf("DONE\r\n");
                    break;
                case EVT_CMD_B:
                    cmd_Bxxx(param1);
                    uart_pc.printf("DONE\r\n");
                    break;
                case EVT_CMD_C:
                    ret = cmd_C000();
                    uart_pc.printf("{\"cmd\":\"C000\",\"current\":%d}\r\n", ret);
                    break;
                case EVT_CMD_RELAY:
                    //TZ1へコマンドをリレー
                    uart_tz.printf("%s\r", (char *)cmd_buf);
                    break;
                default:
                    break;
            }
        } else {
            uart_pc.printf("NG\r\n");
        }
        
        /* ADC計測 */
        if (TimAdcInterval.read_ms() >= ADC_INTERVAL) {
            TimAdcInterval.start();
            put_dat_buf(&adc_TZ_VSYS_buff, (uint32_t)ADC_DIV2_VSYS.read_u16() >> 4);
            put_dat_buf(&adc_TZ_D3V3_buff, (uint32_t)ADC_DIV2_D3V3.read_u16() >> 4);
            put_dat_buf(&adc_TZ_BATT_buff, (uint32_t)ADC_DIV2_BAT.read_u16() >> 4);
        }
        
        /* I2C PingPong */
        switch (I2CS_PINGPONG.receive()) {
            case I2CSlave::ReadAddressed:
                I2CS_PINGPONG.write("PONG", 4);
                break;
            case I2CSlave::WriteAddressed:
                I2CS_PINGPONG.read(pingpong_buf, sizeof(pingpong_buf));
                break;
        }
        
        /* TZ1からの受信内容をPCへリレー */
        if (uart_tz.readable()) {
            uart_pc.putc(uart_tz.getc());
        }
    }
}
