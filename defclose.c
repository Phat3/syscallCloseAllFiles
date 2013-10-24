/*

Group: Fabio Gritti , Alessandro Loregiola , Sebastiano Mariani

Here is the code of our project: writing a syscall for ARM that in a safe manner closes all the open files of a process. In this version of the program the stdin,stdout,stderror ( 0,1,2 ) are closed too ( If you want close only the file with id number >=3 initialize j=3 )

== A better documentation is provided in the official document attached in the mail == 

*/

#include <asm/uaccess.h>
#include <linux/cred.h>
#include <linux/linkage.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <linux/workqueue.h>

/* 
there are lots of library ( maybe some are useless, but the carefulness is never enough in kernel space when a syntax error means “recompile again” that cause a waste of time)
*/


/* 
for retrieving the process descriptor of the process identified by the pid ( for more information see its implementation in the kernel source ) 
*/
static struct task_struct* get_task_by_pid(pid_t pid)
{
 return pid_task(find_pid_ns(pid, task_active_pid_ns(current)), PIDTYPE_PID);
}

/* This function closes all the files of a given files_struct */
static void close_files(struct files_struct * files)
 {
         int i, j;
         struct fdtable *fdt;

        j = 0; //start from the id=0 
        rcu_read_lock();
        fdt = files_fdtable(files);  // receive the array that contains all the open files
        rcu_read_unlock();
        for (;;) {
                unsigned long set;
                i = j * BITS_PER_LONG;
                if (i >= fdt->max_fds) // reached the max_fds?
                        break;
               set = fdt->open_fds[j++]; // take the rest of the bitmap
               while (set) { 
                        if (set & 1) { //it is the current fd used? ( 1 & 1 = 1 )
                                 struct file * file = xchg(&fdt->fd[i], NULL); //logical close the file
                                 if (file) {
                                     filp_close(file, files); //concretely close the file 
                                        cond_resched(); //need scheduling?
                               }
                        }
                      i++; // moving on the next file-descriptor
                      set >>= 1; //shift on the next bit on the bitmap.
                }
        }
}


SYSCALL_DEFINE1(defclose , pid_t , pid)
{
 struct task_struct *result = NULL;
 if( pid <=1 ) //small control on negative pid and attempt to close init's files.
    return -1;
 rcu_read_lock();  // registering my read to the RCU
 result = get_task_by_pid(pid); // find the process descriptor 
 rcu_read_unlock(); 
 close_files(result->files);  //call the close_files with the files_struct of the given process
}


