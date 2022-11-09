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

int PID = 1; //initial value so if no parameter passed, no bug occurs.
void psvis_recursive(struct task_struct* process);

module_param(PID, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(PID, "input PID of the parent process");

// A function that traverses through processes and saves data on a file.
int psvis_init(void) {
  struct task_struct* parent;
  struct task_struct* child;
  struct list_head* next_child;

  parent = get_pid_task(find_get_pid(PID), PIDTYPE_PID);
  if(!parent) {
    printk("No such process exists.");
    return -1;
  }
  u64 start_time_p = parent->start_time;

  list_for_each(next_child, &parent->children) {
    child = list_entry(next_child, struct task_struct, sibling);
    u64 start_time_c = child->start_time;
    pid_t pid = child->pid;
    char buff_c[50];
    sprintf(buff_c, "\tPID=%dStart time=%lld -- PID=%dStart time=%lld;\n", PID, start_time_p, pid, start_time_c);
    printk("%s", buff_c);
    psvis_recursive(child);
  }
  return 0;
}

//Recursively saving information about nodes.
void psvis_recursive(struct task_struct* process) {

  struct task_struct* child;
  struct list_head* next_child;
  u64 start_time_p = process->start_time;
  pid_t PID = parent->pid;

  list_for_each(next_child, &process->children) {
    child = list_entry(next_child, struct task_struct, sibling);
    u64 start_time_c = child->start_time;
    pid_t pid = child->pid;
    char buff_c[50];
    sprintf(buff_c, "\tPID=%dStart time=%lld -- PID=%dStart time=%lld;\n", PID, start_time_p, pid, start_time_c);
    printk("%s", buff_c);
    psvis_recursive(child);
  }
}

// A function that exits the kernel.
void psvis_exit(void) {
  printk("byee");
}

module_init(psvis_init);
module_exit(psvis_exit);
