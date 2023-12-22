#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>
#include <stddef.h>

/*********************************Some Explainations about the code************************************/
/*
                                         id 208685784
                         Explenations are descried in "message_slot.c" file
								                                                                      */
/******************************************************************************************************/

/*********************************************Defines**************************************************/

// The major device number
#define MAJOR_NUM 235
#define DEVICE "message_slot"
#define SLOTS 256
#define BUFFER_SIZE 128
#define EXIT_STATUS 1
#define EXIT_STATUS_ERR -1
#define SUCCESS 0
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

/*********************************************Structures**********************************************/
/*
                    these structures are used in "message_slot.c" as described in the
                    "Explenations about the code" given in "message_slot.c".
								                                                                     */
/*****************************************************************************************************/

struct nodes_via_channel {
/*node struct*/
    unsigned long id;
    char *dlvr;
    unsigned int dlvr_len;
    struct nodes_via_channel *next;
};

struct linked_list_slots {
/*linked list definitions by the previous struct*/
    unsigned int minor_num;
    struct nodes_via_channel *head_pt;
    struct nodes_via_channel *tail_pt;
};


#endif

