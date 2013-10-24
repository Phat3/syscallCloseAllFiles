/*
Ok I know, this syscall is pretty useless for our purpose but can give a concrete idea to
what we are going to do.
In our project we don't need to changing the behavoir of a syscall, but we have to implementing
a NEW LINE to the sys_call_table, so a kernel's recompiling is necessary.

Next hop: write a new line to the sys_call_table and test it.


usage of this module:

(after compiled with the classic argument -Wall ecc... ) 

insmod mysys2.ko uid = < number of id to spy >

and 

rmmod mysys2.ko

You can watch the spying printk with the dmesg command. 
*/


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/unistd.h>
#include <linux/syscalls.h> // necessary for the symbol sys_close
#include <linux/sched.h> // for struct task_struct
#include <asm/uaccess.h>
#include <asm/thread_info.h>
#include <linux/cred.h>  //necessary for achieve the structure that store the credential pf the process

/*
my pointer to the table sys_call_table
*/
unsigned long **sys_call_table;
 
/*
a pointer to a function.
I'm going to use it as a store of the real address of the OPEN
*/
asmlinkage long (*original_call) (const char *,int,int);

/*
Necessary for receive parameters from command line
*/
static int uid;
module_param(uid, int, 0644); //<-- the uid, type, permission
MODULE_PARM_DESC(uid , "User id to spy");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fabio Gritti, Sebastiano Mariani, Alessandro Loregiola");

/*
Function used to aquire the address of the sys_call_table since it is not long exported as
a symbol 
*/
static unsigned long **aquire_sys_call_table(void)
{
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;

        /*We're moving row by row in the sys_call_table*/
	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;

                /* Obscure point: why is correct to return the address when this if is verified? */
		if (sct[__NR_close] == (unsigned long *) sys_close) 
			return sct;
                
                /*
                  each element in the table is 32 bit ( on x86-32 )
                  so we are increasing the address in our offset by 4bytes
                  ( in fact sizeof(void*) returns 4 ) 
                */
		offset += sizeof(void *); 
	}
        
        printk(KERN_ALERT "Failed to load the sys_call_table");
	return NULL;
}

/* our piece of code first the real calling of the open */
asmlinkage int our_sys_open(const char *filename , int flags , int mode )
{
 	int i = 0;
	char ch;
	/* 
	 * Check if this is the user we're spying on
         * the attribute uid has been moved to a structure cred in linux/cred.h, so I
         * take this from current(the current task_struct)->cred(pointer to the cred structure)->uid 
	 */
	if (uid == current->cred->uid) {
		/* 
                 * Oh, a process of our user! 
		 * Report the file! ( Sounds like United spies of America! )
		 */
		printk("Opened file by %d: ", uid);
		do {
			get_user(ch, filename + i);
			i++;
			printk("%c", ch); // printing the file path
		} while (ch != 0);
		printk("\n");
	}
 /* Then jump to the real address of the open function! ( stored in the init of this module ) */
 return original_call(filename,flags,mode); 
}

/*

Ok , this sounds bit aggressive, but it is necessary because the memory where kernel is living
is a read only memory, this implies we can't write anything in the syscalltable.

This is a trick to bypass this limitation, we disable via hardware the control of the read-only 
pages by changing the 16 bit of processor that control this behavoir. 
Some asm volatile, think this must be change for ARM's processor( bad,bad notice... ) 

Of course enable roolbacks the situation to normality after our changing and disable permit our
modification by ( what a news! ) disabling this control.
*/
static void enable_page_protection(void) 
{
	unsigned long value;
	asm volatile("mov %%cr0, %0" : "=r" (value));

	if((value & 0x00010000))
		return;

	asm volatile("mov %0, %%cr0" : : "r" (value | 0x00010000));
}

static void disable_page_protection(void) 
{
	unsigned long value;
	asm volatile("mov %%cr0, %0" : "=r" (value));

	if(!(value & 0x00010000))
		return;

	asm volatile("mov %0, %%cr0" : : "r" (value & ~0x00010000));
}


/*
When module is insmoded we aquire the syscalltable and perform these operations:
1. store the real address of the open in the original_call
2. change the row at __NR_open with our_sys_open
3. re-enabling page protection!
*/
static int __init startup_init(void)
{
printk (KERN_ALERT "spy_module inserted correctly!");
if(!(sys_call_table = aquire_sys_call_table()))
   return -1;

	disable_page_protection();
	original_call = (void *)sys_call_table[__NR_open];
	sys_call_table[__NR_open] = (unsigned long *)our_sys_open;
	enable_page_protection();

 return 0;
}

/*
When module is rmmoded we roolback the situation of the sys_call_table to normality
by re-changing the row __NR_open to the original address.
*/
static void __exit shutdown_exit(void)
{
if(!sys_call_table)
		return;

	disable_page_protection();
	sys_call_table[__NR_open] = (unsigned long *)original_call;
	enable_page_protection();
printk("Module closed");
}

/*classic */
module_init(startup_init);
module_exit(shutdown_exit);

