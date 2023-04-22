#include "Startup/app.h"
#include "Win32/sdl/sdl.h"
#include "Win32/host.h"
#include "Application/Mersenne.h"

void HostGetTrackState(INT8* ps8X, INT8* ps8Y);

static UINT32 sg_u32Controls;
static BOOL (*sg_pKeyCallback)(SDL_keysym sKeySymbol, BOOL bMake) = NULL;
static void (*sg_pControllerCallback)(EControllerIndex eIndex, BOOL bChanged) = NULL;


typedef struct SControlInput
{
    SDLKey sKey;
	EControllerIndex eControllerIndex;
} SControlInput;

// map SDL joystick axis to GC controller
typedef struct SJoyAxisInput
{
	INT16 deadzone;
	INT16 lastValue;	// -1 = center, 0 = left/up, 1 = right/down
	EControllerIndex ctrl[2]; // 0 = left, up; 1 = right/down
} SJoyAxisInput;

typedef struct SJoyHatInput
{
	// last hat state.
	UINT8 state;
	EControllerIndex left, right, up, down;
} SJoyHatInput;

#define MAX_JOY_AXIS (8)
#define MAX_JOY_HAT (4)
#define MAX_JOY_BUTTONS (32)

typedef struct SJoyInput
{
	// if joystick is configured.
	BOOL bActive;
	SDL_Joystick *sdlJoy;
	// # of axis, hats and buttons;
	SJoyAxisInput axis[MAX_JOY_AXIS];
	SJoyHatInput hats[MAX_JOY_HAT];
	EControllerIndex buttons[MAX_JOY_BUTTONS];
} SJoyInput;

typedef struct SDLKeyNames {
	SDLKey k;
	const char *name;
} SDLKeyNames;

// XXX -- move me to BIOS?
// CTRL to key mappings
typedef struct SControllerNames {
	EControllerIndex ctrl;
	char *name;
} SControllerNames;

static BOOL sg_bControllers[CTRL_END];

static SControllerNames sg_sControllerNames[] = {
	{ CTRL_P2L, "P2L" },
	{ CTRL_P2R, "P2R" },
	{ CTRL_P2U, "P2U" },
	{ CTRL_P2D, "P2D" },
	{ CTRL_P2S, "P2S" },
	{ CTRL_P2C, "P2C" },

	{ CTRL_P1L, "P1L" },
	{ CTRL_P1R, "P1R" },
	{ CTRL_P1U, "P1U" },
	{ CTRL_P1D, "P1D" },
	{ CTRL_P1C, "P1C" },
	{ CTRL_P1S, "P1S" },

	{ CTRL_P1B1, "P1B1" },
	{ CTRL_P1B2, "P1B2" },
	{ CTRL_P1B3, "P1B3" },
	{ CTRL_P1B4, "P1B4" },
	{ CTRL_P1B5, "P1B5" },
	{ CTRL_P1B6, "P1B6" },

	{ CTRL_P2B1, "P2B1" },
	{ CTRL_P2B2, "P2B2" },
	{ CTRL_P2B3, "P2B3" },
	{ CTRL_P2B4, "P2B4" },
	{ CTRL_P2B5, "P2B5" },
	{ CTRL_P2B6, "P2B6" },

	{ CTRL_SERVICE, "SERVICE" },
	{ CTRL_TEST, "TEST" },
	{ CTRL_SERVICE2, "SERVICE2" },
	{ CTRL_MENU, "MENU" },
	{ CTRL_END } // terminator
};

