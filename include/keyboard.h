
#define inportb(p)      inb(p)
#define outportb(p,v)   outb(p,v)

#define KEYBOARD_DATAREG 0x60  // :Data Register(Read\Write)
#define KEYBOARD_CMDREG 0x64   // :Command Register(Read\Write)

#define KBD_BUFFER_SIZE 10

extern unsigned char kblayout [128];  // { ... } Fill your layout yourself 

struct kbd_state_t {
    unsigned char buffer[KBD_BUFFER_SIZE];
    uint32 count;
    uint32 tail;
    uint32 head;
    sid32 sem_buffer;
    sid32 sem_device;
    pid32 pr_guest;
};

extern struct kbd_state_t kbd_state;

/* kbd control function codes */
#define	KBD_CLEAR	0		/* Clear buffer	*/