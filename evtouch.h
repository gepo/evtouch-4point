
/*
 *
 * Copyright 2004 by Kenan Esau <kenan.esau@conan.de>, Baltmannsweiler, 
 * Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of copyright holders not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The copyright holders
 * make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#ifndef _evtouch_H_
#define _evtouch_H_


/******************************************************************************
 *  Definitions
 *  structs, typedefs, #defines, enums
 *****************************************************************************/

/* Physical Screen Dimensions. (for default values)
   For the Lifebook B Series currently 800x600 pixels */
#define EV_SCREEN_WIDTH       800
#define EV_SCREEN_HEIGHT      600
#define EV_AXIS_MIN_RES       0
#define EV_AXIS_MAX_RES       1024
#define EV_PAN_BORDER         12

#define EV_TIMEOUT            500

/*
 * The event structure itself
 */


/*
 * Event types
 */
#define EV_SYN			0x00
#define EV_KEY			0x01
#define EV_REL			0x02
#define EV_ABS			0x03

/*
 * Absolute axes
 */
#define ABS_X			0x00
#define ABS_Y			0x01

/*
 * Buttons
 */
#define BTN_LEFT		0x110
#define BTN_RIGHT		0x111
#define BTN_TOUCH		0x14a

#define TOUCHED 0x01
#define X_COORD 0x02
#define Y_COORD 0x04
#define LB_STAT 0x08  /* LB up / down */
#define RB_STAT 0x10  /* RB up / down (both needed for 3-btn emu) */

typedef struct _EVTouchPrivateRec
{
        int min_x;  /* Minimum x reported by calibration        */
        int max_x;  /* Maximum x                    */
        int min_y;  /* Minimum y reported by calibration        */
        int max_y;  /* Maximum y                    */

        int click_timer;
        int drag_timer;
        int emulate3;
        int emulate3_timeout;
        Bool emulate3_timer_expired;

        int move_limit;

        int cur_x;
        int cur_y;
        int old_x;
        int old_y;
        /* pointers to the current viewport coordinates */
        int *pViewPort_X0;    /* Min X */
        int *pViewPort_X1;    /* Max X */
        int *pViewPort_Y0;    /* Min Y */
        int *pViewPort_Y1;    /* Max Y */
        int virtual;          /* virtual=1 indicates that there is a virtual screen */
        int x;                /* x in screen coords */
        int y;                /* y in screen coords */
        int phys_width;       /* Physical X-Resolution */
        int phys_height;      /* Physical Y-Resolution */

        struct input_event ev_touched;
        int touch_x;
        int touch_y;
        unsigned char touch_flags; /* 1 - touched, 2 - x-coord received
                                      4 - y-coord received */
        Bool drag;
        Bool currently_dragging;
        Bool pan_viewport;

        int button_number;   /* which button to report */
        int reporting_mode;   /* TS_Raw or TS_Scaled */

        int screen_num;    /* Screen associated with the device */
        int screen_width;   /* Width of the associated X screen  */
        int screen_height;   /* Height of the screen              */
        
        XISBuffer *buffer;
        struct input_event ev; /* packet being/just read */
        
        int packeti;    /* index into packet */
        Bool cs7flag;
        Bool binary_pkt;   /* indicates packet was a binary touch */

        int bin_byte;    /* bytes recieved in binary packet */

        LocalDevicePtr local;
} EVTouchPrivateRec, *EVTouchPrivatePtr;

/******************************************************************************
 *  Declarations
 *****************************************************************************/
/*int DumpOpts (XF86OptionPtr opts); */
static Bool DeviceControl (DeviceIntPtr dev, int mode);
static Bool DeviceOn (DeviceIntPtr dev);
static Bool DeviceOff (DeviceIntPtr dev);
static Bool DeviceInit (DeviceIntPtr dev);
static void ReadInput (LocalDevicePtr local);
/* static int ControlProc (LocalDevicePtr local, xDeviceCtl * control); */
static void ControlProc(DeviceIntPtr device, PtrCtrl *ctrl);
static void CloseProc (LocalDevicePtr local);
static int SwitchMode (ClientPtr client, DeviceIntPtr dev, int mode);
static Bool ConvertProc (LocalDevicePtr local, int first, int num, int v0, int v1, int v2, int v3, int v4, int v5, int *x, int *y);
static Bool QueryHardware (LocalDevicePtr local);
static void EVTouchNewPacket (EVTouchPrivatePtr priv);

/*static Bool EVTouchSendCommand(LocalDevicePtr local, unsigned char command[]);*/
static unsigned char EVTouchRead (EVTouchPrivatePtr priv);
static Bool EVTouchGetPacket (EVTouchPrivatePtr priv);
void EVTouchLBRBEvent(EVTouchPrivatePtr priv);

#ifdef LOG_RAW_PACKET
#define EVTouchDumpPacketToLog(priv) ( xf86ErrorFVerb(2, "EVTOUCHPCKT %d %d %d\n",                                       priv->ev.type, priv->ev.code, priv->ev.value ) )
#else
#define EVTouchDumpPacketToLog(priv)
#endif
/* 
 *    DO NOT PUT ANYTHING AFTER THIS ENDIF
 */
#endif
