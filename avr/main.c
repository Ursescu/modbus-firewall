/* ----------------------- AVR includes -------------------------------------*/
#include "avr/io.h"
#include "avr/interrupt.h"
#define F_CPU 16000000
#include "avr/delay.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_f.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define REG_INPUT_START 1000
#define REG_INPUT_NREGS 4

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

    eStatus = eMBFirewallInit( MB_RTU, 0, 115200, MB_PAR_NONE, 1, 115200, MB_PAR_NONE, firewall_rule);

    sei(  );

    /* Enable the Modbus Protocol Stack. */
    eStatus = eMBFirewallEnable(  );
    DDRD |= _BV(DD7);
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