// for input map parsing, maps strings to SDL key names
SDLKeyNames sg_sKeyNames[] = {
	{ SDLK_KP0, "KP0" },
	{ SDLK_KP1, "KP1" },
	{ SDLK_KP2, "KP2" },
	{ SDLK_KP3, "KP3" },
	{ SDLK_KP4, "KP4" },
	{ SDLK_KP5,"KP5" },
	{ SDLK_KP6, "KP6" },
	{ SDLK_KP7, "KP7" },
	{ SDLK_KP8, "KP8" },
	{ SDLK_KP9, "KP9" },
	{ SDLK_KP_PERIOD, "KP_PERIOD" },
	{ SDLK_KP_DIVIDE, "KP_DIVIDE" },
	{ SDLK_KP_MULTIPLY, "KP_MULTIPLY" },
	{ SDLK_KP_MINUS, "KP_MINUS" },
	{ SDLK_KP_PLUS, "KP_PLUS" },
	{ SDLK_KP_ENTER, "KP_ENTER" },
	{ SDLK_KP_EQUALS, "KP_EQUALS" },

	/* Arrows + Home/End pad */
	{ SDLK_UP, "UP" },
	{ SDLK_DOWN, "DOWN" },
	{ SDLK_RIGHT, "RIGHT" },
	{ SDLK_LEFT, "LEFT" },
	{ SDLK_INSERT, "INSERT" },
	{ SDLK_HOME, "HOME" },
	{ SDLK_END, "END" },
	{ SDLK_PAGEUP, "PAGEUP" },
	{ SDLK_PAGEDOWN, "PAGEDOWN" },

	/* Function keys */
	{ SDLK_F1, "F1" },
	{ SDLK_F2, "F2" },
	{ SDLK_F3, "F3" },
	{ SDLK_F4, "F4" },
	{ SDLK_F5, "F5" },
	{ SDLK_F6, "F6" },
	{ SDLK_F7, "F7" },
	{ SDLK_F8, "F8" },
	{ SDLK_F9, "F9" },
	{ SDLK_F10, "F10" },
	{ SDLK_F11, "F11" },
	{ SDLK_F12, "F12" },
	{ SDLK_F13, "F13" },
	{ SDLK_F14, "F14" },
	{ SDLK_F15, "F15" },

	{ SDLK_SPACE, "SPACE" },
	{ SDLK_SPACE, "SP" },
	{ SDLK_ESCAPE, "ESC" },
	{ SDLK_ESCAPE, "ESCAPE" },
	{ SDLK_TAB, "TAB" },

	/* Key state modifier keys */
	{ SDLK_NUMLOCK, "NUMLOCK" },
	{ SDLK_CAPSLOCK, "CAPSLOCK" },
	{ SDLK_SCROLLOCK, "SCROLLOCK" },
	{ SDLK_SCROLLOCK, "SCROLLLOCK" },
	{ SDLK_RSHIFT, "RSHIFT" },
	{ SDLK_LSHIFT, "LSHIFT" },
	{ SDLK_RCTRL, "RCTRL" },
	{ SDLK_LCTRL, "LCTRL" },
	{ SDLK_RALT, "RALT" },
	{ SDLK_LALT, "LALT" },
	{ SDLK_RMETA, "RMETA" },
	{ SDLK_LMETA, "RMETA" },
	{ SDLK_LSUPER, "LSUPER" },
	{ SDLK_RSUPER, "RSUPER" },
	{ SDLK_MODE, "MODE" },
	{ SDLK_COMPOSE, "COMPOSE" },

	/* Miscellaneous function keys */
	{ SDLK_HELP, "HELP" },
	{ SDLK_PRINT, "PRINT" },
	{ SDLK_SYSREQ, "SYSREQ" },
	{ SDLK_BREAK, "BREAK" },
	{ SDLK_MENU, "MENU" },
	{ SDLK_POWER, "POWER" },
	{ SDLK_EURO, "EURO" },
	{ SDLK_UNDO, "UNDO" },
	{ SDLK_LAST, 0 },	// terminator.
};

