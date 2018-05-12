/*
* hzklab6-2km.c
*
*  Created on: Apr 12, 2016
*      Author: zhqk6
*/


/*
* hzklab6-1.c
*
*  Created on: Apr 8, 2016
*      Author: zhqk6
*/


#ifndef MODULE
#define MODULE
#endif

#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <rtai.h>
#include <rtai_sched.h>
#include <linux/delay.h>
#include<linux/time.h>
#include<rtai_sched.h>                  //  scheduler
#include<rtai_fifos.h>//for fifos

MODULE_LICENSE("GPL");
static RT_TASK mytask;
RTIME period;
unsigned long *PFDR,*PFDDR,*PBDR,*PBDDR,*ptr,*ptr2;
unsigned long *GPIOBIntType1,*GPIOBIntType2,*GPIOBIntEn,*GPIOBEOI,*RawIntStsB;
unsigned long *VIC2IntEnable,*VIC2SoftIntClear;


static void rt_speaker_process(int t){
  while(1){
    *PFDR = 0x00;
    rt_task_wait_period();
    *PFDR = 0x02;
    rt_task_wait_period();
  }
}

static void HW_ISR(unsigned irq_num, void *cookie){
  char buffer2[10];
  rt_disable_irq(irq_num);         //disable interrup handling
  if(*RawIntStsB & 0x10){
    buffer2[0]='@';
    buffer2[1]='A';
    rtf_put(1,buffer2,sizeof(buffer2));
    period = start_rt_timer(nano2count(100000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
    // send these things to FIFO which will be broadcast to other slaves use thread
  }
  else if(*RawIntStsB & 0x08){
    buffer2[0]='@';
    buffer2[1]='B';
    rtf_put(1,buffer2,sizeof(buffer2));
    period = start_rt_timer(nano2count(500000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
  }
  else if(*RawIntStsB & 0x04){
    buffer2[0]='@';
    buffer2[1]='C';
    rtf_put(1,buffer2,sizeof(buffer2));
    period = start_rt_timer(nano2count(750000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
  }
  else if(*RawIntStsB & 0x02){
    buffer2[0]='@';
    buffer2[1]='D';
    rtf_put(1,buffer2,sizeof(buffer2));
    period = start_rt_timer(nano2count(1000000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);

  }
  else if(*RawIntStsB & 0x01){
    buffer2[0]='@';
    buffer2[1]='E';
    rtf_put(1,buffer2,sizeof(buffer2));
    period = start_rt_timer(nano2count(2000000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
  }
  *GPIOBEOI |=0x1F;              //xxx11111   clear buttons
  rt_enable_irq(irq_num);        //enable interrup handling
}

static void SW_ISR(unsigned irq_num, void *cookie){
  char buffer[40];
  rt_disable_irq(irq_num);       //disable interrup handling
  rtf_get(0,buffer,sizeof(buffer));
  if(buffer[1]=='A'){
    period = start_rt_timer(nano2count(250000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
  }
  else if(buffer[1]=='B'){
    period = start_rt_timer(nano2count(500000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
  }
  else if(buffer[1]=='C'){
    period = start_rt_timer(nano2count(750000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
  }
  else if(buffer[1]=='D'){
    period = start_rt_timer(nano2count(1000000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
  }
  else if(buffer[1]=='E'){
    period = start_rt_timer(nano2count(2000000));
    rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
  }
  *VIC2SoftIntClear |=0x80000000;
  rt_enable_irq(irq_num);        //enable interrup handling
}

int init_module(void){
  char buffer[40];
  char buffer2[10];
  if(rtf_create(0,1*sizeof(buffer))<0){                   // create fifo
    printk("error");
  }
  if(rtf_create(1,1*sizeof(buffer2))<0){
    printk("create fifo error\n");
  }
  //create FIFOs, 0 is from usp to kernel, 1 is from kernel to usp
  rt_set_periodic_mode();
  period = start_rt_timer(nano2count(500000));
  ptr = (unsigned long*)__ioremap(0x80840000,4096,0);
  rt_task_init(&mytask,rt_speaker_process,0,256,0,0,0);
  rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
  PBDR = ptr+1;
  PBDDR = ptr+5;
  *PBDDR |= 0xE0;                 //xxx00000   inputs
  PFDR = ptr+12;
  PFDDR = ptr+13;
  *PFDDR |= 0x02;
  GPIOBIntType1 = (unsigned long*)__ioremap(0x808400AC,4096,0);
  GPIOBIntType2 = GPIOBIntType1+1;
  GPIOBEOI = GPIOBIntType1+2;
  GPIOBIntEn = GPIOBIntType1+3;
  RawIntStsB = GPIOBIntType1+5;
  *GPIOBIntEn |= 0x1F;            //xxx11111   enable interrupt
  *GPIOBIntType1 |= 0x1F;         //xxx11111   edge
  *GPIOBIntType2 |= 0x00;         //xxx00000   falling edge
  ptr2 = (unsigned long*)__ioremap(0x800C0000,4096,0);
  VIC2IntEnable = ptr2+4;
  *VIC2IntEnable |= 0x80000000;
  VIC2SoftIntClear = ptr2+7;
  rt_request_irq(59,HW_ISR,0,1);
  *GPIOBEOI |=0x1F;
  //xxx11111   clear buttons
  *GPIOBIntEn |= 0x1F;
  //xxx11111   enable interrupt(register level)
  rt_enable_irq(59);
  //enable interrupt handling
  rt_request_irq(63,SW_ISR,0,1);

  rt_enable_irq(63);
  return 0;
}

void cleanup_module(void){
  rt_task_delete(&mytask);
  rt_release_irq(59);
  rt_release_irq(63);
  rtf_destroy(0);
  rtf_destroy(1);
  stop_rt_timer();
}
