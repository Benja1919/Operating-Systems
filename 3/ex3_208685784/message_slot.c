/*********************************************Defines*****************************************************/
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE
/*********************************************************************************************************/

/*********************************************Includes****************************************************/

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "message_slot.h"

/*********************************************************************************************************/
MODULE_LICENSE("GPL");

/*********************************Some Explainations about the code***************************************/
/*
                                        id 208685784
				In this assignment we are implementing kernel module called "msg_slot". As described in 
				the assignment guides - A message slot device has multiple message channels active
				concurrently, which can be used by multiple processes. The Device's functions and setup
				skeleton are implemented in this "message_slot.c" file as seen in the recitation of week 6.
				Moreover, the process uses ioctl() to specify the id of the message channel it wants to use.
                In this assignment we were given the option to choose the Data-Structure for kernel purpuses
                so we used linked-list to gain the capability to obtain this method. Also, as requested 
                in the assingment - we needed to preserve space complexity - so i decided to use this DS.
                Structure used in this file are given in the "message.slot.h" file.
								                                                                        */
/********************************************************************************************************/

/*********************************Device Functions declarations******************************************/
static int device_open(struct inode* inode, struct file* file);
static long device_ioctl( struct file* file, unsigned int ioctl_command_id, unsigned long channel);
static int device_release( struct inode* inode, struct file*  file);
static ssize_t device_read( struct file* file, char __user* buffer, size_t length, loff_t* offset);
static ssize_t device_write( struct file* file, const char __user* buffer, size_t length, loff_t* offset);
/*******************************************************************************************************/

/*********************************Device setup declarations*********************************************/
static int __init initialization_process(void);
static void __exit freeing_process(void);
/*******************************************************************************************************/
/*********************************************Structures**********************************************/
/*
                    these structures are used in "message_slot.c" as mentioned above.
                    they will use as in order to gain the capability of linked-list.
                    these structures are static because we will use them in this file 
                    (which is why they are not included in "message_slot.h" because
                    aren't relevant for "message_reader.c" and "message_sender.c"
								                                                                     */
/*****************************************************************************************************/

/*will use us as slots array*/
static struct linked_list_slots **quantity; 

