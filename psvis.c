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
int x;
int y;

module_param(PID, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(PID, "input PID of the parent process");

module_param(x, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(x, "x range of the output plot");

module_param(y, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(y, "y range of the output plot");

// A function that traverses through processes and saves data on a file.
int psvis_init(void) {
  struct task_struct* parent;
  struct task_struct* child;
  struct list_head* next_child;
  FILE* data = fopen("data.txt", "w");
  fprintf(data, "PID\tParent\tLabel\n");
  parent = get_pid_task(find_get_pid(PID), PIDTYPE_PID);
  u64 start_time = &parent->start_time;
  //int x_coor = x/2;
  //int y_coor = y;
  char buff[50];
  sprintf(buff, "%d\t-\tPID=%dStart time=%lld\n", PID, PID, start_time);
  fprintf(data, buff);


  list_for_each(next_child, &parent->children) {
    child = list_entry(next_child, struct task_struct, sibling);
    start_time = &child->start_time;
    pid_t pid = &child->pid;
    char buff_c[50];
    sprintf(buff_c, "%d\t-\tPID=%dStart time=%lld\n", pid, pid, start_time);
    fprintf(data, buff_c);
    psvis_recursive(child, data);
  }
  fclose(data);
  return 0;
}

//Recursively saving information about nodes.
void psvis_recursive(struct task_struct* process, FILE* data) {

  struct task_struct* child;
  struct list_head* next_child;

  list_for_each(next_child, &process->children) {
    child = list_entry(next_child, struct task_struct, sibling);
    start_time = &child->start_time;
    pid_t pid = &child->pid;
    char buff_c[50];
    sprintf(buff_c, "%d\t-\tPID=%dStart time=%lld\n", pid, pid, start_time);
    fprintf(data, buff_c);
    psvis_recursive(child, data);
  }
}

// A function that exits the kernel.
void psvis_exit(void) {}

module_init(psvis_init);
module_exit(psvis_exit);