static EControllerIndex sg_sControlInputs[SDLK_LAST];
#define MAX_JOYSTICKS (4)
static SJoyInput sg_sJoyInputs[MAX_JOYSTICKS];
static SControlInput sg_sDefaultControlInputs[] =
{
	{SDLK_1,	CTRL_P1S},
	{SDLK_2,	CTRL_P2S},
	{SDLK_3,	CTRL_P1C},
	{SDLK_4,	CTRL_P2C},
	{SDLK_UP,	CTRL_P1U},
	{SDLK_DOWN,	CTRL_P1D},
	{SDLK_LEFT,	CTRL_P1L},
	{SDLK_RIGHT, CTRL_P1R},
	{SDLK_z,	CTRL_P1B1},
	{SDLK_x,	CTRL_P1B2},
	{SDLK_SPACE,CTRL_P1B3},
	{SDLK_v,	CTRL_P1B4},
	{SDLK_b,	CTRL_P1B5},
	{SDLK_n,	CTRL_P1B6},
	{SDLK_LCTRL,	CTRL_P1B1},
	{SDLK_LALT,		CTRL_P1B2},
	{SDLK_h,	CTRL_P2L },
	{SDLK_j,	CTRL_P2D },
	{SDLK_k,	CTRL_P2U },
	{SDLK_l,	CTRL_P2R },
	{SDLK_q,	CTRL_P2B1 },
	{SDLK_w,	CTRL_P2B2 },
	{SDLK_e,	CTRL_P2B3 },
	{SDLK_r,	CTRL_P2B4 },
	{SDLK_t,	CTRL_P2B5 },
	{SDLK_y,	CTRL_P2B6 },

	{SDLK_ESCAPE,	CTRL_MENU},
	{SDLK_SYSREQ,	CTRL_TEST},
	{SDLK_INSERT,	CTRL_SERVICE},

	// terminator.
	{SDLK_LAST},
};

// clear all input mappings.
static void InputMapReset(void)
{
	int i, j;
	SJoyInput *joy;

	// initialize controller mappings.
	for ( i = 0 ; i < SDLK_LAST; i++ )
	{
		sg_sControlInputs[i] = CTRL_END;	// invalid controller.
	}

	// init joysticks.
	for ( joy = sg_sJoyInputs, i=0; i < MAX_JOYSTICKS; i++, joy++ ) 
	{
		joy->bActive = FALSE;
		joy->sdlJoy = NULL;

		for ( j = 0 ; j < MAX_JOY_AXIS; j++ )
		{
			joy->axis[j].ctrl[0] = CTRL_END;
			joy->axis[j].ctrl[1] = CTRL_END;
			joy->axis[j].deadzone = 1000;
			joy->axis[j].lastValue = 0;
		}

		// hats.
		for (j = 0; j < MAX_JOY_HAT; j++)
		{
			joy->hats[j].up = CTRL_END;
			joy->hats[j].down = CTRL_END;
			joy->hats[j].left = CTRL_END;
			joy->hats[j].right = CTRL_END;
			joy->hats[j].state = 0;
		}

		// now buttons.
		for (j = 0; j < MAX_JOY_BUTTONS; j++)
		{
			joy->buttons[j] = CTRL_END;
		}
	}
}


// called if input.ini can't be opened -- installs
// default keymap.
static void InputMapSetDefault(void)
{
	SControlInput *p;
	
	InputMapReset();
	for ( p = sg_sDefaultControlInputs; p->sKey != SDLK_LAST; p++)
	{
		sg_sControlInputs[p->sKey] = p->eControllerIndex;
	}
}

static INT32 getNumber(char **pp)
{
	char *p = *pp;
	char *q;
	char numbuf[16];
	INT32 num;

	// skip leading whitespace
	while (*p && isspace(*p)) p++;
	// optional '-'
	if (*p == '-') p++;
	// allow hex.
	q = numbuf;
	while (*p && (isxdigit(*p)) && (q < numbuf + sizeof(numbuf)-1)) *q++ = *p++;
	*q = 0;

	// skip whitespace after number
	while (*p && isspace(*p)) p++;
	*pp = p;
	num = strtol(numbuf, &p, 0);
	if (num == 0 && *p != 0)
		return INT_MIN;

	return num;
}

