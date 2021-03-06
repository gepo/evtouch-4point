
/*
 * simple and generic touchscreen calibration program using the linux 2.6
 * input event interface
 *
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>

#ifndef DISABLE_HAL
#include <dbus/dbus.h>
#include <libhal.h>
#endif

#include <getopt.h>

/*****************************************************************************/
/* stuff for event interface */
struct input_event {
	struct timeval time;
	unsigned short type;
	unsigned short code;
	int value;
};

/* event types */
#define EV_SYN			0x00
#define EV_KEY			0x01
#define EV_REL			0x02
#define EV_ABS			0x03

/* codes */
#define ABS_X			0x00
#define ABS_Y			0x01
#define SYN_REPORT		0
#define BTN_LEFT		0x110
#define BTN_RIGHT		0x111
#define BTN_TOUCH		0x14a

/*****************************************************************************/
/* some constants */
#define FONT_NAME		"9x15"
#define IDLETIMEOUT		15
#define BLINKPERD		0.16        

#define ROUND_SYMBOL
#define NumRect			5

#if 0 
#define Background		cCYAN
#define	TouchedCross		cYELLOW
#define BlinkCrossBG		cRED
#define BlinkCrossFG		cWHITE
#define nTouchedCross		cBLUE
#define Cross			cWHITE
#define DrawGrid		cWHITE
#define DrawLine		cYELLOW
#define DrawUp			cRED
#define DrawDown		cBLUE
#define TimerLine		cRED
#define PromptText		cBLUE
#else
#define Background		cBLACK
#define	TouchedCross		cYELLOW
#define BlinkCrossBG		cRED
#define BlinkCrossFG		cWHITE
#define nTouchedCross		cBLUE
#define Cross			cYELLOW
#define DrawGrid		cGREEN
#define DrawLine		cYELLOW
#define DrawUp			cRED
#define DrawDown		cGREEN
#define TimerLine		cRED
#define PromptText		cWHITE
#endif

#define N_Colors		10

static char colors[N_Colors][10] =
{ "BLACK", "WHITE", "RED", "YELLOW", "GREEN", "BLUE", "#40C0C0" };
				
static unsigned long pixels[N_Colors];

#define cBLACK			(pixels[0])
#define cWHITE			(pixels[1])
#define cRED			(pixels[2])
#define cYELLOW			(pixels[3])
#define cGREEN			(pixels[4])
#define cBLUE			(pixels[5])
#define cCYAN			(pixels[6])

/* some stupid loops */
#define SYS_1( zzz... ) do {	\
	while ( (zzz) != 1 );	\
} while (0)

#define SYS_0( zzz... ) do {	\
	while ( (zzz) != 0 );	\
} while (0)

/* where the calibration points are placed */
#define SCREEN_DIVIDE	16
#define SCREEN_MAX	0x800
#define M_POINT		(SCREEN_MAX/SCREEN_DIVIDE)
int MARK_POINT[] = { M_POINT, SCREEN_MAX - 1 - M_POINT };

/*****************************************************************************/
/* globals */
int job_done = 0;
int points_touched = 0;
char *deviceName;
int points_x[4], points_y[4];

Display *display;
int screen;
GC gc;
Window root;
Window win;
XFontStruct *font_info;
unsigned int width, height;	/* window size */
char *progname;
int evfd;

int verbose_output = 0;

#ifndef DISABLE_HAL
int use_hal = 0;
char* hal_udi = NULL;
DBusError dbus_error;
static LibHalContext *hal_ctx;

void evtouch_set_property(char* key, int ivalue);
#endif

/*****************************************************************************/

int x = -1, y = -1;

int get_events(int *px, int *py)
{
	int ret;
	int touch = 0, sync = 0;
	struct input_event ev;
    int limit = 10;

	/* read till sync event */
	while (sync == 0 || touch == 0) {
		ret = read(evfd, &ev, sizeof(ev));
		if (ret == -1)
			return -1;

		switch (ev.type) {
		case EV_ABS:
			switch (ev.code) {
			case ABS_X:
				if (x == -1)
					x = ev.value;
				break;

			case ABS_Y:
				if (y == -1)
					y = ev.value;
				break;

			default:
                if (verbose_output) {
                    printf("Unknown ev.code=%d for ev.type=EV_ABS\n", ev.code);
                }
				break;
			}

			break;

		case EV_KEY:
			switch (ev.code) {
			case BTN_LEFT:
			case BTN_TOUCH:
				touch = 1;
                break;

			default:
                if (verbose_output) {
                    printf("Unknown ev.code=%d for ev.type=EV_KEY\n", ev.code);
                }
			    break;
			}

			break;

		case EV_SYN:
			if (ev.code == SYN_REPORT)
				sync = 1;
            else
                if (verbose_output) {
                    printf("Unknown ev.code=%d for ev.type=EV_SYN\n", ev.code);
                }

			break;

		default:
            if (verbose_output) {
                printf("Unknown ev.type=EV_SYN\n", ev.type);
            }
			break;
		}
	}

	if (!touch || x == -1 || y == -1)
		return -1;

	*px = x;
	*py = y;

    x = -1;
    y = -1;

	return 0;
}