static struct nodes_via_channel *chan_recieved(struct linked_list_slots* slot, unsigned long id) {
/*linked list by the previous structs*/
    struct nodes_via_channel *curr = slot->head_pt;
    while (curr != NULL)
    {
        if (curr->id == id)
        {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}
/*******************************************************************************************************/


//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode* inode, struct file* file) {
    unsigned int minor = iminor(inode); 
    if (quantity[minor] == NULL)
    { 
        struct linked_list_slots *current_slot = (struct linked_list_slots *) kmalloc(sizeof(struct linked_list_slots), GFP_KERNEL);
        if (current_slot == NULL) {
            /*raise error if alloc failed*/
            return -ENOMEM;
        }
        memset(current_slot, 0, sizeof(struct linked_list_slots));
        current_slot->minor_num = minor;
        quantity[minor] = current_slot;
    }
    return SUCCESS;
}
//----------------------------------------------------------------


static ssize_t device_read(struct file* file, char __user* buffer, size_t buffer_length, loff_t* offset) {
    struct linked_list_slots* curr;
    struct nodes_via_channel *curr_chan;
    int value;
    int index = 0;
    int dlvr_len;
    unsigned long id;

    if (file->private_data == NULL) {
    /*as described in the description in case of no channel has been set on the file descriptor*/
        return -EINVAL;
    }
    curr = quantity[iminor(file->f_inode)];
    id = (long)(file->private_data);
    curr_chan = chan_recieved(curr, id);
    if (curr_chan == NULL) 
    {
        return -EINVAL;
    }
    if (curr_chan->dlvr == NULL) 
    {
    /*as described in the description in case no message exists on the channel*/
        return -EWOULDBLOCK;
    }
    dlvr_len = curr_chan->dlvr_len;
    if (buffer_length < dlvr_len) 
    {
    /*as described in the description in case If the provided buffer length is too small
     to hold the last message written on the channel*/
        return -ENOSPC;
    }
    while (index < dlvr_len && index < BUFFER_SIZE) 
    {
        value = put_user(curr_chan->dlvr[index], &buffer[index]);
        if (value != 0) 
        {
        /*raise error*/
            return -EFAULT;
        }
        index++;
    }
    return curr_chan->dlvr_len;
}
//----------------------------------------------------------------


static ssize_t device_write(struct file* file, const char __user* buffer, size_t dlvr_len, loff_t* offset) {
    struct linked_list_slots* curr;
    struct nodes_via_channel *curr_chan;
    int value;
    int index = 0;
    unsigned long id;

    if (file->private_data == NULL) 
    {
    /*as described in the description in case of no channel has been set on the file descriptor*/
        return -EINVAL;
    }
    if (dlvr_len == 0 || dlvr_len > BUFFER_SIZE) 
    {
    /*as described in the description in case the passed message length is 0 or more than BUFFER_SIZE*/
        return -EMSGSIZE;
    }

    curr = quantity[iminor(file->f_inode)];
    id = (long)(file->private_data);
    curr_chan = chan_recieved(curr, id);

    if (curr_chan == NULL) 
    {
        /*raise error*/
        return -EINVAL;
    }

    curr_chan->dlvr = (char *) kmalloc(dlvr_len, GFP_KERNEL);
    if (curr_chan->dlvr == NULL) 
    {
        /*raise error*/
        return -ENOMEM;
    }
    memset(curr_chan->dlvr, 0, dlvr_len);
    curr_chan->dlvr_len = dlvr_len;
    while(index < dlvr_len)
    {
        value = get_user(curr_chan->dlvr[index], &buffer[index]);
        if (value != 0) 
        {
        /*raise error*/
            return -EFAULT;
        }
        index++;
    }
    return dlvr_len;
}
//----------------------------------------------------------------


static long device_ioctl(struct file* file, unsigned int ioctl_command, unsigned long id) {
    struct nodes_via_channel* curr_chan;
    struct linked_list_slots* curr;

    if (ioctl_command == MSG_SLOT_CHANNEL && id != 0) 
    {
        file->private_data = (void *) id;
        curr = quantity[iminor(file->f_inode)];

        curr_chan = chan_recieved(curr, id);
        if (curr_chan == NULL)
        { 
            curr_chan = (struct nodes_via_channel*) kmalloc(sizeof(struct nodes_via_channel), GFP_KERNEL);
            if (curr_chan == NULL) 
            {
                /*raise error*/
                return -ENOMEM;
            }
            memset(curr_chan, 0, sizeof(struct nodes_via_channel));
            curr_chan->id = id;


            /*linked list procedures*/
            if (curr->head_pt == NULL) 
            { 
                curr->head_pt = curr_chan;
                curr->tail_pt = curr_chan;
            }
            else 
            { 
                curr->tail_pt->next = curr_chan;
                curr->tail_pt = curr_chan;
            }
        }
        return SUCCESS;
    }
    else
    {   
    /*as described in the description in case of ioctl returns -1*/
        return -EINVAL;
    }
}
//----------------------------------------------------------------


static int device_release(struct inode* inode, struct file* file) 
{
    return SUCCESS;
}
//----------------------------------------------------------------

//==================== DEVICE SETUP =============================
struct file_operations Fops =
{
    .owner	        = THIS_MODULE, 
    .open           = device_open,
    .read           = device_read,
    .write          = device_write,
    .unlocked_ioctl = device_ioctl,
    .release        = device_release,
};

static int initialization_process(void) 
{
    /*as seen in recitation 6 files*/
    int num;

    num = register_chrdev(MAJOR_NUM, DEVICE, &Fops);

    if (num < 0)
    {
        /*print error in kernel*/
        printk(KERN_ERR "%s registraion failed for %d\n", DEVICE, num);
        return num;
    }

    quantity = (struct linked_list_slots **) kmalloc(SLOTS * sizeof(struct linked_list_slots *), GFP_KERNEL);
    if (quantity == NULL) 
    {
        /*raise error*/
        return -ENOMEM;
    }
    memset(quantity, 0, SLOTS * sizeof(struct linked_list_slots *));

    return SUCCESS;
}
//---------------------------------------------------------------


static void freeing_process(void) {
    /*as seen in recitation 6 files*/
    struct linked_list_slots* curr; 
    struct nodes_via_channel *chan_curr, *node;
    /*node will use as as temporary*/
    int index = 0;
    while(index < SLOTS) 
    {
        curr = quantity[index];
        if (curr != NULL)
        {
            chan_curr = curr->head_pt;
            while (chan_curr != NULL)
            {
                /*complete as linked list*/
                node = chan_curr;
                chan_curr = chan_curr->next;
                if (node->dlvr != NULL) 
                {
                    /*free val*/
                    kfree(node->dlvr); 
                }
                /*free node*/
                kfree(node); 
                /*free quant*/
            }
            kfree(quantity[index]); 
        }
        index++;
    }
    kfree(quantity); 
    unregister_chrdev(MAJOR_NUM, DEVICE);
}
//---------------------------------------------------------------
module_init(initialization_process);
module_exit(freeing_process);
//========================= END OF FILE =========================