static SDLKey FindSdlKeyByName(const char *name)
{
	SDLKeyNames *p; 

	// single char is ASCII.
	if (strlen(name) == 1)
	{
		SDLKey c = isupper(*name) ? tolower(*name) : *name;
		return c;
	}
	for (p = sg_sKeyNames; p->k != SDLK_LAST; p++) {
		if (_stricmp(name, p->name)==0) return p->k;
	}
	return SDLK_LAST;
}

static EControllerIndex FindCtrlByName( const char *name )
{
	SControllerNames *p;

	for (p = sg_sControllerNames; p->ctrl != CTRL_END; p++)
	{
		if (_stricmp(p->name, name) == 0) return p->ctrl;
	}
	return CTRL_END;
}


// parse a list of GC control names. 
// returns # of errors (i.e.: 0 == okay)
int parseCtrlList(char *fn, int lineno, char **pp, EControllerIndex *ctrls, int num)
{
	int i;
	char *p = *pp;
	char *gcCtrlName;
	int errs = 0;
	EControllerIndex gcCtrl;

	for (i = 0 ; i < num; i++) 
	{
		ctrls[i] = CTRL_END;
	}
	for ( i = 0 ; i < num; i++, ctrls++)
	{
		// skip whitespace before name
		while (*p && isspace(*p)) p++;
		gcCtrlName = p;
		while (*p && !isspace(*p)) p++;
		if (*p) *p++ = '\0';		// clobber trailing whitespace
		// match GC ctrl 
		if (*gcCtrlName == 0)
		{
			if (num == 1)
			{
				DebugOut("%s[%d]: Expected %d controler names, got %d.\n", fn, lineno, num, i );
			}
			else
			{
				DebugOut("%s[%d]: Expected controler name.\n", fn, lineno);
			}
			*pp = p;
			return 1;
		}

		if (strcmp(gcCtrlName,"-")==0) continue;

		gcCtrl = FindCtrlByName(gcCtrlName);
		if (gcCtrl == CTRL_END)
		{
			DebugOut("%s[%d]: Invalid GC Control '%s'\n", fn, lineno, gcCtrlName);
			errs++;
			continue;
		}
		*ctrls = gcCtrl;
	}
	*pp = p;
	return errs;
}