/*****************************************************************************/

void cleanup_exit()
{
	SYS_1(XUnloadFont(display, font_info->fid));
	XUngrabServer(display);
	XUngrabKeyboard(display, CurrentTime);
	SYS_1(XFreeGC(display, gc));
	SYS_0(XCloseDisplay(display));
	close(evfd);
	exit(0);
}


void load_font(XFontStruct **font_info)
{
	char *fontname = FONT_NAME;

	if ((*font_info = XLoadQueryFont(display, fontname)) == NULL) {
		printf("Cannot open %s font\n", FONT_NAME);
		exit(1);
	}
}


void draw_point(int x, int y, int width, int size, unsigned long color)
{
	XSetForeground(display, gc, color);
	XSetLineAttributes(display, gc, width, LineSolid,
			   CapRound, JoinRound);
	XDrawLine(display, win, gc, x - size, y, x + size, y);
	XDrawLine(display, win, gc, x, y - size, x, y + size);
}


void point_blink(unsigned long color)
{
	int i, j;
	int cx, cy;
	static int shift = 0;

	if (points_touched != 4) {
		int RectDist = width / 200;
		i = points_touched / 2;
		j = points_touched % 2;
		cx = (MARK_POINT[j] * width) / SCREEN_MAX;
		cy = (MARK_POINT[i] * height) / SCREEN_MAX;

		XSetLineAttributes(display, gc, 1, LineSolid, CapRound, JoinRound);
		for (i = 0; i < NumRect; i++) {
			if ((i + shift) % NumRect == 0)
				XSetForeground(display, gc, BlinkCrossBG);
			else
				XSetForeground(display, gc, BlinkCrossFG);

#ifdef ROUND_SYMBOL
			XDrawArc(display, win, gc,
			         cx - i * RectDist, cy - i * RectDist,
			         i * (2 * RectDist), i * (2 * RectDist),
			         0, 359 * 64);
#else
			XDrawRectangle(display, win, gc,
				       cx - i * RectDist, cy - i * RectDist,
				       i * (2 * RectDist), i * (2 * RectDist));
#endif
		}
		shift++;
	}
}


void draw_message(char *msg)
{
	char buf[300];
	char *prompt[] = { buf };
#define num	(sizeof(prompt) / sizeof(prompt[0]))
	static int init = 0;
	static int p_len[num];
	static int p_width[num];
	static int p_height;
	static int p_maxwidth = 0;
	int i, x, y;
	int line_height;

	strncpy(buf, msg, sizeof buf);

	for (i = 0; i < num; i++) {
		p_len[i] = strlen(prompt[i]);
		p_width[i] = XTextWidth(font_info, prompt[i], p_len[i]);

		if (p_width[i] > p_maxwidth)
			p_maxwidth = p_width[i];
	}

	p_height = font_info->ascent + font_info->descent;
	init = 1;

	line_height = p_height + 5;
	x = (width - p_maxwidth) / 2;
	y = height / 2 - line_height;

	XSetForeground(display, gc, PromptText);
	XSetLineAttributes(display, gc, 3, LineSolid, CapRound, JoinRound);
	XClearArea(display, win, x - 8, y - 8 - p_height, p_maxwidth + 8 * 2,
	           num * line_height + 8 * 2, False);
	XDrawRectangle(display, win, gc, x - 8, y - 8 - p_height,
	               p_maxwidth + 8 * 2, num * line_height + 8 * 2);

	for (i = 0; i < num; i++) {
		XDrawString(display, win, gc, x, y + i * line_height, prompt[i],
			    p_len[i]);
	}
#undef num
}


