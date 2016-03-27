/*Atomulator v1.0 by Tom Walker
   Debugger*/

#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>

#include "atom.h"
#include "debugger.h"
#include <stdio.h>
#include <string.h>

#undef printf

#define readmem(a) readmeml(a)
#define writemem(a, v) writememl(a, v)

int debug;
int indebug = 0;
int debug_on_brk = 0;

// MH - moved these here to get a clean compile on OSX
int fetchc[65536], readc[65536], writec[65536];

//---------------------------
ALLEGRO_BITMAP *mem;
ALLEGRO_STATE memstate;
ALLEGRO_LOCKED_REGION *mlr;
extern ALLEGRO_DISPLAY *memDisplay;

extern ALLEGRO_DISPLAY *inputDisplay;
extern ALLEGRO_EVENT_QUEUE *events;


ALLEGRO_TEXTLOG *debugLog;

static void lockMemScreen()
{
    al_store_state(&memstate, ALLEGRO_STATE_TARGET_BITMAP);
    al_set_target_bitmap(mem);
    mlr = al_lock_bitmap(mem, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
}

static void unlockMemScreen()
{
    al_unlock_bitmap(mem);
    al_restore_state(&memstate);
}

static void initMemVideo()
{
    mem = al_create_bitmap(256, 256);
    lockMemScreen();
}

//HACK FIXME
#define winmemsizex 256
#define winmemsizey 256

void drawMemScreen()
{

	int pos=0;
	int line=0;

	unsigned int *ptr;

	for (line=0; line<256; line++)
	{
		ptr = (unsigned int *)(mlr->data + mlr->pitch * line);

		for (pos=(line<<8); pos < ((line<<8)+256); pos++)
		{
			*ptr++ = (writec[pos]<<21) + (readc[pos]<<11) + (fetchc[pos]<<3);
			if (fetchc[pos]>0) fetchc[pos]--;
			if (readc[pos]>0) readc[pos]--;
			if (writec[pos]>0) writec[pos]--;
		}
	}

    unlockMemScreen();
    al_draw_scaled_bitmap(mem, 0, 0, 256, 256, 0, 0, winmemsizex, winmemsizey, 0);
    al_flip_display();
	al_clear_to_color(al_map_rgb(0,0,0));
    lockMemScreen();
}

void startdebug()
{
	if (debug)
	{
		//show memory console
        al_set_new_display_flags(ALLEGRO_WINDOWED|ALLEGRO_NO_PRESERVE_TEXTURE);

        // The screen handling performance has been improved by directly 
		// manipulating the bitmap the code assumes a 32bit pixel size
        al_set_new_display_option(ALLEGRO_COLOR_SIZE, 32, ALLEGRO_REQUIRE);

    	memDisplay = al_create_display(256, 256);
    	if (memDisplay == NULL)
        {
        	rpclog("Error creating Allegro memory display (32bit pixel size required).\n");
			return;
    	}

		al_set_window_title(memDisplay, "Memory viewer");
		initMemVideo();

		// Show output console
		debugLog = al_open_native_text_log("Debugger Output", ALLEGRO_TEXTLOG_NO_CLOSE|ALLEGRO_TEXTLOG_MONOSPACE);

		// Show input console
    	inputDisplay = al_create_display(640, 128);
    	if (inputDisplay == NULL)
        {
        	rpclog("Error creating Allegro input display (32bit pixel size required).\n");
			return;
    	}
		al_set_window_title(inputDisplay, "Debugger input console");
		al_register_event_source(events, al_get_display_event_source(inputDisplay));
	}
}

void enddebug()
{
	debug = debugon = 0;
}

static void debugout(char *s)
{
	al_append_native_text_log(debugLog, "%s", s);
	//printf("%s", s);
	//fflush(stdout);
	rpclog("%s", s);
}

void debuglog(char *format, ...)
{
	char buf[MAXPATH];

	va_list ap;
	va_start(ap, format);
	vsprintf(buf, format, ap);
	va_end(ap);

	debugout(buf);
}

int debugopen = 0;

uint32_t debugmemaddr = 0;
uint32_t debugdisaddr = 0;
uint8_t debuglastcommand = 0;

static uint8_t dreadmem(uint16_t addr)
{
	if (addr >= 0xA00 && addr < 0xAFF)
		return 0xFF;
	if (addr >= 0xB000 && addr < 0xBFFF)
		return 0xFF;
	return readmem(addr);
}
enum
{
	IMP, IMPA, IMM, ZP, ZPX, ZPY, INDX, INDY, IND, ABS, ABSX, ABSY, IND16, IND1X, BRA
};
uint8_t dopname[256][6] =
{
/*00*/ "BRK", "ORA", "---", "---", "TSB", "ORA", "ASL", "---", "PHP", "ORA", "ASL", "---", "TSB", "ORA", "ASL", "---",
/*10*/ "BPL", "ORA", "ORA", "---", "TRB", "ORA", "ASL", "---", "CLC", "ORA", "INC", "---", "TRB", "ORA", "ASL", "---",
/*20*/ "JSR", "AND", "---", "---", "BIT", "AND", "ROL", "---", "PLP", "AND", "ROL", "---", "BIT", "AND", "ROL", "---",
/*30*/ "BMI", "AND", "AND", "---", "BIT", "AND", "ROL", "---", "SEC", "AND", "DEC", "---", "BIT", "AND", "ROL", "---",
/*40*/ "RTI", "EOR", "---", "---", "---", "EOR", "LSR", "---", "PHA", "EOR", "LSR", "---", "JMP", "EOR", "LSR", "---",
/*50*/ "BVC", "EOR", "EOR", "---", "---", "EOR", "LSR", "---", "CLI", "EOR", "PHY", "---", "---", "EOR", "LSR", "---",
/*60*/ "RTS", "ADC", "---", "---", "STZ", "ADC", "ROR", "---", "PLA", "ADC", "ROR", "---", "JMP", "ADC", "ROR", "---",
/*70*/ "BVS", "ADC", "ADC", "---", "STZ", "ADC", "ROR", "---", "SEI", "ADC", "PLY", "---", "JMP", "ADC", "ROR", "---",
/*80*/ "BRA", "STA", "---", "---", "STY", "STA", "STX", "---", "DEY", "BIT", "TXA", "---", "STY", "STA", "STX", "---",
/*90*/ "BCC", "STA", "STA", "---", "STY", "STA", "STX", "---", "TYA", "STA", "TXS", "---", "STZ", "STA", "STZ", "---",
/*A0*/ "LDY", "LDA", "LDX", "---", "LDY", "LDA", "LDX", "---", "TAY", "LDA", "TAX", "---", "LDY", "LDA", "LDX", "---",
/*B0*/ "BCS", "LDA", "LDA", "---", "LDY", "LDA", "LDX", "---", "CLV", "LDA", "TSX", "---", "LDY", "LDA", "LDX", "---",
/*C0*/ "CPY", "CMP", "---", "---", "CPY", "CMP", "DEC", "---", "INY", "CMP", "DEX", "WAI", "CPY", "CMP", "DEC", "---",
/*D0*/ "BNE", "CMP", "CMP", "---", "---", "CMP", "DEC", "---", "CLD", "CMP", "PHX", "STP", "---", "CMP", "DEC", "---",
/*E0*/ "CPX", "SBC", "---", "---", "CPX", "SBC", "INC", "---", "INX", "SBC", "NOP", "---", "CPX", "SBC", "INC", "---",
/*F0*/ "BEQ", "SBC", "SBC", "---", "---", "SBC", "INC", "---", "SED", "SBC", "PLX", "---", "---", "SBC", "INC", "---",
};

int dopaddr[256] =
{
/*00*/ IMP, INDX,  IMP, IMP,  ZP,   ZP,	   ZP,	 IMP,	IMP,  IMM,   IMPA,  IMP,  ABS,	  ABS,	 ABS,  IMP,
/*10*/ BRA, INDY,  IND, IMP,  ZP,   ZPX,   ZPX,	 IMP,	IMP,  ABSY,  IMPA,  IMP,  ABS,	  ABSX,	 ABSX, IMP,
/*20*/ ABS, INDX,  IMP, IMP,  ZP,   ZP,	   ZP,	 IMP,	IMP,  IMM,   IMPA,  IMP,  ABS,	  ABS,	 ABS,  IMP,
/*30*/ BRA, INDY,  IND, IMP,  ZPX,  ZPX,   ZPX,	 IMP,	IMP,  ABSY,  IMPA,  IMP,  ABSX,	  ABSX,	 ABSX, IMP,
/*40*/ IMP, INDX,  IMP, IMP,  ZP,   ZP,	   ZP,	 IMP,	IMP,  IMM,   IMPA,  IMP,  ABS,	  ABS,	 ABS,  IMP,
/*50*/ BRA, INDY,  IND, IMP,  ZP,   ZPX,   ZPX,	 IMP,	IMP,  ABSY,  IMP,   IMP,  ABS,	  ABSX,	 ABSX, IMP,
/*60*/ IMP, INDX,  IMP, IMP,  ZP,   ZP,	   ZP,	 IMP,	IMP,  IMM,   IMPA,  IMP,  IND16,  ABS,	 ABS,  IMP,
/*70*/ BRA, INDY,  IND, IMP,  ZPX,  ZPX,   ZPX,	 IMP,	IMP,  ABSY,  IMP,   IMP,  IND1X,  ABSX,	 ABSX, IMP,
/*80*/ BRA, INDX,  IMP, IMP,  ZP,   ZP,	   ZP,	 IMP,	IMP,  IMM,   IMP,   IMP,  ABS,	  ABS,	 ABS,  IMP,
/*90*/ BRA, INDY,  IND, IMP,  ZPX,  ZPX,   ZPY,	 IMP,	IMP,  ABSY,  IMP,   IMP,  ABS,	  ABSX,	 ABSX, IMP,
/*A0*/ IMM, INDX,  IMM, IMP,  ZP,   ZP,	   ZP,	 IMP,	IMP,  IMM,   IMP,   IMP,  ABS,	  ABS,	 ABS,  IMP,
/*B0*/ BRA, INDY,  IND, IMP,  ZPX,  ZPX,   ZPY,	 IMP,	IMP,  ABSY,  IMP,   IMP,  ABSX,	  ABSX,	 ABSY, IMP,
/*C0*/ IMM, INDX,  IMP, IMP,  ZP,   ZP,	   ZP,	 IMP,	IMP,  IMM,   IMP,   IMP,  ABS,	  ABS,	 ABS,  IMP,
/*D0*/ BRA, INDY,  IND, IMP,  ZP,   ZPX,   ZPX,	 IMP,	IMP,  ABSY,  IMP,   IMP,  ABS,	  ABSX,	 ABSX, IMP,
/*E0*/ IMM, INDX,  IMP, IMP,  ZP,   ZP,	   ZP,	 IMP,	IMP,  IMM,   IMP,   IMP,  ABS,	  ABS,	 ABS,  IMP,
/*F0*/ BRA, INDY,  IND, IMP,  ZP,   ZPX,   ZPX,	 IMP,	IMP,  ABSY,  IMP,   IMP,  ABS,	  ABSX,	 ABSX, IMP,
};

uint8_t dopnamenmos[256][6] =
{
/*00*/ "BRK", "ORA", "HLT", "SLO", "NOP", "ORA", "ASL", "SLO", "PHP", "ORA", "ASL", "ANC", "NOP", "ORA", "ASL", "SLO",
/*10*/ "BPL", "ORA", "HLT", "SLO", "NOP", "ORA", "ASL", "SLO", "CLC", "ORA", "NOP", "SLO", "NOP", "ORA", "ASL", "SLO",
/*20*/ "JSR", "AND", "HLT", "RLA", "NOP", "AND", "ROL", "RLA", "PLP", "AND", "ROL", "ANC", "BIT", "AND", "ROL", "RLA",
/*30*/ "BMI", "AND", "HLT", "RLA", "NOP", "AND", "ROL", "RLA", "SEC", "AND", "NOP", "RLA", "NOP", "AND", "ROL", "RLA",
/*40*/ "RTI", "EOR", "HLT", "SRE", "NOP", "EOR", "LSR", "SRE", "PHA", "EOR", "LSR", "ASR", "JMP", "EOR", "LSR", "SRE",
/*50*/ "BVC", "EOR", "HLT", "SRE", "NOP", "EOR", "LSR", "SRE", "CLI", "EOR", "NOP", "SRE", "NOP", "EOR", "LSR", "SRE",
/*60*/ "RTS", "ADC", "HLT", "RRA", "NOP", "ADC", "ROR", "RRA", "PLA", "ADC", "ROR", "ARR", "JMP", "ADC", "ROR", "RRA",
/*70*/ "BVS", "ADC", "HLT", "RRA", "NOP", "ADC", "ROR", "RRA", "SEI", "ADC", "NOP", "RRA", "NOP", "ADC", "ROR", "RRA",
/*80*/ "BRA", "STA", "NOP", "SAX", "STY", "STA", "STX", "SAX", "DEY", "NOP", "TXA", "ANE", "STY", "STA", "STX", "SAX",
/*90*/ "BCC", "STA", "HLT", "SHA", "STY", "STA", "STX", "SAX", "TYA", "STA", "TXS", "SHS", "SHY", "STA", "SHX", "SHA",
/*A0*/ "LDY", "LDA", "LDX", "LAX", "LDY", "LDA", "LDX", "LAX", "TAY", "LDA", "TAX", "LXA", "LDY", "LDA", "LDX", "LAX",
/*B0*/ "BCS", "LDA", "HLT", "LAX", "LDY", "LDA", "LDX", "LAX", "CLV", "LDA", "TSX", "LAS", "LDY", "LDA", "LDX", "LAX",
/*C0*/ "CPY", "CMP", "NOP", "DCP", "CPY", "CMP", "DEC", "DCP", "INY", "CMP", "DEX", "SBX", "CPY", "CMP", "DEC", "DCP",
/*D0*/ "BNE", "CMP", "HLT", "DCP", "NOP", "CMP", "DEC", "DCP", "CLD", "CMP", "NOP", "DCP", "NOP", "CMP", "DEC", "DCP",
/*E0*/ "CPX", "SBC", "NOP", "ISB", "CPX", "SBC", "INC", "ISB", "INX", "SBC", "NOP", "SBC", "CPX", "SBC", "INC", "ISB",
/*F0*/ "BEQ", "SBC", "HLT", "ISB", "NOP", "SBC", "INC", "ISB", "SED", "SBC", "NOP", "ISB", "NOP", "SBC", "INC", "ISB",
};

int dopaddrnmos[256] =
{
/*00*/ IMP, INDX,  IMP, INDX,  ZP,  ZP,	  ZP,	ZP,   IMP,   IMM,   IMPA,  IMM,	  ABS,	  ABS,	ABS,  ABS,
/*10*/ BRA, INDY,  IMP, INDY,  ZPX, ZPX,  ZPX,	ZPX,  IMP,   ABSY,  IMP,   ABSY,  ABSX,	  ABSX, ABSX, ABSX,
/*20*/ ABS, INDX,  IMP, INDX,  ZP,  ZP,	  ZP,	ZP,   IMP,   IMM,   IMPA,  IMM,	  ABS,	  ABS,	ABS,  ABS,
/*30*/ BRA, INDY,  IMP, INDY,  ZPX, ZPX,  ZPX,	ZPX,  IMP,   ABSY,  IMP,   ABSY,  ABSX,	  ABSX, ABSX, ABSX,
/*40*/ IMP, INDX,  IMP, INDX,  ZP,  ZP,	  ZP,	ZP,   IMP,   IMM,   IMPA,  IMM,	  ABS,	  ABS,	ABS,  ABS,
/*50*/ BRA, INDY,  IMP, INDY,  ZPX, ZPX,  ZPX,	ZPX,  IMP,   ABSY,  IMP,   ABSY,  ABSX,	  ABSX, ABSX, ABSX,
/*60*/ IMP, INDX,  IMP, INDX,  ZP,  ZP,	  ZP,	ZP,   IMP,   IMM,   IMPA,  IMM,	  IND16,  ABS,	ABS,  ABS,
/*70*/ BRA, INDY,  IMP, INDY,  ZPX, ZPX,  ZPX,	ZPX,  IMP,   ABSY,  IMP,   ABSY,  ABSX,	  ABSX, ABSX, ABSX,
/*80*/ BRA, INDX,  IMM, INDX,  ZP,  ZP,	  ZP,	ZP,   IMP,   IMM,   IMP,   IMM,	  ABS,	  ABS,	ABS,  ABS,
/*90*/ BRA, INDY,  IMP, INDY,  ZPX, ZPX,  ZPY,	ZPY,  IMP,   ABSY,  IMP,   ABSY,  ABSX,	  ABSX, ABSX, ABSX,
/*A0*/ IMM, INDX,  IMM, INDX,  ZP,  ZP,	  ZP,	ZP,   IMP,   IMM,   IMP,   IMM,	  ABS,	  ABS,	ABS,  ABS,
/*B0*/ BRA, INDY,  IMP, INDY,  ZPX, ZPX,  ZPY,	ZPY,  IMP,   ABSY,  IMP,   ABSY,  ABSX,	  ABSX, ABSY, ABSX,
/*C0*/ IMM, INDX,  IMM, INDX,  ZP,  ZP,	  ZP,	ZP,   IMP,   IMM,   IMP,   IMM,	  ABS,	  ABS,	ABS,  ABS,
/*D0*/ BRA, INDY,  IMP, INDY,  ZPX, ZPX,  ZPX,	ZPX,  IMP,   ABSY,  IMP,   ABSY,  ABSX,	  ABSX, ABSX, ABSX,
/*E0*/ IMM, INDX,  IMM, INDX,  ZP,  ZP,	  ZP,	ZP,   IMP,   IMM,   IMP,   IMM,	  ABS,	  ABS,	ABS,  ABS,
/*F0*/ BRA, INDY,  IMP, INDY,  ZPX, ZPX,  ZPX,	ZPX,  IMP,   ABSY,  IMP,   ABSY,  ABSX,	  ABSX, ABSX, ABSX,
};


static void debugdisassemble()
{
	uint16_t temp;
	char s[256];
	uint8_t op = dreadmem(debugdisaddr);
	uint8_t p1 = dreadmem(debugdisaddr + 1), p2 = dreadmem(debugdisaddr + 2);

	sprintf(s, "%04X : ", debugdisaddr);
	debugout(s);
	debugout((char *)dopnamenmos[op]);
	debugout(" ");
	switch (dopaddr[op])
	{
	case IMP:
		sprintf(s, "        ");
		debugout(s);
		break;
	case IMPA:
		sprintf(s, "A       ");
		debugout(s);
		break;
	case IMM:
		sprintf(s, "#%02X     ", p1);
		debugout(s);
		debugdisaddr++;
		break;
	case ZP:
		sprintf(s, "%02X      ", p1);
		debugout(s);
		debugdisaddr++;
		break;
	case ZPX:
		sprintf(s, "%02X,X    ", p1);
		debugout(s);
		debugdisaddr++;
		break;
	case ZPY:
		sprintf(s, "%02X,Y    ", p1);
		debugout(s);
		debugdisaddr++;
		break;
	case IND:
		sprintf(s, "(%02X)    ", p1);
		debugout(s);
		debugdisaddr++;
		break;
	case INDX:
		sprintf(s, "(%02X,X)  ", p1);
		debugout(s);
		debugdisaddr++;
		break;
	case INDY:
		sprintf(s, "(%02X),Y  ", p1);
		debugout(s);
		debugdisaddr++;
		break;
	case ABS:
		sprintf(s, "%02X%02X    ", p2, p1);
		debugout(s);
		debugdisaddr += 2;
		break;
	case ABSX:
		sprintf(s, "%02X%02X,X  ", p2, p1);
		debugout(s);
		debugdisaddr += 2;
		break;
	case ABSY:
		sprintf(s, "%02X%02X,Y  ", p2, p1);
		debugout(s);
		debugdisaddr += 2;
		break;
	case IND16:
		sprintf(s, "(%02X%02X)  ", p2, p1);
		debugout(s);
		debugdisaddr += 2;
		break;
	case IND1X:
		sprintf(s, "(%02X%02X,X)", p2, p1);
		debugout(s);
		debugdisaddr += 2;
		break;
	case BRA:
		temp = debugdisaddr + 2 + (signed char)p1;
		sprintf(s, "%04X    ", temp);
		debugout(s);
		debugdisaddr++;
		break;
	}
	debugdisaddr++;
//        WriteConsole(consf,"\n",strlen("\n"),NULL,NULL);
}

int breakpoints[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
int breakr[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
int breakw[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
int watchr[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
int watchw[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
int debugstep = 0;

void debugread(uint16_t addr)
{
	int c;
	char outs[256];

	for (c = 0; c < 8; c++)
	{
		if (breakr[c] == addr)
		{
			debug = 1;
			sprintf(outs, "    Break on read from %04X\n", addr);
			debugout(outs);
			return;
		}
		if (watchr[c] == addr)
		{
			sprintf(outs, "    Read from %04X - A=%02X X=%02X Y=%02X PC=%04X\n", addr, a, x, y, pc);
			debugout(outs);
		}
	}
}
void debugwrite(uint16_t addr, uint8_t val)
{
	int c;
	char outs[256];

	for (c = 0; c < 8; c++)
	{
		if (breakw[c] == addr)
		{
			debug = 1;
			sprintf(outs, "    Break on write to %04X - val %02X\n", addr, val);
			debugout(outs);
			return;
		}
		if (watchw[c] == addr)
		{
			sprintf(outs, "    Write %02X to %04X - A=%02X X=%02X Y=%02X PC=%04X\n", val, addr, a, x, y, pc);
			debugout(outs);
		}
	}
}

extern char inputString[256];
extern bool inputStringReady;

uint16_t oldpc, oldoldpc, pc3;
void dodebugger(int linenum)
{
	int c, d, e, f;
	int params;
	uint8_t temp;
	char outs[256];
	char ins[256];
	int lines;

	if ((!opcode) && debug_on_brk)
	{
		sprintf(outs, "BRK %04X! %04X %04X\n", pc, oldpc, oldoldpc);
		debugout(outs);
		debug = 1;
	}
//	if (!opcode)
//		debug = 1;

	for (c = 0; c < 8; c++)
	{
		if (breakpoints[c] == pc)
		{
			debug = 1;
			sprintf(outs, "    Break at %04X\n", pc);
			debugout(outs);
		}
	}
	if (!debug)
		return;
//        if (!opcode) printf("BRK at %04X\n",pc);
	if (debugstep)
	{
		debugstep--;
		if (debugstep)
			return;
	}
/*        if (!debugopen)
        {
                debugopen=1;
                debugdisaddr=pc;
        }*/
	indebug = 1;

	while (1)
	{
		d = debugdisaddr;
		debugdisaddr = pc;
		debugdisassemble();
		debugdisaddr = d;

		if (!inputStringReady)
			break;
		else
		{
			strcpy(ins, inputString);
			debugout(ins);
		}

		// Force the screen to be refreshed before each debug command
		for (lines = 0; lines < linenum; lines++) {
			if (lines < 262 || lines == 311) {
				drawline(lines);
			}
		}

		sprintf(outs, "  >");
		debugout(outs);
#ifdef WIN32
/*                gotstr=0;
                while (!gotstr && debug) sleep(10);
                if (!debug)
                {
                        indebug=0;
                        return;
                }
                strcpy(ins,debugconsoleins);*/
		// Come back to these lines, commented out so we compile under Visual Studio
		// c = ReadConsoleA(cinf, ins, 255, (LPDWORD)&d, NULL);
		// ins[d] = 0;
#else
		d = (int)fgets(ins, 255, stdin);
//                gets(ins);
#endif
//printf("Got %s %i\n",ins,d);
//                rpclog("Got %s\n",ins);
//                rpclog("%i chars %i chars return %i\n",strlen(ins),d,c);
		d = 0;
		while (ins[d] != 32 && ins[d] != 0xA && ins[d] != 0xD && ins[d] != 0)
			d++;
		while (ins[d] == 32)
			d++;
		if (ins[d] == 0xA || ins[d] == 0xD || ins[d] == 0)
			params = 0;
		else
			params = 1;

		if (ins[0] == 0xA || ins[0] == 0xD)
			ins[0] = debuglastcommand;
//debugout("Processing!\n");
		switch (ins[0])
		{
		case 'c': case 'C':
			debug = 0;
//                        debugopen=0;
//                        FreeConsole();
			indebug = 0;
			return;
		case 'm': case 'M':
			if (params)
				sscanf(&ins[d], "%X", (unsigned int*)&debugmemaddr);
			for (c = 0; c < 16; c++)
			{
				sprintf(outs, "    %04X : ", debugmemaddr);
				debugout(outs);
				for (d = 0; d < 16; d++)
				{
					sprintf(outs, "%02X ", dreadmem(debugmemaddr + d));
					debugout(outs);
				}
				debugout("  ");
				for (d = 0; d < 16; d++)
				{
					temp = dreadmem(debugmemaddr + d);
					if (temp < 32)
						sprintf(outs, ".");
					else
						sprintf(outs, "%c", temp);
					debugout(outs);
				}
				debugmemaddr += 16;
				debugout("\n");
			}
			break;
		case 'd': case 'D':
			if (params)
				sscanf(&ins[d], "%X", (unsigned int*)&debugdisaddr);
			for (c = 0; c < 12; c++)
			{
				debugout("    ");
				debugdisassemble();
				debugout("\n");
			}
			break;
		case 'r': case 'R':
			sprintf(outs, "    6502 registers :\n");
			debugout(outs);
			sprintf(outs, "    A=%02X X=%02X Y=%02X S=01%02X PC=%04X\n", a, x, y, s, pc);
			debugout(outs);
			sprintf(outs, "    Status : %c%c%c%c%c%c\n", (p.n) ? 'N' : ' ', (p.v) ? 'V' : ' ', (p.d) ? 'D' : ' ', (p.i) ? 'I' : ' ', (p.z) ? 'Z' : ' ', (p.c) ? 'C' : ' ');
			debugout(outs);
			break;
		case 's': case 'S':
			if (params)
				sscanf(&ins[d], "%i", &debugstep);
			else
				debugstep = 1;
			debuglastcommand = ins[0];
			indebug = 0;
			return;
		case 'b': case 'B':
			if (!strncasecmp(ins, "breakr", 6))
			{
				if (!params)
					break;
				for (c = 0; c < 8; c++)
				{
					if (breakpoints[c] == -1)
					{
						sscanf(&ins[d], "%X", &breakr[c]);
						sprintf(outs, "    Read breakpoint %i set to %04X\n", c, breakr[c]);
						debugout(outs);
						break;
					}
				}
			}
			else if (!strncasecmp(ins, "breakw", 6))
			{
				if (!params)
					break;
				for (c = 0; c < 8; c++)
				{
					if (breakpoints[c] == -1)
					{
						sscanf(&ins[d], "%X", &breakw[c]);
						sprintf(outs, "    Write breakpoint %i set to %04X\n", c, breakw[c]);
						debugout(outs);
						break;
					}
				}
			}
			else if (!strncasecmp(ins, "break", 5))
			{
				if (!params)
					break;
				for (c = 0; c < 8; c++)
				{
					if (breakpoints[c] == -1)
					{
						sscanf(&ins[d], "%X", &breakpoints[c]);
						sprintf(outs, "    Breakpoint %i set to %04X\n", c, breakpoints[c]);
						debugout(outs);
						break;
					}
				}
			}
			if (!strncasecmp(ins, "blist", 5))
			{
				for (c = 0; c < 8; c++)
				{
					if (breakpoints[c] != -1)
					{
						sprintf(outs, "    Breakpoint %i : %04X\n", c, breakpoints[c]);
						debugout(outs);
					}
				}
				for (c = 0; c < 8; c++)
				{
					if (breakr[c] != -1)
					{
						sprintf(outs, "    Read breakpoint %i : %04X\n", c, breakr[c]);
						debugout(outs);
					}
				}
				for (c = 0; c < 8; c++)
				{
					if (breakw[c] != -1)
					{
						sprintf(outs, "    Write breakpoint %i : %04X\n", c, breakr[c]);
						debugout(outs);
					}
				}
			}
			if (!strncasecmp(ins, "bclearr", 7))
			{
				if (!params)
					break;
				sscanf(&ins[d], "%X", &e);
				for (c = 0; c < 8; c++)
				{
					if (breakr[c] == e)
						breakr[c] = -1;
					if (c == e)
						breakr[c] = -1;
				}
			}
			else if (!strncasecmp(ins, "bclearw", 7))
			{
				if (!params)
					break;
				sscanf(&ins[d], "%X", &e);
				for (c = 0; c < 8; c++)
				{
					if (breakw[c] == e)
						breakw[c] = -1;
					if (c == e)
						breakw[c] = -1;
				}
			}
			else if (!strncasecmp(ins, "bclear", 6))
			{
				if (!params)
					break;
				sscanf(&ins[d], "%X", &e);
				for (c = 0; c < 8; c++)
				{
					if (breakpoints[c] == e)
						breakpoints[c] = -1;
					if (c == e)
						breakpoints[c] = -1;
				}
			}
			break;
		case 'w': case 'W':
			if (!strncasecmp(ins, "watchr", 6))
			{
				if (!params)
					break;
				for (c = 0; c < 8; c++)
				{
					if (watchr[c] == -1)
					{
						sscanf(&ins[d], "%X", &watchr[c]);
						sprintf(outs, "    Read watchpoint %i set to %04X\n", c, watchr[c]);
						debugout(outs);
						break;
					}
				}
				break;
			}
			if (!strncasecmp(ins, "watchw", 6))
			{
				if (!params)
					break;
				for (c = 0; c < 8; c++)
				{
					if (watchw[c] == -1)
					{
						sscanf(&ins[d], "%X", &watchw[c]);
						sprintf(outs, "    Write watchpoint %i set to %04X\n", c, watchw[c]);
						debugout(outs);
						break;
					}
				}
				break;
			}
			if (!strncasecmp(ins, "wlist", 5))
			{
				for (c = 0; c < 8; c++)
				{
					if (watchr[c] != -1)
					{
						sprintf(outs, "    Read watchpoint %i : %04X\n", c, watchr[c]);
						debugout(outs);
					}
				}
				for (c = 0; c < 8; c++)
				{
					if (watchw[c] != -1)
					{
						sprintf(outs, "    Write watchpoint %i : %04X\n", c, watchw[c]);
						debugout(outs);
					}
				}
			}
			if (!strncasecmp(ins, "wclearr", 7))
			{
				if (!params)
					break;
				sscanf(&ins[d], "%X", &e);
				for (c = 0; c < 8; c++)
				{
					if (watchr[c] == e)
						watchr[c] = -1;
					if (c == e)
						watchr[c] = -1;
				}
			}
			else if (!strncasecmp(ins, "wclearw", 7))
			{
				if (!params)
					break;
				sscanf(&ins[d], "%X", &e);
				for (c = 0; c < 8; c++)
				{
					if (watchw[c] == e)
						watchw[c] = -1;
					if (c == e)
						watchw[c] = -1;
				}
			}
			else if (!strncasecmp(ins, "writem", 6))
			{
				if (!params)
					break;
				sscanf(&ins[d], "%X %X", &e, &f);
				rpclog("WriteM %04X %04X\n", e, f);
				writemem(e, f);
			}
			break;
		case 'q': case 'Q':
			setquit();
			while (1)
				;
			break;
		case 'h': case 'H': case '?':
			sprintf(outs, "\n    Debugger commands :\n\n");
			debugout(outs);
			sprintf(outs, "    bclear n   - clear breakpoint n or breakpoint at n\n");
			debugout(outs);
			sprintf(outs, "    bclearr n  - clear read breakpoint n or read breakpoint at n\n");
			debugout(outs);
			sprintf(outs, "    bclearw n  - clear write breakpoint n or write breakpoint at n\n");
			debugout(outs);
			sprintf(outs, "    blist      - list current breakpoints\n");
			debugout(outs);
			sprintf(outs, "    break n    - set a breakpoint at n\n");
			debugout(outs);
			sprintf(outs, "    breakr n   - break on reads from address n\n");
			debugout(outs);
			sprintf(outs, "    breakw n   - break on writes to address n\n");
			debugout(outs);
			sprintf(outs, "    c          - continue running indefinitely\n");
			debugout(outs);
			sprintf(outs, "    d [n]      - disassemble from address n\n");
			debugout(outs);
			sprintf(outs, "    m [n]      - memory dump from address n\n");
			debugout(outs);
			sprintf(outs, "    q          - force emulator exit\n");
			debugout(outs);
			sprintf(outs, "    r          - print 6502 registers\n");
			debugout(outs);
			sprintf(outs, "    s [n]      - step n instructions (or 1 if no parameter)\n\n");
			debugout(outs);
			sprintf(outs, "    watchr n   - watch reads from address n\n");
			debugout(outs);
			sprintf(outs, "    watchw n   - watch writes to address n\n");
			debugout(outs);
			sprintf(outs, "    wclearr n  - clear read watchpoint n or read watchpoint at n\n");
			debugout(outs);
			sprintf(outs, "    wclearw n  - clear write watchpoint n or write watchpoint at n\n");
			debugout(outs);
			sprintf(outs, "    writem a v - write to memory, a = address, v = value\n");
			debugout(outs);
			break;
		}
		debuglastcommand = ins[0];
//                WriteConsole(consf,"\n",1,NULL,NULL);
//                WriteConsole(consf,ins,strlen(ins),NULL,NULL);
	}
	indebug = 0;
}