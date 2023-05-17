#include <xinu.h>
#include <keyboard.h>

static void kbd_clearbuffer()
{
    kbd_state.count = 0;
    kbd_state.tail = 0;
    kbd_state.head = 0;
    semreset(kbd_state.sem_buffer, 0);
}

syscall	kbdcontrol(did32 descrp, int32 func, int32 arg1, int32 arg2)
{
    if(kbd_state.pr_guest!=getpid())    /* if not open */
        return SYSERR;

	switch (func)
    {
        case KBD_CLEAR: kbd_clearbuffer(); break;
        default: return SYSERR;
    }
    return OK;
}