// initialize input maps.  tries to load 'input.ini', if it fails,
// it just installs a default input map.
// input map syntax (comments begin with ';'):
// [keyboard]
// <key> = <ctrl>
// [joy <n>]
// deadzone <n>
// axis <n> = <left/up> <right/down>
// hat <n> = <left> <right> <up> <down>
// button <n> = <ctrl>
void InputMapInit(char *fn)
{
	FILE *fp;
	SJoyInput *joy;
	char *p, *q, line[1024];
	int i, lineno, maxJoysticks, num, parse_mode, errs;
	char *sdlKeyName, *keyword;
	EControllerIndex gcCtrl;
	SDLKey sdlKey;
	BOOL activeJoy = FALSE;
	// parse_mode:
	// >0 = joystick #
	// PARSE_KEYBOARD = keyboard
	// PARSE_SKIP_SECTION = error in header; skipping section
	// PARSE_NO_SECTION = no mode set.
#define PARSE_KEYBOARD (-1)
#define PARSE_SKIP_SECTION (-2)
#define PARSE_NO_SECTION (-3)

	maxJoysticks = SDL_NumJoysticks();
	if (maxJoysticks > MAX_JOYSTICKS) maxJoysticks = MAX_JOYSTICKS;

	InputMapReset();
	fp = fopen(fn,"r");
	if (fp == NULL) goto bail_out;

	// parse input map file.


	parse_mode = PARSE_NO_SECTION;	
	lineno = 0;
	errs = 0;
	while (1)
	{
		lineno++;
		p = fgets(line, sizeof(line)-1, fp);
		if ( p == NULL) break;	// eof.

		q = strrchr(p,'\n');
		if (q) *q = '\0';	// clobber eol.

		// find first non-ws
		while (*p && isspace(*p)) p++;

		// skip comments and blank lines
		if (!*p || *p == ';') continue;

		// lowercase string
		for( q = p; *q; q++) 
		{
			*q = isupper(*q) ? tolower(*q) : *q;
		}

		// [joy<n>] or [keyboard] section start.
		if (*p == '[')
		{
			p++;	// skip '['
			if (!*p)
			{
				DebugOut("%s[%d]: Expected section name after [.\n", fn, lineno);
				parse_mode = PARSE_SKIP_SECTION;
				errs++;
			}

			q = p;	// mark start of word

			// get section name.
			while (*p && *p != ']') p++;
			if (*p != ']')
			{
				DebugOut("%s[%d]: Missing ']' in section name\n", fn, lineno);
				parse_mode = PARSE_SKIP_SECTION;
				errs++;
				continue;
			}
			if (*p) *p++ = '\0';	// terminate and skip ']'.

			// grok section name
			if (strcmp(q,"keyboard") == 0)
			{
				// keyboard parsing.
				// <key> = <ctrl>
				parse_mode = PARSE_KEYBOARD;
				continue;
			}
			else if (strncmp(q, "joy", 3) == 0)
			{
				q += 3;	// skip 'joy'
				// default to joystick 0.
				parse_mode = 0;
				if (*q)
				{
					if (!isdigit(*q) || q[1] != 0) 
					{
						DebugOut("%s[%d]: Invalid joystick number '%s'",fn,lineno,q);
						parse_mode = PARSE_SKIP_SECTION;
						errs++;
						continue;
					}
					parse_mode = *q - '0';
				}
				continue;
			}
			DebugOut("%s[%d]: Invalid section '%s'\n", fn, lineno, q);
			errs++;
			parse_mode = PARSE_SKIP_SECTION;
			continue;
		}
		// skipping this section for some reason?
		if (parse_mode == PARSE_SKIP_SECTION) continue;

		// no section type yet; error.
		if (parse_mode == PARSE_NO_SECTION)
		{
			DebugOut("%s[%d]: Syntax error; not in a [keyboard] or [joyN] section\n", fn, lineno);
			parse_mode = PARSE_SKIP_SECTION;
			errs++;
			continue;
		}

		// parse keyboard commands.
		if (parse_mode == PARSE_KEYBOARD )
		{
			sdlKeyName = p;
			while(*p && !isspace(*p)) p++;
			// skip optional whitespace after key name.
			if (isspace(*p))
			{
				*p++ = 0;	// terminate keyname
				while (*p && isspace(*p)) p++;
			}
			if (*p != '=')
			{
				DebugOut("%s[%d]: Expected '=' after SDL key nanme '%s'\n", fn, lineno, sdlKeyName);
				errs++;
				continue;
			}
			if (*p) *p++ = 0;	// terminate keyname
			sdlKey = FindSdlKeyByName(sdlKeyName);
			if (sdlKey == SDLK_LAST)
			{
				DebugOut("%s[%d]: Invalid SDL key name '%s'\n", fn, lineno, sdlKeyName);
				errs++;
				continue;
			}
			if (parseCtrlList(fn, lineno, &p, &gcCtrl, 1))
			{
				errs++;
				continue;
			}
			if (sg_sControlInputs[sdlKey] != CTRL_END) 
			{
				DebugOut("%s[%d]: Duplicate keyboard mapping for key \"%s\"\"", fn, lineno, sdlKeyName);
				errs++;
				continue;
			}
			// DebugOut("Map Key %s (%d) -> %d\n", sdlKeyName, sdlKey, gcCtrl);
			sg_sControlInputs[sdlKey] = gcCtrl;
			continue;
		}

		// parsing joystick
		joy = sg_sJoyInputs + parse_mode; 
		// if joystick isn't active, open it up.
		if (! joy->bActive)
		{
			activeJoy = TRUE;
			joy->sdlJoy = SDL_JoystickOpen(parse_mode);
			if (joy->sdlJoy == NULL)
			{
				DebugOut("%s[%d]: failed to open joystick %d, ignoring\n", fn, lineno, parse_mode );
				parse_mode = PARSE_SKIP_SECTION;	// skip section.
				continue;
			}
			DebugOut("%s[%d]: Opened Joy %d ('%s', axes=%d, buttons=%d, balls=%d, hats=%d)\n", 
					fn,lineno, parse_mode, SDL_JoystickName(parse_mode), 
					SDL_JoystickNumAxes(joy->sdlJoy),
					SDL_JoystickNumButtons(joy->sdlJoy),
					SDL_JoystickNumBalls(joy->sdlJoy),
					SDL_JoystickNumHats(joy->sdlJoy));
			joy->bActive = TRUE;
		}
		
		// now, look for first non-space.
		keyword = p;
		while (*p && !isspace(*p)) p++;
		if (*p) *p++ = 0;

		// get axis, button or hat number.
		num = getNumber(&p);
		if (num == INT_MIN)
		{
			DebugOut("%s[%d]: Invalid number", fn, lineno);
			errs++;
			continue;
		}

		if (*p != '=')
		{
			DebugOut("%s[%d]: Expected '='\n", fn,lineno);
			errs++;
			continue;
		}

		// skip '='.
		p++;

		// skip whitespace after '='
		while (*p && isspace(*p)) p++;

		if (strcmp(keyword,"deadzone")==0)
		{
			// deadzone <axis> = <value>
			// 'num' is axis number.
			num -=1;	// axis number from 1
			if (num < 0 || num >= MAX_JOY_AXIS)
			{
				DebugOut("%s[%d]: Invalid axis number %d\n", fn,lineno,num);
				errs++;
				continue;
			}
			// RHS is deadzone value.
			joy->axis[num].deadzone = getNumber(&p);
			if (joy->axis[num].deadzone == INT_MIN)
			{
				DebugOut("%s[%d]: Invalid number", fn, lineno);
				errs++;
				continue;
			}
			// DebugOut("Joy %d, axis %d: deadzone set to %d\n", parse_mode, num, joy->axis[num].deadzone);
			continue;
		}
		else if (strcmp(keyword,"axis")==0)
		{
			// axis <n> = <left/up> <right/down>
			num -=1;	// axis number from 1
			if (num < 0 || num >= MAX_JOY_AXIS)
			{
				DebugOut("%s[%d]: Invalid axis number %d\n", fn,lineno,num);
				errs++;
				continue;
			}
			if (num >= SDL_JoystickNumAxes(joy->sdlJoy))
			{
				DebugOut("%s[%d]: Warning: joy %d only has %d axes\n",
					fn,lineno, parse_mode, SDL_JoystickNumAxes(joy->sdlJoy));
				continue;
			}
			if (parseCtrlList(fn, lineno, &p, joy->axis[num].ctrl, 2))
			{
				errs++;
				continue;
			}
		}
		else if (strcmp(keyword,"button")==0)
		{
			// button <n> = <ctrl>
			num -=1;	// buttons number from 1
			if (num < 0 || num >= MAX_JOY_BUTTONS)
			{
				DebugOut("%s[%d]: Invalid button number %d\n", fn,lineno,num);
				errs++;
				continue;
			}
			if (num >= SDL_JoystickNumButtons(joy->sdlJoy))
			{
				DebugOut("%s[%d]: Warning: joy %d only has %d buttons\n",
					fn,lineno, parse_mode, SDL_JoystickNumButtons(joy->sdlJoy));
				continue;
			}
			if (parseCtrlList(fn, lineno, &p, &(joy->buttons[num]), 1))
			{
				errs++;
				continue;
			}
		}
		else if (strcmp(keyword,"hat")==0)
		{
			EControllerIndex parseTmp[4];
			num -=1;	// hat number from 1
			// hat <n> = <left> <right> <up> <down>
			if (num < 0 || num >= MAX_JOY_HAT)
			{
				DebugOut("%s[%d]: Invalid POV hat number %d\n", fn,lineno,num);
				errs++;
				continue;
			}
			if (num >= SDL_JoystickNumHats(joy->sdlJoy))
			{
				int nhat = SDL_JoystickNumHats(joy->sdlJoy);
				DebugOut("%s[%d]: Warning: joy %d has %d POV hat%s, can't map hat %d\n",
					fn,lineno,parse_mode, 
					nhat, nhat==1 ? "" : "s", num);
				continue;
			}
			if (parseCtrlList(fn, lineno, &p, parseTmp, 4))
			{
				errs++;
				continue;
			}
			joy->hats[num].left = parseTmp[0];
			joy->hats[num].right = parseTmp[1];
			joy->hats[num].up = parseTmp[2];
			joy->hats[num].down = parseTmp[3];
		}
		else
		{
			DebugOut("%s[%d]: Invalid mapping command '%s' for joy %d", 
				fn, lineno, keyword, parse_mode );
			errs++;
			continue;
		}

		// successfully parsed command/line.
	}
	fclose(fp);

	// successfully parsed everything.
	if (!errs) 
	{
		if (activeJoy) SDL_JoystickEventState(SDL_ENABLE);
		return;
	}
  
bail_out:
	for (i = 0 ; i < MAX_JOYSTICKS; i++ )
	{
		joy = sg_sJoyInputs + i;

		if (joy->sdlJoy && SDL_JoystickOpened(i))
			SDL_JoystickClose(joy->sdlJoy);

	    joy->sdlJoy = NULL;
	}

	if (fp) fclose(fp);
	InputMapSetDefault();
}

