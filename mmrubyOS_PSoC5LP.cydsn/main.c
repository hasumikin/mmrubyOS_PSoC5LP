/*****************************************************************************
* File Name: main.c
*
* Version: 1.10
*
* Description: 
*  This code example project demonstrates how to communicate between 
*  the PC and UART component in Full duplex mode. The UART has receiver(RX) and 
*  transmitter(TX) part. The data received by RX is looped back to the TX.
*
* Related Document: 
* CE210741_UART_Full_Duplex_and_printf_Support_with_PSoC_3_4_5LP.pdf
*
* Hardware Dependency: See 
* CE210741_UART_Full_Duplex_and_printf_Support_with_PSoC_3_4_5LP.pdf
*
*******************************************************************************
* Copyright (2017), Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* (“Software”), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries (“Cypress”) and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software (“EULA”).
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress’s integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, 
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED 
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress 
* reserves the right to make changes to the Software without notice. Cypress 
* does not assume any liability arising out of the application or use of the 
* Software or any product or circuit described in the Software. Cypress does 
* not authorize its products for use in any products where a malfunction or 
* failure of the Cypress product may reasonably be expected to result in 
* significant property damage, injury or death (“High Risk Product”). By 
* including Cypress’s product in a High Risk Product, the manufacturer of such 
* system or application assumes all risk of such use and in doing so agrees to 
* indemnify Cypress against all liability. 
*******************************************************************************/

#include <project.h>
#include <stdio.h>
#include "project_common.h"

#include "mrubyc/src/mrubyc.h"
#include "mmrbc.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "scope.h"
#include "stream.h"

/* Add an explicit reference to the floating point printf library to allow
the usage of floating point conversion specifier */
#if defined (__GNUC__)
    asm (".global _printf_float");
#endif

uint8 errorStatus = 0u;


/*******************************************************************************
* Function Name: RxIsr
********************************************************************************
*
* Summary:
*  Interrupt Service Routine for RX portion of the UART
*
* Parameters:
*  None.
*
* Return:
*  None.
*
*******************************************************************************/
CY_ISR(RxIsr)
{
  uint8 rxStatus;
  uint8 rxData;
  do
  {
    /* Read receiver status register */
    rxStatus = UART_RXSTATUS_REG;
    if((rxStatus & (UART_RX_STS_BREAK      | UART_RX_STS_PAR_ERROR |
                    UART_RX_STS_STOP_ERROR | UART_RX_STS_OVERRUN)) != 0u)
    {
      /* ERROR handling. */
      errorStatus |= rxStatus & ( UART_RX_STS_BREAK      | UART_RX_STS_PAR_ERROR |
                                  UART_RX_STS_STOP_ERROR | UART_RX_STS_OVERRUN);
    }
    if((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u)
    {
      /* Read data from the RX data register */
      rxData = UART_RXDATA_REG;
      if(errorStatus == 0u)
      {
        switch (rxData) {
          case (0x08): /* '\b' backspace */
            UART_TXDATA_REG = rxData;
            UART_TXDATA_REG = ' ';
            UART_TXDATA_REG = rxData;
            break;
          case (0x09): /* '\t' horizontal tab */
            break;     /* ignore */
          default:
            /* Send data backward */
            UART_TXDATA_REG = rxData;
        }
      }
    }
  }while((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u);
}

/*
 * _fd is dummy
 */
int hal_write(int _fd, const void *buf, int nbytes)
{
  UART_PutArray( buf, nbytes );
  return nbytes;
}

void run(uint8_t *mrb)
{
  init_static();
  struct VM *vm = mrbc_vm_open(NULL);
  if( vm == 0 ) {
    FATALP("Error: Can't open VM.");
    return;
  }
  if( mrbc_load_mrb(vm, mrb) != 0 ) {
    FATALP("Error: Illegal bytecode.");
    return;
  }
  mrbc_vm_begin(vm);
  mrbc_vm_run(vm);
  find_class_by_object(vm, vm->current_regs);
  mrbc_value ret = mrbc_send(vm, vm->current_regs, 0, vm->current_regs, "inspect", 0);
  hal_write(1, "=> ", 3);
  hal_write(1, ret.string->data, ret.string->size);
  hal_write(1, "\n", 1);
  mrbc_vm_end(vm);
  mrbc_vm_close(vm);
}

#define HEAP_SIZE (1024 * 40 - 1)
static uint8_t heap[HEAP_SIZE];

/*******************************************************************************
* Function Name: main()
********************************************************************************
* Summary:
*  Main function for the project.
*
* Theory:
*  The function starts UART and interrupt components.
*
*******************************************************************************/
int main()
{
  uint8 button = 0u;
  uint8 buttonPre = 0u;
  LED_Write(LED_OFF);     /* Clear LED */
  UART_Start();           /* Start communication component */

#if(INTERRUPT_CODE_ENABLED == ENABLED)
  isr_rx_StartEx(RxIsr);
#endif /* INTERRUPT_CODE_ENABLED == ENABLED */

#if(UART_PRINTF_ENABLED == ENABLED)
  printf("\r\nprintf() is enabled\r\n\r\n");
#endif

  CyGlobalIntEnable;      /* Enable global interrupts. */

  UART_PutString("Welcome to mmrubyOS!\r\n\r\n");
  UART_PutString("% puts 'hello world!'\r\n");

  mrbc_init(heap, HEAP_SIZE);

  static Scope *scope;
  scope = Scope_new(NULL);
  StreamInterface *si = StreamInterface_new("puts 'hello world!'", STREAM_TYPE_MEMORY);
  if (Compile(scope, si)) {
    run(scope->vm_code);
    UART_PutString("% ");
//    SET_TRUE_RETURN();
  } else {
//    SET_FALSE_RETURN();
  }
  StreamInterface_free(si);

  for(;;)
  {
    if(errorStatus != 0u)
    {
      /* Indicate an error on the LED */
      LED_Write(LED_ON);
      /* Clear error status */
      errorStatus = 0u;
    }

    /***********************************************************************
    * Handle SW2 press. 
    ***********************************************************************/
    button = SW2_Read();
    if((button == 0u) && (buttonPre != 0u))
    {
      LED_Write(LED_OFF);     /* Clear LED */
    }
    buttonPre = button;
  }
}

/* [] END OF FILE */
