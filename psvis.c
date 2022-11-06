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
char* image;

module_param(PID, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(PID, "input PID of the parent process");

module_param(image, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(name, "name of the output image file");

// A function that traverses through processes and draws the graph.
int psvis_init(void) {
  struct task_struct* parent;
  struct task_struct* child;
  struct list_head* next_child;

  parent = get_pid_task(find_get_pid(PID), PIDTYPE_PID);
  FILE* image_file;
  //draw
  list_for_each(next_child, &parent->children) {
    child = list_entry(next_child, struct task_struct, sibling);
    //draw
    psvis_recursive(child, image_file);
  }
  return 0;
}

//Recursively drawing nodes.
void psvis_recursive(struct task_struct* process, FILE* image_file) {

  struct task_struct* child;
  struct list_head* next_child;

  list_for_each(next_child, &process->children) {
    child = list_entry(next_child, struct task_struct, sibling);
    //draw
    psvis_recursive(child, image_file);
  }
}

// A function that exits the kernel.
void psvis_exit(void) {
  //printk("Goodbye from the kernel, user: %s, age: %d\n", name, age);
}

module_init(psvis_init);
module_exit(psvis_exit);
