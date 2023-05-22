/* kbdread.c  -  kbdread */

#include <xinu.h>
#include <keyboard.h>

/*------------------------------------------------------------------------
 * kbdread  -  Read the status of the keyboard driver
 *------------------------------------------------------------------------
 */
devcall	kbdread (
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char          *buffer,        /* Address of buffer            */
          uint32        count           /* Length of buffer             */
	)
{
	if(getpid()!=kbd_state.pr_guest)	// NOT OPEN
		return SYSERR;
		
	for(int i=0; i<count; i++)
		buffer[i] = kbdgetc(devptr);

	return OK;
}