static UINT32 sg_u32RandomSeed = 0;

/* ************************************************************************* *\
** FUNCTION: GCInputSetCallback
\* ************************************************************************* */
void GCInputSetCallback(void (*pCallback)(EControllerIndex, BOOL))
{
	UINT32 u32Loop = 0;

	sg_pControllerCallback = pCallback;

	if (sg_pControllerCallback)
	{
		for (u32Loop = 0; u32Loop < CTRL_END; u32Loop++)
		{
			sg_pControllerCallback((EControllerIndex) u32Loop, sg_bControllers[u32Loop]);
		}
	}
}


/* ************************************************************************* *\
** FUNCTION: SetKeyCallback
\* ************************************************************************* */
void SetKeyCallback(BOOL (*pKeyCallback)(SDL_keysym, BOOL))
{
	sg_pKeyCallback = pKeyCallback;
}

// 'low' level controller manipulation -- calls user controller callback
// if callback is defined and control has actually changed.
static void InjectControllerEvent( EControllerIndex ctrl, BOOL bMake) 
{
	GCASSERT(ctrl >= 0 && ctrl < CTRL_END);

	// only notify on controllers that have NOT changed
	if (sg_bControllers[ctrl] == bMake ) return;

	if (sg_pControllerCallback) sg_pControllerCallback(ctrl, bMake);
	sg_bControllers[ctrl] = bMake;
}


