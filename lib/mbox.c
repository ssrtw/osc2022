#include "mbox.h"

#include "compiler.h"
#include "gpio.h"
#include "uart.h"

#define MBOX_BASE   MMIO_BASE + 0xb880
#define MBOX_READ   ((volatile uint32_t *)(MBOX_BASE + 0x00000000))
#define MBOX_STATUS ((volatile uint32_t *)(MBOX_BASE + 0x00000018))
#define MBOX_WRITE  ((volatile uint32_t *)(MBOX_BASE + 0x00000020))

// magic number
#define REQUEST_CODE     0x00000000
#define TAG_REQUEST_CODE 0x00000000
#define END_TAG          0x00000000
// states code
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000
#define REQUEST_SUCCEED 0x80000000
#define REQUEST_FAILED  0x80000001
// request
#define GET_BOARD_REVISION 0x00010002
#define GET_ARM_MEMORY     0x00010005

#define ARM2VC 8
#define VC2ARM 9

// need alignment
volatile uint32_t mbox[64] __attribute__((aligned(16)));

int mbox_call(byte ch) {
    uint32_t r = (((uint32_t)((uint64_t)mbox) & ~0xF) | (ch & 0xF));
    /* wait until we can write to the mailbox */
    WAITING(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while (1) {
        /* is there a response? */
        WAITING(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if (r == *MBOX_READ) {
            /* is it a valid successful response? */
            return mbox[1] == REQUEST_SUCCEED;
        }
    }
    return 0;
}

void get_board_revision(uint32_t *res) {
    mbox[0] = 7 * 4;         // buffer size in bytes
    mbox[1] = REQUEST_CODE;  // magic number(0)
    // tags begin
    mbox[2] = GET_BOARD_REVISION;  // tag identifier
    mbox[3] = 4;                   // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;    // magic number(0)
    mbox[5] = 0;                   // value buffer
    // tags end
    mbox[6] = END_TAG;

    mbox_call(ARM2VC);
    // it should be 0xa020d3 for rpi3 b+
    *res = mbox[5];
}

void get_arm_memory(uint32_t *addr, uint32_t *size) {
    mbox[0] = 8 * 4;         // buffer size in bytes
    mbox[1] = REQUEST_CODE;  // magic number(0)
    // tags begin
    mbox[2] = GET_ARM_MEMORY;    // tag identifier
    mbox[3] = 8;                 // maximum of request and response value buffer's length.
    mbox[4] = TAG_REQUEST_CODE;  // magic number(0)
    mbox[5] = 0;                 // value buffer
    mbox[6] = 0;
    // tags end
    mbox[7] = END_TAG;

    mbox_call(ARM2VC);
    *addr = mbox[5];
    *size = mbox[6];
}