void draw_text()
{
	static char *prompt[] = {
		"                    4-Pt Calibration",
		"Please touch the blinking symbol until beep or stop blinking",
		"                     (ESC to Abort)",
	};
#define num	(sizeof(prompt) / sizeof(prompt[0]))
	static int init = 0;
	static int p_len[num];
	static int p_width[num];
	static int p_height;
	static int p_maxwidth = 0;
	int i, x, y;
	int line_height;

	if (!init) {
		for (i = 0; i < num; i++) {
			p_len[i] = strlen(prompt[i]);
			p_width[i] = XTextWidth(font_info, prompt[i], p_len[i]);
			if (p_width[i] > p_maxwidth)
				p_maxwidth = p_width[i];
		}
		p_height = font_info->ascent + font_info->descent;
		init = 1;
	}
	line_height = p_height + 5;
	x = (width - p_maxwidth) / 2;
	y = height / 2 - 6 * line_height;

	XSetForeground(display, gc, PromptText);
	XClearArea(display, win, x - 11, y - 8 - p_height,
		   p_maxwidth + 11 * 2, num * line_height + 8 * 2, False);
	XSetLineAttributes(display, gc, 3, FillSolid,
			   CapRound, JoinRound);
	XDrawRectangle(display, win, gc, x - 11, y - 8 - p_height,
		       p_maxwidth + 11 * 2, num * line_height + 8 * 2);

	for (i = 0; i < num; i++) {
		XDrawString(display, win, gc, x, y + i * line_height, prompt[i],
			    p_len[i]);
	}
#undef num
}


void draw_graphics()
{
	int i, j;
	unsigned cx, cy;
	unsigned long color;

	draw_text();

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			int num = 2 * i + j;

			if (num == points_touched)
				continue;
			
			if (num > points_touched)
				color = nTouchedCross;
			else
				color = TouchedCross;

			cx = (MARK_POINT[j] * width) / SCREEN_MAX;
			cy = (MARK_POINT[i] * height) / SCREEN_MAX;

            if (verbose_output) {
                printf("drawed: x=%d,y=%d\n",cx,cy);
            }

			draw_point(cx, cy, width / 200, width / 64, color);
		}
	}
}


void get_gc(Window win, GC *gc, XFontStruct *font_info)
{
	unsigned long valuemask = 0;	/* ignore XGCvalues and use defaults */
	XGCValues values;
	unsigned int line_width = 5;
	int line_style = LineSolid;
	int cap_style = CapRound;
	int join_style = JoinRound;

	*gc = XCreateGC(display, win, valuemask, &values);

	XSetFont(display, *gc, font_info->fid);

	XSetLineAttributes(display, *gc, line_width, line_style,
			   cap_style, join_style);
}


int get_color()
{
	int default_depth;
	Colormap default_cmap;
	XColor my_color;
	int i;

	default_depth = DefaultDepth(display, screen);
	default_cmap = DefaultColormap(display, screen);

	for (i = 0; i < N_Colors; i++) {
		XParseColor(display, default_cmap, colors[i], &my_color);
		XAllocColor(display, default_cmap, &my_color);
		pixels[i] = my_color.pixel;
	}

	return 0;
}


Cursor create_empty_cursor()
{
	char nothing[] = { 0 };
	XColor nullcolor;
	Pixmap src = XCreateBitmapFromData(display, root, nothing, 1, 1);
	Pixmap msk = XCreateBitmapFromData(display, root, nothing, 1, 1);
	Cursor mycursor = XCreatePixmapCursor(display, src, msk,
					      &nullcolor, &nullcolor, 0, 0);
	XFreePixmap(display, src);
	XFreePixmap(display, msk);

	return mycursor;
}


void process_event()
{
	XEvent event;

	while (XCheckWindowEvent(display, win, -1, &event) == True) {
		switch (event.type) {
		case KeyPress:
			{
				KeySym keysym = XKeycodeToKeysym(display,
							     event.xkey.keycode, 0);

				if (keysym == XK_Escape) {
					puts("Aborted");
					cleanup_exit();
				}
			}
			break;

		case Expose:
			draw_graphics(win, gc, width, height);
			break;

		default:
			break;
		}
	}
}


double idle_time = 0;
double tick = 0;

void set_timer(double interval /* in second */ )
{
	struct itimerval timer;
	long sec = interval;
	long usec = (interval - sec) * 1.0e6;

	timer.it_value.tv_sec = sec;
	timer.it_value.tv_usec = usec;
	timer.it_interval = timer.it_value;
	setitimer(ITIMER_REAL, &timer, NULL);
	tick = interval;
}


void update_timer(void)
{
	int current = width * idle_time / IDLETIMEOUT;

	XSetLineAttributes(display, gc, 2, LineSolid, CapRound, JoinRound);
	XSetForeground(display, gc, Background);
	XDrawLine(display, win, gc, 0, height - 1, current, height - 1);
	XSetForeground(display, gc, TimerLine);
	XDrawLine(display, win, gc, current, height - 1, width, height - 1);
}


