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

MODULE_LICENSE("GPL");

static RT_TASK mytask;
RTIME period;
unsigned long *PFDR,*PFDDR,*PBDR,*PBDDR,*ptr;
unsigned long *GPIOBIntType1,*GPIOBIntType2,*GPIOBIntEn,*GPIOBEOI,*RawIntStsB;
//define pointers to each register we will use

static void rt_speaker_process(int t){
    while(1){
    	 *PFDR = 0x00;
    	rt_task_wait_period();
    	 *PFDR = 0x02;
    	rt_task_wait_period();
    	// create square wave
    }
}

static void HW_ISR(unsigned irq_num, void *cookie){
	rt_disable_irq(irq_num);         //disable interrup handling
	if(*RawIntStsB & 0x10){
		period = start_rt_timer(nano2count(100000));
		rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
		//the fifth button is pressed 00010000 on pin4
	}
	else if(*RawIntStsB & 0x08){
		period = start_rt_timer(nano2count(250000));
		rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
		//the fourth button is pressed 00001000 on pin3
	}
	else if(*RawIntStsB & 0x04){
		period = start_rt_timer(nano2count(750000));
		rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
		//the third button is pressed 00000100  on pin2
	}
	else if(*RawIntStsB & 0x02){
		period = start_rt_timer(nano2count(1000000));
		rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
		//the second button is pressed 00000010  on pin1
	}
	else if(*RawIntStsB & 0x01){
		period = start_rt_timer(nano2count(2000000));
		rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
		//the first button is pressed 00000001   on pin0
	}
	*GPIOBEOI |=0x1F;              //xxx11111   clear buttons
	rt_enable_irq(irq_num);        //enable interrup handling
}


int init_module(void){
	rt_set_periodic_mode();
	period = start_rt_timer(nano2count(500000));
	ptr = (unsigned long*)__ioremap(0x80840000,4096,0);
	rt_task_init(&mytask,rt_speaker_process,0,256,0,0,0);
	rt_task_make_periodic(&mytask,rt_get_time()+0*period,period);
	//make real time task periodic
	PBDR = ptr+1;
	PBDDR = ptr+5;
	*PBDDR |= 0xE0;                 //xxx00000   inputs
    PFDR = ptr+12;
    PFDDR = ptr+13;                 //the location of pbdr,pbddr,pfdr,pfddr
    *PFDDR |= 0x02;                 //xxxxx010   port F data direction register
    GPIOBIntType1 = (unsigned long*)__ioremap(0x808400AC,4096,0);
	GPIOBIntType2 = GPIOBIntType1+1;
	GPIOBEOI = GPIOBIntType1+2;
	GPIOBIntEn = GPIOBIntType1+3;
	RawIntStsB = GPIOBIntType1+5;
	//the locations of other registers we will use
	*GPIOBIntEn |= 0x1F;            //xxx11111   enable interrupt
	*GPIOBIntType1 |= 0x1F;         //xxx11111   edge
	*GPIOBIntType2 |= 0x00;         //xxx00000   falling edge
	rt_request_irq(59,HW_ISR,0,1);  //interrupt request
	*GPIOBEOI |=0x1F;
	//xxx11111   clear buttons
	*GPIOBIntEn |= 0x1F;
	//xxx11111   enable interrupt(register level)
	rt_enable_irq(59);
	//enable interrupt handling
	return 0;
}

void cleanup_module(void){
	rt_task_delete(&mytask);
	rt_release_irq(59);
    stop_rt_timer();
}
