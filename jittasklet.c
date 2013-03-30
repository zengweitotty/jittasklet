/*
    file Name:      jittasklet.c
	Author:         zengweitotty
    version:        V1.0   
    Data:           2013/03/11
    Email:          zengweitotty@gmail.com
    Description     using proc fs to test tasklet
*/
#include <linux/init.h>	//using for function module_init module_exit
#include <linux/module.h>	
#include <linux/kernel.h>	//using for printk
#include <linux/moduleparam.h>	//using for kernel param
#include <linux/sched.h>	//using for schedule
#include <linux/proc_fs.h>	//using for create_proc_read_entry
#include <linux/interrupt.h>	//using for struct tasklet_struct
#include <linux/slab.h>	//using for kmalloc and kfree
#include <linux/wait.h>	//using for wait
#include <linux/errno.h>	//using for error number

#define LOW 0x01
#define HIGH 0x10
#define LOOP 10

struct jit_tasklet_data{
	int loop;
	struct tasklet_struct task;
	char* buf;
	int len;
	unsigned long prevjiffies;
	wait_queue_head_t wait;
	unsigned long  mode;
};

struct jit_tasklet_data *jitTasklet;

static void tasklet_fn(unsigned long data){
	struct jit_tasklet_data *jitTasklet_temp = (struct jit_tasklet_data *)data;
	jitTasklet_temp->buf  += sprintf(jitTasklet_temp->buf, "%9li\t%3li\t%i\t%6i\t%i\t%s\n",jiffies,jiffies - jitTasklet_temp->prevjiffies,in_interrupt()?1:0,current->pid,smp_processor_id(),current->comm);
	if(--jitTasklet_temp->loop){
		printk(KERN_INFO "The seq is %d",jitTasklet_temp->loop);
		jitTasklet_temp->prevjiffies = jiffies;
		tasklet_init(&jitTasklet_temp->task,tasklet_fn,(unsigned long)jitTasklet_temp);
		if(jitTasklet_temp->mode == LOW){
			tasklet_schedule(&jitTasklet_temp->task);
		}else if(jitTasklet_temp->mode == HIGH){
		    tasklet_hi_schedule(&jitTasklet_temp->task);
		}else{
		    printk(KERN_ERR "[jittasklet/tasklet_fn] Can not support others\n");
		}														    
	}else{
	   wake_up_interruptible(&jitTasklet_temp->wait);
	}
}

static int jit_tasklet_fn(char* buf,char** start,off_t offset,int len,int *eof,void *data){
	jitTasklet->buf = buf;
	jitTasklet->loop = LOOP;
	jitTasklet->mode = (unsigned long *)data;
	jitTasklet->buf += sprintf(jitTasklet->buf,"timer\tdelta\tinirq\tpid\tcpu\tcommand\n");
	tasklet_init(&jitTasklet->task,tasklet_fn,(unsigned long *)jitTasklet);
	jitTasklet->prevjiffies = jiffies;
	if(jitTasklet->mode == LOW){
		tasklet_schedule(&jitTasklet->task);
	}else if(jitTasklet->mode == HIGH){
		tasklet_hi_schedule(&jitTasklet->task);
	}else{
		printk(KERN_ERR "[jittasklet/jit_tasklet_fn] Can not support others\n");
	}
	wait_event_interruptible(jitTasklet->wait,!jitTasklet->loop);
	if(signal_pending(current))
		return -ERESTARTSYS;
	tasklet_kill(&jitTasklet->task);
	*start = buf;
	jitTasklet->len = jitTasklet->buf - buf;
	return jitTasklet->len;
}

static int __init jit_tasklet_init(void){
	jitTasklet = kmalloc(sizeof(struct jit_tasklet_data),GFP_KERNEL);
	if(!jitTasklet){
		printk(KERN_ERR "[jittasklet/jit_tasklet_init] Can not malloc memory\n");		
		return -ENOMEM;
	}
	memset(jitTasklet,0,sizeof(struct jit_tasklet_data));
	init_waitqueue_head(&jitTasklet->wait);
	create_proc_read_entry("jittasklet",0,NULL,jit_tasklet_fn,LOW);
	create_proc_read_entry("jittasklethi",0,NULL,jit_tasklet_fn,HIGH);
	printk(KERN_INFO "[jittasklet/jit_tasklet_init] Success to initialize jittasklet module\n ");
	return 0;
}
static void __exit jit_tasklet_exit(void){
	kfree(jitTasklet);
	remove_proc_entry("jittasklet",NULL);
	remove_proc_entry("jittasklethi",NULL);
	printk(KERN_INFO "[jittasklet/jit_tasklet_exit] Success to remove nod\n");
}

module_init(jit_tasklet_init);
module_exit(jit_tasklet_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zengweitotty");
MODULE_DESCRIPTION("tasklet test");