int register_fasync(int fd, void (*handle) (int))
{
	signal(SIGIO, handle);
	fcntl(fd, F_SETOWN, getpid());
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | FASYNC);
	return 0;
}


void sig_handler(int num)
{
	char buf[100];
	int i, rval, x, y;
	static int is_busy = 0;

	if (is_busy)
		return;
	is_busy = 1;

	switch (num) {
	case SIGALRM:
		if (!job_done)
			point_blink(BlinkCrossFG);

		update_timer();
		if (idle_time >= IDLETIMEOUT)
			cleanup_exit();

		idle_time += tick;
		XFlush(display);
		process_event();

		break;

	case SIGIO:
		rval = get_events(&x, &y);

		if (rval == -1)
			break;

		idle_time = 0;

		points_x[points_touched] = x;
		points_y[points_touched] = y;
	
        if (verbose_output) {
            printf("measured: x=%d,y=%d\n", x, y);
        }

		points_touched++;
		draw_graphics();

		break;

	default:
		break;
	}

	/* do the math */
	if (points_touched == 4 && !job_done) {
		int x_low, y_low, x_hi, y_hi;
		int x_min, y_min, x_max, y_max;
		int x_seg, y_seg;
		int x_inv = 0, y_inv = 0;

		/* get the averages */
		x_low = (points_x[0] + points_x[2]) / 2;
		y_low = (points_y[0] + points_y[1]) / 2;
		x_hi = (points_x[1] + points_x[3]) / 2;
		y_hi = (points_y[2] + points_y[3]) / 2;

        if (verbose_output) {
            printf("x_low=%d,y_low=%d,x_hi=%d,y_hi=%d\n", x_low, y_low, x_hi, y_hi);
        }

		/* calc the min and max values */
		x_seg = abs(x_hi - x_low) / (SCREEN_DIVIDE - 2);
		x_min = x_low - x_seg;
		x_max = x_hi + x_seg;

		y_seg = abs(y_hi - y_low) / (SCREEN_DIVIDE - 2);
		y_min = y_low - y_seg;
		y_max = y_hi + y_seg;

		/* print it, hint: evtouch has Y inverted */
		printf("Copy-Paste friendly, for evtouch XFree86 driver\n");
		printf("	Option \"MinX\" \"%d\"\n", x_min);
		printf("	Option \"MinY\" \"%d\"\n", y_min);
		printf("	Option \"MaxX\" \"%d\"\n", x_max);
		printf("	Option \"MaxY\" \"%d\"\n", y_max);

#ifndef DISABLE_HAL
        if (use_hal) {
            evtouch_set_property("MinX", x_min);
            evtouch_set_property("MinY", y_min);
            evtouch_set_property("MaxX", x_max);
            evtouch_set_property("MaxY", y_max);
        }
#endif

		draw_message("   Done...   ");
		XFlush(display);

		job_done = 1;
		idle_time = IDLETIMEOUT * 3 / 4;
		update_timer();
	}

	is_busy = 0;

	return;
}

void usage(char* programName)
{
    fprintf(stderr, "Usage: %s [OPTIONS] <device>\n\n", basename(programName));
    fprintf(stderr, "OPTIONS:\n");
#ifndef DISABLE_HAL
    fprintf(stderr, "\t--hal      set calibration results to Xorg via hal\n");
#endif
    fprintf(stderr, "\t--verbose  do output of debug data (into stdout)\n");
    fprintf(stderr, "\t--help|h   show this help\n");
}

#ifndef DISABLE_HAL
char* hal_find_by_property(char* key, char* value)
{
    int num_udis;
    char** udis;
    char* result;

    udis = libhal_manager_find_device_string_match (hal_ctx, key, value, &num_udis, &dbus_error);

    if (dbus_error_is_set (&dbus_error)) {
        fprintf (stderr, "error: %s: %s\n", dbus_error.name, dbus_error.message);
        dbus_error_free (&dbus_error);
        return NULL;
    }

    if (verbose_output) {
        fprintf (stderr, "Found %d device objects with string property %s = '%s'\n", num_udis, key, value);
    }

    if (num_udis == 0) {
        return NULL;
    }

    if (num_udis > 1) {
        //TODO: print device select menu
    }

    result = (char*)malloc(strlen(udis[0]) + 1);
    strcpy(result, udis[0]);

    libhal_free_string_array (udis);
    free(value);

    return result;
}

/*
 * Set several options: MinX,MaxX, MinY, MaxY
 *  for udi = hal_udi
 */