void SetJoyAxisState(UINT8 joy, UINT8 axis, INT16 value )
{
	SJoyInput *j = sg_sJoyInputs + joy;
	SJoyAxisInput *a;
	int pos;

	GCASSERT(joy < MAX_JOYSTICKS);
	GCASSERT(axis < MAX_JOY_AXIS);
	if (! j->sdlJoy) return;

	a = j->axis + axis;

	// if axis value is not outside of deadzone, 
	pos = -1;
	if (value > a->deadzone)
	{
		pos = 1;
	}
	else if (value < -(a->deadzone)) 
	{
		pos = 0;
	}

	if (pos == a->lastValue) return;

	// 'break' controller
	if (a->lastValue != -1 && a->ctrl[a->lastValue] != CTRL_END) 
		InjectControllerEvent( a->ctrl[a->lastValue], FALSE );

	// 'make' new controller
	if (pos != -1 && a->ctrl[pos] != CTRL_END) 
		InjectControllerEvent( a->ctrl[pos], TRUE );

	a->lastValue = pos;
}

void SetJoyButtonState(UINT8 joy, UINT8 button, UINT8 bMake)
{
	SJoyInput *j = sg_sJoyInputs + joy;
	GCASSERT(joy < MAX_JOYSTICKS);
	GCASSERT(button < MAX_JOY_BUTTONS);
	if (! j->sdlJoy) return;

	if (j->buttons[button] != CTRL_END)
	{
		InjectControllerEvent(j->buttons[button], bMake );
	}
}

