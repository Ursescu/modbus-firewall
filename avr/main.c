/* ----------------------- AVR includes -------------------------------------*/
#include "avr/io.h"
#include "avr/interrupt.h"
#include "avr/delay.h"

#include "mbfirewall.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_f.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define REG_INPUT_START 1000
#define REG_INPUT_NREGS 4

#define MB_DEV_SPEED 115200

#define MB_UART_INPUT_NUM 0
#define MB_UART_OUTPUT_NUM 1



/* ----------------------- Static variables ---------------------------------*/
static USHORT   usRegInputStart = REG_INPUT_START;
static USHORT   usRegInputBuf[REG_INPUT_NREGS];

/* ----------------------- Start implementation -----------------------------*/

/* Write the firewall function rule */
static char firewall_rule(unsigned char addr, unsigned char *frame, unsigned short len) {

    return 1;
}

int
main( void )
{
    eMBErrorCode    eStatus;

    eStatus = eMBFirewallInit( MB_RTU, MB_UART_INPUT_NUM, MB_DEV_SPEED, MB_PAR_NONE, MB_UART_OUTPUT_NUM, MB_DEV_SPEED, MB_PAR_NONE, mb_firewall_cb);

    sei(  );

    /* Enable the Modbus Protocol Stack. */
    eStatus = eMBFirewallEnable(  );
    for( ;; )
    {
        // _delay_ms(1000);
        ( void )eMBFirewallPoll(  );

    }
}

/* Keep those here to compile the functions */

eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    return MB_ENOREG;
}

eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                 eMBRegisterMode eMode )
{
    return MB_ENOREG;
}

eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,
               eMBRegisterMode eMode )
{
    return MB_ENOREG;
}

eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOREG;
}