void evtouch_set_property(char* key, int ivalue)
{
    char* optionName = (char*)malloc(50);
    dbus_bool_t rc = 0;
    char* value = (char*)malloc(10);
    sprintf(value, "%d", ivalue);

    optionName = "input.x11_options.";
    optionName = strcat(optionName, key);

    rc = libhal_device_set_property_string (hal_ctx, hal_udi, optionName, value, &dbus_error);

    free(optionName);

    if (!rc) {
        fprintf (stderr, "error: libhal_device_set_property: %s: %s\n", dbus_error.name, dbus_error.message);
        dbus_error_free (&dbus_error);
    }
}
#endif

int main(int argc, char *argv[], char *env[])
{
	char *display_name = NULL;
	XSetWindowAttributes xswa;

    int help_flag = 0;
    char* deviceName;

    if (argc == 1) {
        help_flag = 1;
    } else {
        /* parse arguments */
        struct option long_options[] =
        {
#ifndef DISABLE_HAL
            {"hal", no_argument, &use_hal, 1},
#endif
            {"verbose", no_argument, &verbose_output, 1},
            {"help", no_argument, &help_flag, 1},
            {0, 0, 0, 0}
        };

        while (1) {
            int option_index = 0;
            int c = getopt_long(argc, argv, "h", long_options, &option_index);

            if (c == -1)
                break;

            switch (c) {
            case 0:
                /* Nothing to do */
                break;
            case 'h':
                help_flag = 1;
                break;
            default:
                exit(EXIT_FAILURE);
            }
        }
    }

    if (help_flag) {
        usage(argv[0]);
        return 0;
    }

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }

#ifndef DISABLE_HAL
    if (use_hal) {
        dbus_error_init(&dbus_error);
        if ((hal_ctx = libhal_ctx_new ()) == NULL) {
            fprintf (stderr, "error: libhal_ctx_new\n");
            return 1;
        }

        if (!libhal_ctx_set_dbus_connection (hal_ctx, dbus_bus_get (DBUS_BUS_SYSTEM, &dbus_error))) {
            fprintf (stderr, "error: libhal_ctx_set_dbus_connection: %s: %s\n", dbus_error.name, dbus_error.message);
            LIBHAL_FREE_DBUS_ERROR (&dbus_error);
            return 1;
        }

        if (!libhal_ctx_init (hal_ctx, &dbus_error)) {
            if (dbus_error_is_set(&dbus_error)) {
                fprintf (stderr, "error: libhal_ctx_init: %s: %s\n", dbus_error.name, dbus_error.message);
                dbus_error_free (&dbus_error);
            }

            fprintf (stderr, "Could not initialise connection to hald.\n"
            "Normally this means the HAL daemon (hald) is not running or not ready.\n");
            return 1;
        }

        hal_udi = hal_find_by_property("input.x11_driver", "evtouch");
        if (hal_udi == NULL) {
            fprintf (stderr, "couldn't find evtouch device via hal");
            return 1;
        }
    }
#endif

    deviceName = argv[optind];

	evfd = open(deviceName, O_RDONLY | O_NONBLOCK);
	if (evfd == -1) {
		fprintf(stderr, "Cannot open device file '%s': %s\n", deviceName, strerror(errno));
		return 1;
	}

	/* connect to X server */
	if ((display = XOpenDisplay(display_name)) == NULL) {
		fprintf(stderr, "%s: cannot connect to X server %s\n",
			progname, XDisplayName(display_name));
		close(evfd);
		exit(1);
	}

	screen = DefaultScreen(display);
	root = RootWindow(display, screen);

	/* setup window attributes */
	xswa.override_redirect = True;
	xswa.background_pixel = BlackPixel(display, screen);
	xswa.event_mask = ExposureMask | KeyPressMask;
	xswa.cursor = create_empty_cursor();

	/* get screen size from display structure macro */
	width = DisplayWidth(display, screen);
	height = DisplayHeight(display, screen);

	win = XCreateWindow(display, RootWindow(display, screen),
	                    0, 0, width, height, 0,
	                    CopyFromParent, InputOutput, CopyFromParent,
	                    CWOverrideRedirect | CWBackPixel | CWEventMask |
	                    CWCursor, &xswa);
	XMapWindow(display, win);
	XGrabKeyboard(display, win, False, GrabModeAsync, GrabModeAsync,
	              CurrentTime);
	XGrabServer(display);
	load_font(&font_info);
	get_gc(win, &gc, font_info);
	get_color();

	XSetWindowBackground(display, win, Background);
	XClearWindow(display, win);

	signal(SIGALRM, sig_handler);
	set_timer(BLINKPERD);
	register_fasync(evfd, sig_handler);

	/* wait for signals */
	while (1)
		pause();

	return 0;
}