void SetJoyHatState(UINT8 joy, UINT8 hat, UINT8 value )
{
	SJoyInput *j = sg_sJoyInputs + joy;
	SJoyHatInput *h;
	UINT8 changed;
	GCASSERT(joy < MAX_JOYSTICKS);
	GCASSERT(hat < MAX_JOY_HAT);
	if (! j->sdlJoy) return;

	h = j->hats + hat;

	changed = value ^ h->state;

	if ((changed & SDL_HAT_UP) && h->up != CTRL_END)
		InjectControllerEvent( h->up, (value & SDL_HAT_UP) ? TRUE : FALSE );

	if ((changed & SDL_HAT_LEFT) && h->left != CTRL_END)
		InjectControllerEvent( h->left, (value & SDL_HAT_LEFT) ? TRUE : FALSE );

	if ((changed & SDL_HAT_DOWN) && h->down != CTRL_END)
		InjectControllerEvent( h->down, (value & SDL_HAT_DOWN) ? TRUE : FALSE );

	if ((changed & SDL_HAT_RIGHT) && h->right != CTRL_END)
		InjectControllerEvent( h->right, (value & SDL_HAT_RIGHT) ? TRUE : FALSE );

	h->state = value;
}

// handle SDL key events.
// search through keymap, if matching key is found, process it.

/* ************************************************************************* *\
** FUNCTION: SetKeyState
\* ************************************************************************* */
void SetKeyState(SDL_keysym sKeySymbol,
				 BOOL bMake)
{
	if (sg_pKeyCallback)
	{
		if (sg_pKeyCallback(sKeySymbol, bMake))
		{
			return;
		}
	}

	if (sg_sControlInputs[sKeySymbol.sym] == CTRL_END) return;

	InjectControllerEvent( sg_sControlInputs[sKeySymbol.sym], bMake );
}


/* ************************************************************************* *\
** FUNCTION: GCInputReadState
\* ************************************************************************* */
BOOL GCInputReadState(EControllerIndex eController)
{
	GCASSERT(eController < (sizeof(sg_bControllers) / sizeof(sg_bControllers[0])));
	return(sg_bControllers[eController]);
}

/* ************************************************************************* *\
** FUNCTION: GCInputReadTrackball
\* ************************************************************************* */
EGCResultCode GCInputReadTrackball(UINT8 u8TrackballNumber,
								   INT8 *ps8X,
								   INT8 *ps8Y)
{
	if (u8TrackballNumber >= 2)
	{
		return(GC_TRACKBALL_OUT_OF_RANGE);
	}


	if(u8TrackballNumber == 0)
	{
		HostGetTrackState(ps8X, ps8Y);
		return(GC_OK);
	}
	else
	{
		if (ps8X)
		{
			*ps8X = 0;
		}

		if (ps8Y)
		{
			*ps8Y = 0;
		}
	}

	return(GC_OK);
}

EGCResultCode GCInputSetTrackball(UINT8 u8TrackballNumber,
								  BOOL bInvertX,
								  BOOL bInvertY,
								  BOOL bSwapXY)
{
	if (u8TrackballNumber >= 2)
	{
		return(GC_TRACKBALL_OUT_OF_RANGE);
	}

	// Doesn't do jack under Windows
	return(GC_OK);
}

/* ************************************************************************* *\
** ************************************************************************* **
** EOF
** ************************************************************************* **
\* ************************************************************************* */
