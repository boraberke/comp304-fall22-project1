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

int PID;
void psvis_recursive(struct task_struct* process);

module_param(PID, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(PID, "input PID of the parent process");

// A function that traverses through processes and saves data on a file.
int psvis_init(void) {
  struct task_struct* parent;
  struct task_struct* child;
  struct list_head* next_child;

  parent = get_pid_task(find_get_pid(PID), PIDTYPE_PID);
  u64 start_time = parent->start_time;
  char buff[50];
  sprintf(buff, "%d\t-\tPID=%d Start time=%lld\n", PID, PID, start_time);
  printk("%s", buff);

  list_for_each(next_child, &parent->children) {
    child = list_entry(next_child, struct task_struct, sibling);
    start_time = child->start_time;
    pid_t pid_c = child->pid;
    char buff_c[50];
    sprintf(buff_c, "%d\t%d\tPID=%dStart time=%lld\n", pid, PID, pid, start_time);
    printk("%s", buff_c);
    psvis_recursive(child);
  }
  return 0;
}

//Recursively saving information about nodes.
void psvis_recursive(struct task_struct* process) {

  struct task_struct* child;
  struct list_head* next_child;

  list_for_each(next_child, &process->children) {
    child = list_entry(next_child, struct task_struct, sibling);
    u64 start_time = child->start_time;
    pid_t pid = child->pid;
    char buff_c[50];
    sprintf(buff_c, "%d\t%d\tPID=%dStart time=%lld\n", pid, process->pid, pid, start_time);
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
