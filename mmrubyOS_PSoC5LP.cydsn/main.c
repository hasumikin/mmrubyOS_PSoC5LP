/*****************************************************************************
*
* Project: mmrubyOS
*
* File Name: main.c
*
* Version: 0.1.0
*
* Description:
*  This project is based on an example code of UART Full duplex.
*
* Related Document:
* CE210741_UART_Full_Duplex_and_printf_Support_with_PSoC_3_4_5LP.pdf
*
*******************************************************************************/

#include <project.h>
#include <stdio.h>
#include "project_common.h"

#include "shell.h"

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

static mrbc_tcb *tcb_shell;

#define HEAP_SIZE (1024 * 40 - 1)
static uint8_t heap[HEAP_SIZE];

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
  if (tcb_shell == NULL) return;
  mrbc_resume_task(tcb_shell);
}

/*
 * _fd is dummy
 */
int hal_write(int _fd, const void *buf, int nbytes)
{
  UART_PutArray( buf, nbytes );
  return nbytes;
}

static void
c_print(mrbc_vm *vm, mrbc_value *v, int argc)
{
  hal_write(1, GET_STRING_ARG(1), strlen((char *)GET_STRING_ARG(1)));
}

static void
c_is_fd_empty(mrbc_vm *vm, mrbc_value *v, int argc)
{
  uint8 rxStatus;
  /* Read receiver status register */
  rxStatus = UART_RXSTATUS_REG;
  if ((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u) {
    SET_FALSE_RETURN();
  } else {
    SET_TRUE_RETURN();
  }
}

static void
c_getc(mrbc_vm *vm, mrbc_value *v, int argc)
{
  uint8 rxStatus;
  /* Read receiver status register */
  rxStatus = UART_RXSTATUS_REG;
  if((rxStatus & UART_RX_STS_FIFO_NOTEMPTY) != 0u) {
    /* Read data from the RX data register */
    char str[] = { UART_RXDATA_REG, '\0' };
    mrbc_value val = mrbc_string_new(vm, str, 1);
    SET_RETURN(val);
  } else {
    SET_NIL_RETURN();
  }
}

static void
c_exit_shell(mrbc_vm *vm, mrbc_value *v, int argc)
{
  /*
   * PENDING
   *
   * you can not call q_delete_task() as it is static function in rrt0.c
   *
  q_delete_task(tcb_shell);
  */
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
  hal_write(1, "\r\n", 2);
  mrbc_vm_end(vm);
  mrbc_vm_close(vm);
}

static void
c_compile_and_run(mrbc_vm *vm, mrbc_value *v, int argc)
{
  static Scope *scope;
  scope = Scope_new(NULL);
  StreamInterface *si = StreamInterface_new((char *)GET_STRING_ARG(1), STREAM_TYPE_MEMORY);
  if (Compile(scope, si)) {
    run(scope->vm_code);
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
  StreamInterface_free(si);
}

static void
c_pid(mrbc_vm *vm, mrbc_value *v, int argc)
{
  SET_INT_RETURN(0);
}

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
  printf("\r\n\r\nStarting...\r\n\r\n");
#endif

  CyGlobalIntEnable;      /* Enable global interrupts. */

  mrbc_init(heap, HEAP_SIZE);

  mrbc_define_method(0, mrbc_class_object, "compile_and_run", c_compile_and_run);
  mrbc_define_method(0, mrbc_class_object, "fd_empty?", c_is_fd_empty);
  mrbc_define_method(0, mrbc_class_object, "print", c_print);
  mrbc_define_method(0, mrbc_class_object, "getc", c_getc);
  mrbc_define_method(0, mrbc_class_object, "pid", c_pid);
  mrbc_define_method(0, mrbc_class_object, "exit_shell", c_exit_shell);

  tcb_shell = mrbc_create_task(shell, 0);
  mrbc_run();

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
