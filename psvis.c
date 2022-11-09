#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/slab.h>

// Meta Information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("BUSRA-BORA");
MODULE_DESCRIPTION("A module that draws the tree graph of a parent process.");

int PID; //initial value so if no parameter passed, no bug occurs.
void psvis_recursive(struct task_struct* process);

module_param(PID, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(PID, "input PID of the parent process");

// A function that traverses through processes and prints data onto kernel.
int psvis_init(void) {
  struct task_struct* parent;

  parent = get_pid_task(find_get_pid(PID), PIDTYPE_PID);
  if(!parent) {
    printk("No such process exists.\n");
    return -1;
  }
  printk("Hiii\n");
  psvis_recursive(parent);
  return 0;
}

//Recursively saving information about nodes.
void psvis_recursive(struct task_struct* parent) {

  struct task_struct* child;
  struct list_head* next_child;
  u64 start_time_p = parent->start_time;
  pid_t pid_p = parent->pid;

  //Finding the oldest child to colour in a different colour.
  struct task_struct* youngest_child = parent->p_cptr;
  struct task_struct* oldest_child;
  pid_t pid_o;
  if (youngest_child) {
    oldest_child = youngest_child->p_osptr;
  }

  list_for_each(next_child, &parent->children) {
    child = list_entry(next_child, struct task_struct, sibling);
    u64 start_time_c = child->start_time;
    pid_t pid_c = child->pid;
    if (oldest_child) {
      pid_o = oldest_child->pid;
      if (pid_o == pid_c) {
        printk("\t\"PID=%d Start time=%lld\"[fillcolor=red, style=filled];\n",pid_c, start_time_c);
      }
    }
    printk("\t\"PID=%d Start time=%lld\" -- \"PID=%d Start time=%lld\";\n", pid_p, start_time_p, pid_c, start_time_c);
    psvis_recursive(child);
  }
}

// A function that exits the kernel.
void psvis_exit(void) {
  printk("byee\n");
}

module_init(psvis_init);
module_exit(psvis_exit);
