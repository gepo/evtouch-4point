
#define _evdev_touch_C_

#include "xorg-server.h"
//#include <xf86Version.h>
//#if XF86_VERSION_CURRENT >= XF86_VERSION_NUMERIC(3,9,0,0,0)
//#define XFREE86_V4
//#endif

/*****************************************************************************
 *        Standard Headers
 ****************************************************************************/
#define SYSCALL(call)        while(((call) == -1) && (errno == EINTR))
#include <misc.h>

#define NEED_EVENTS
#include <X11/X.h>
#include <X11/Xproto.h>

#include <xf86.h>

#ifdef XINPUT
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#include "extnsionst.h"
#include "extinit.h"
#else
#include "inputstr.h"
#endif

#include <xf86Xinput.h>
#include <xf86_OSproc.h>
#include "xf86OSmouse.h"
#include <exevents.h>

#ifndef NEED_XF86_TYPES
#define NEED_XF86_TYPES	/* for xisb.h when !XFree86LOADER */
#endif

//#ifdef XFree86LOADER
#include "xf86Module.h"
//#endif

#ifdef DBG
#undef DBG
#endif

static int debug_level = 0;
#define DBG(lvl, f) {if ((lvl) <= debug_level) f;}


/*****************************************************************************
 *        Local Headers
 ****************************************************************************/
#include "xisb.h"
#include "linux/input.h"
#include "evtouch.h"

/*****************************************************************************
 *        Variables without includable headers
 ****************************************************************************/

/*****************************************************************************
 *        Local Variables
 ****************************************************************************/


static InputInfoPtr
EVTouchPreInit(InputDriverPtr drv, IDevPtr dev, int flags);

_X_EXPORT InputDriverRec EVTOUCH = {
        1,
        "evtouch",
        NULL,
        EVTouchPreInit,
        /*EVTouchUnInit*/ NULL,
        NULL,
        0
};

//#ifdef XFree86LOADER

static XF86ModuleVersionInfo VersionRec =
{
        "evtouch",
        "Kenan Esau",
        MODINFOSTRING1,
        MODINFOSTRING2,
        XORG_VERSION_CURRENT,
        0, 5, 1,
        ABI_CLASS_XINPUT,
        ABI_XINPUT_VERSION,
        MOD_CLASS_XINPUT,
        {0, 0, 0, 0}                                /* signature, to be patched into the file by
                                                     * a tool */
};

#ifdef LBDBG
#define DBGOUT(lvl, ...) (xf86ErrorFVerb(lvl, __VA_ARGS__))
#else 
#define DBGOUT(lvl, ...)
#endif

static pointer
Plug( pointer module,
      pointer options,
      int *errmaj,
      int *errmin )
{
//        xf86AddModuleInfo(&EVTouchInfo, module);
        xf86AddInputDriver(&EVTOUCH, module, 0);
        return module;
}


static void
Unplug(pointer        p)
{
        DBGOUT(1, "Unplug\n");
}


_X_EXPORT XF86ModuleData evtouchModuleData = {
        &VersionRec, 
        Plug, 
        Unplug
};

//#endif /* XFree86LOADER */


static const char *default_options[] =
{
        "BaudRate", "9600",
        "StopBits", "1",
        "DataBits", "8",
        "Parity", "None",
        "Vmin", "5",
        "Vtime", "1",
        "FlowControl", "None"
};


/*****************************************************************************
 *        Function Definitions
 ****************************************************************************/




static unsigned long 
time_passed(struct input_event *ev1,
            struct input_event *ev2)
{
        unsigned long t1 = 0;
        unsigned long t2 = 0;
        unsigned long diff = 0;

        t1 = ev1->time.tv_usec / 1000;
        t2 = ((ev2->time.tv_sec - ev1->time.tv_sec) * 1000) + 
                ev2->time.tv_usec / 1000;
        
        diff = t2 - t1;
        DBGOUT(2, "diff = %u t1 = %u t2= %u\n", diff, t1, t2);
        return diff;
}

static int
delta(int x1, int x2)
{
        return (x1 > x2) ? x1 - x2 : x2 - x1;
}


static CARD32
emulate3Timer(OsTimerPtr timer, CARD32 now, pointer _local)
{
        int sigstate;

        LocalDevicePtr local = (LocalDevicePtr)_local;
        EVTouchPrivatePtr priv = (EVTouchPrivatePtr) local->private;

        sigstate = xf86BlockSIGIO ();

        xf86PostMotionEvent(local->dev, TRUE, 0, 2, 
                            priv->cur_x, 
                            priv->cur_y);

        /* 
         * Emit a button press -- release is handled in EVTouchLBRBEvent
         */
        if ( ( priv->touch_flags & LB_STAT ) &&
             !( priv->touch_flags & RB_STAT ) ) {
                DBGOUT(2, "Left Press\n");
                xf86PostButtonEvent (local->dev, TRUE,
                                     1, 1, 0, 2, 
                                     priv->cur_x, 
                                     priv->cur_y);
        }

        if ( ( priv->touch_flags & RB_STAT ) &&
             !( priv->touch_flags & LB_STAT ) ) {
                DBGOUT(2, "Right Press\n");
                xf86PostButtonEvent (local->dev, TRUE,
                                     3, 1, 0, 2, 
                                     priv->cur_x, 
                                     priv->cur_y);
        }

        /*
          Handling "middle" button press
        */
        if ( ( priv->touch_flags & RB_STAT ) &&
             ( priv->touch_flags & LB_STAT ) ) {
                DBGOUT(2, "Middle Press\n");
                xf86PostButtonEvent (local->dev, TRUE,
                                     2, 1, 0, 2, 
                                     priv->cur_x, 
                                     priv->cur_y);
        }


        priv->emulate3_timer_expired=TRUE;
        xf86UnblockSIGIO (sigstate);
             
        return 0;
}




static InputInfoPtr
EVTouchPreInit(InputDriverPtr drv, IDevPtr dev, int flags)
{
        /* LocalDevicePtr local; */
        InputInfoPtr local;
        EVTouchPrivatePtr priv;

        char *s;
        priv = xcalloc (1, sizeof (EVTouchPrivateRec));
        if (!priv)
                return NULL;

        local = xf86AllocateInput(drv, 0);
        if (!local) {
                xfree(priv);
                return NULL;
        }

        local->name = dev->identifier;
        local->type_name = XI_TOUCHSCREEN;
        local->device_control = DeviceControl;
        local->read_input = ReadInput;
        local->control_proc = ControlProc;
        local->close_proc = CloseProc;
        local->switch_mode = SwitchMode;
        local->conversion_proc = ConvertProc;
        local->reverse_conversion_proc = NULL;
        local->fd = -1;
        local->dev = NULL;
        local->private = priv;
        priv->local = local;
        local->private_flags = 0;
        local->flags = XI86_POINTER_CAPABLE | XI86_SEND_DRAG_EVENTS;
        local->conf_idev = dev;

        xf86CollectInputOptions(local, default_options, NULL);

        xf86OptionListReport(local->options);

        local->fd = xf86OpenSerial (local->options);
        if (local->fd == -1)
        {
                ErrorF ("EVTouch driver unable to open device\n");
                goto SetupProc_fail;
        }
        xf86ErrorFVerb( 3, "evdev device opened successfully\n" );

        priv->min_x = xf86SetIntOption( local->options, "MinX", 0 );
        priv->max_x = xf86SetIntOption( local->options, "MaxX", EV_SCREEN_WIDTH );
        priv->min_y = xf86SetIntOption( local->options, "MinY", 0 );
        priv->max_y = xf86SetIntOption( local->options, "MaxY", EV_SCREEN_HEIGHT );
        priv->screen_num = xf86SetIntOption( local->options, "ScreenNumber", 0 );
        priv->button_number = xf86SetIntOption( local->options, "ButtonNumber", 2 );
        priv->drag_timer = xf86SetIntOption(local->options, "DragTimer", 180);
        priv->click_timer = xf86SetIntOption(local->options, 
                                              "ClickTimer", 500);
        priv->emulate3 = xf86SetIntOption(local->options, "Emulate3Buttons", 1);
        priv->emulate3_timeout = xf86SetIntOption( local->options, 
                                                 "Emulate3Timeout", 50);
        priv->move_limit = xf86SetIntOption( local->options, "MoveLimit", 12 );
        
        

#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) == 0
        xf86AlwaysCore(local, TRUE);
#endif

        s = xf86FindOptionValue (local->options, "ReportingMode");
        if ((s) && (xf86NameCmp (s, "raw") == 0))
                priv->reporting_mode = TS_Raw;
        else
                priv->reporting_mode = TS_Scaled;

        priv->buffer = XisbNew (local->fd, 200);
        priv->touch_flags = 0;
        priv->drag = FALSE;

        DBG (9, XisbTrace (priv->buffer, 1));

        if (QueryHardware(local) != Success)
        {
                ErrorF ("Unable to query/initialize EVTouch hardware.\n");
                goto SetupProc_fail;
        }

        local->history_size = xf86SetIntOption( local->options, "HistorySize", 0 );

        /* prepare to process touch packets */
        EVTouchNewPacket (priv);

        /* this results in an xstrdup that must be freed later */
        local->name = xf86SetStrOption( local->options, "DeviceName", "EVTouch TouchScreen" );
        xf86ProcessCommonOptions(local, local->options);
        local->flags |= XI86_CONFIGURED;

        if (local->fd != -1)
        { 
                if (priv->buffer)
                {
                        XisbFree(priv->buffer);
                        priv->buffer = NULL;
                }
                xf86CloseSerial(local->fd);
        }
        local->fd = -1;
        return (local);

 SetupProc_fail:
        if ((local) && (local->fd))
                xf86CloseSerial (local->fd);
        if ((local) && (local->name))
                xfree (local->name);

        if ((priv) && (priv->buffer))
                XisbFree (priv->buffer);
        if (priv)
                xfree (priv);
        return (local);
}




void EVTouchProcessAbs(EVTouchPrivatePtr priv)
{
        struct input_event *ev; /* packet being/just read */
        int dx;
        int dy;
        unsigned long diff;
        LocalDevicePtr local = priv->local;

        ev = &priv->ev;
        priv->old_x = priv->cur_x;
        priv->old_y = priv->cur_y;
        if ( ev->code == ABS_X ) {
                priv->cur_x = ev->value;

                if ( (priv->touch_flags & TOUCHED) &&
                     !(priv->touch_flags & X_COORD) ) {
                        priv->touch_x = ev->value;
                        priv->touch_flags |= X_COORD;
                }
                dx   = delta(priv->touch_x, ev->value);
                if (dx > priv->move_limit) {
                        priv->drag = FALSE;
                        DBGOUT(2, "cancelling  drag dx = %d\n", dx);
                }
        } 
        if ( ev->code == ABS_Y ) {
                priv->cur_y = ev->value;

                if ( (priv->touch_flags & TOUCHED) &&
                     !(priv->touch_flags & Y_COORD) ) {
                        priv->touch_y = ev->value;
                        priv->touch_flags |= Y_COORD;
                }
                dy   = delta(priv->touch_y, ev->value);
                if (dy > priv->move_limit) {
                        priv->drag = FALSE;
                        DBGOUT(2, "cancelling  drag dy = %d\n", dy);
                }
        }
        
        diff = time_passed(&priv->ev_touched, ev);
        if ( ( diff > priv->drag_timer ) && 
             ( priv->drag == TRUE ) ) {
                DBGOUT(2, "starting drag\n");

                xf86PostButtonEvent(local->dev, TRUE,
                                    1, 1, 0, 2, 
                                    priv->cur_x, 
                                    priv->cur_y);
                priv->currently_dragging = TRUE;
        }
}




void EVTouchProcessRel(EVTouchPrivatePtr priv)
{
        struct input_event *ev; /* packet being/just read */

        ev = &priv->ev;
        priv->old_x = priv->cur_x;
        priv->old_y = priv->cur_y;
        if ( ev->code == ABS_X ) {
                priv->cur_x += ev->value;
                if (priv->cur_x > priv->max_x)
                        priv->cur_x = priv->max_x;
                if (priv->cur_x < priv->min_x)
                        priv->cur_x = priv->min_x;
                return;
        } 
        if ( ev->code == ABS_Y ) {
                priv->cur_y += ev->value;
                if (priv->cur_y > priv->max_y)
                        priv->cur_y = priv->max_y;
                if (priv->cur_y < priv->min_y)
                        priv->cur_y = priv->min_y;                    
                return;
        } 
}




void EVTouchProcessKey(EVTouchPrivatePtr priv)
{
        struct input_event *ev; /* packet being/just read */
        LocalDevicePtr local = priv->local;
        unsigned long diff = 0;
        int btn = 0;

        ev = &priv->ev;
        if ( (ev->code == BTN_LEFT) || 
             (ev->code == BTN_RIGHT) ) {
                /* give lb and rb-events some special treatment 
                   (emulate3 or not, ...) */
                EVTouchLBRBEvent(priv);
                return;
        }
        
        if (ev->code == BTN_TOUCH) {
                if (ev->value == 1) {
                        priv->touch_flags |= TOUCHED;
                        priv->drag     = TRUE;
                        memcpy(&priv->ev_touched, ev, 
                               sizeof(struct input_event));
                } else {
                        priv->touch_flags = 0;
                        if (priv->currently_dragging) {
                                DBGOUT(2, "stopping drag\n");

                                xf86PostButtonEvent(local->dev, TRUE,
                                                    1, 0, 0, 2, 
                                                    priv->cur_x, 
                                                    priv->cur_y);
                                priv->currently_dragging = FALSE;
                        }
                        diff = time_passed(&priv->ev_touched, ev);
                        if ( diff <= priv->click_timer) {
                                DBGOUT(2, "Pointer click\n");

                                xf86PostButtonEvent(local->dev, TRUE,
                                                    1, 1, 0, 2, 
                                                    priv->cur_x, 
                                                    priv->cur_y);
                                xf86PostButtonEvent(local->dev, TRUE,
                                                    1, 0, 0, 2, 
                                                    priv->cur_x, 
                                                    priv->cur_y);
                        }
                }
                return;
        }

#ifdef _0_
        switch (ev->code) {
        case BTN_SIDE:
                break;
        case BTN_EXTRA:
                break;
        case BTN_FORWARD:
                break;
        case BTN_BACK:
                break;
        case BTN_TASK:
                break;
        default:
                return;
        }
        xf86PostButtonEvent(local->dev, TRUE,
                            btn, ev->value, 0, 2, 
                            priv->cur_x, 
                            priv->cur_y);
#endif

        return;
}




void EVTouchLBRBEvent(EVTouchPrivatePtr priv)
{
        struct input_event *ev; /* packet being/just read */
        LocalDevicePtr local = priv->local;
        static OsTimerPtr emulate3_timer = NULL;
        int btn = 0;  

        ev = &priv->ev;        
        if (priv->emulate3) {
                if ( (ev->value==1) && (emulate3_timer==NULL) )
                        emulate3_timer = TimerSet(emulate3_timer, 0,
                                                  priv->emulate3_timeout,
                                                  emulate3Timer,
                                                  local);

                if ( (ev->value == 1) && (ev->code == BTN_LEFT) )
                        priv->touch_flags |= LB_STAT;
                if ( (ev->value == 1) && (ev->code == BTN_RIGHT) )
                        priv->touch_flags |= RB_STAT;

                if ( (ev->value == 0) && 
                     (priv->touch_flags & RB_STAT) && 
                     (priv->touch_flags & LB_STAT) ) {
                        DBGOUT(2, "Middle Release\n");
                        priv->touch_flags &= ~LB_STAT;
                        priv->touch_flags &= ~RB_STAT;
                        btn = 2;                  
                }
                if ( (ev->value == 0) && (ev->code == BTN_LEFT) &&
                     (priv->touch_flags & LB_STAT) ) {
                        DBGOUT(2, "Left Release\n");
                        priv->touch_flags &= ~LB_STAT;
                        btn = 1;
                }
                if ( (ev->value == 0) && (ev->code == BTN_RIGHT) &&
                     (priv->touch_flags & RB_STAT) ) {
                        DBGOUT(2, "Right Release\n");
                        priv->touch_flags &= ~RB_STAT;
                        btn = 3;
                }                
                if (ev->value==0) {
                        TimerFree(emulate3_timer);
                        emulate3_timer=NULL;
                        priv->emulate3_timer_expired = FALSE;
                        xf86PostButtonEvent(local->dev, TRUE,
                                            btn, ev->value, 0, 2, 
                                            priv->cur_x, 
                                            priv->cur_y);
                }
                
        } else {
                if (ev->code == BTN_LEFT) {
                        xf86PostButtonEvent(local->dev, TRUE,
                                            1, ev->value, 0, 2, 
                                            priv->cur_x, 
                                            priv->cur_y);
                }


                if (ev->code == BTN_RIGHT) {
                        xf86PostButtonEvent (local->dev, TRUE,
                                             3, ev->value, 0, 2,
                                             priv->cur_x, 
                                             priv->cur_y);
                }
        }
}




static Bool
DeviceControl (DeviceIntPtr dev,
               int mode)
{
        Bool        RetValue;

        switch (mode)
        {
        case DEVICE_INIT:
                DeviceInit (dev);
                RetValue = Success;
                break;
        case DEVICE_ON:
                RetValue = DeviceOn( dev );
                break;
        case DEVICE_OFF:
        case DEVICE_CLOSE:
                RetValue = DeviceOff( dev );
                break;
        default:
                RetValue = BadValue;
        }

        return( RetValue );
}




static Bool
DeviceOn (DeviceIntPtr dev)
{
        LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
        EVTouchPrivatePtr priv = (EVTouchPrivatePtr) (local->private);
        
        local->fd = xf86OpenSerial(local->options);

        DBGOUT(2, "Device ON\n");

        if (local->fd == -1)
        {
                xf86Msg(X_WARNING, "%s: cannot open input device\n", local->name);
                return (!Success);
        }

        priv->buffer = XisbNew(local->fd, 64);


        if (!priv->buffer) 
        {
                xf86CloseSerial(local->fd);
                local->fd = -1;
                return (!Success);
        }
        xf86FlushInput(local->fd);

#ifndef XFREE86_V4
        xf86AddEnabledDevice(local);
#else
        AddEnabledDevice(local->fd);
#endif


        dev->public.on = TRUE;

        return (Success);
}




static Bool
DeviceOff (DeviceIntPtr dev)
{
        LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
        EVTouchPrivatePtr priv = (EVTouchPrivatePtr) (local->private);

        DBGOUT(2, "Device OFF\n");

        if (local->fd != -1)
        { 
                xf86RemoveEnabledDevice (local);
                if (priv->buffer)
                {
                        XisbFree(priv->buffer);
                        priv->buffer = NULL;
                }
                xf86CloseSerial(local->fd);
        }

        xf86RemoveEnabledDevice (local);
        dev->public.on = FALSE;

        return (Success);
}




static Bool
DeviceInit (DeviceIntPtr dev)
{
        LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
        EVTouchPrivatePtr priv = (EVTouchPrivatePtr) (local->private);
        unsigned char map[] = {0, 1, 2, 3};
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 7
    Atom btn_labels[MSE_MAXBUTTONS] = {0};
    Atom axes_labels[2] = { 0, 0 };
#endif

   
        /* 
         * these have to be here instead of in the SetupProc, because when the
         * SetupProc is run at server startup, screenInfo is not setup yet
         */

        ScrnInfoPtr   pScrn = xf86Screens[priv->screen_num];

        priv->phys_width = pScrn->currentMode->HDisplay;  /* physical screen resolution */
        priv->phys_height = pScrn->currentMode->VDisplay;
        priv->screen_width  = pScrn->virtualX;     /* It's the virtual screen size ! */
        priv->screen_height = pScrn->virtualY;
        priv->pViewPort_X0  = &(pScrn->frameX0);   /* initialize the pointers to the viewport coords */
        if ( (priv->screen_width != priv->phys_width) ||
             (priv->screen_height != priv->phys_height) ) 
              priv->virtual = 1;
        else  
                priv->virtual = 0;

        priv->pViewPort_Y0  = &(pScrn->frameY0);
        priv->pViewPort_X1  = &(pScrn->frameX1);
        priv->pViewPort_Y1  = &(pScrn->frameY1);

        DBGOUT(2, "DeviceInit\n");
        DBGOUT(2, "Display X,Y: %d %d\n", priv->phys_width, priv->phys_height);
        DBGOUT(2, "Virtual X,Y: %d %d\n", priv->screen_width, priv->screen_height);
        DBGOUT(2, "DriverName, Rev.: %s %d\n", pScrn->driverName, pScrn->driverVersion);
        DBGOUT(2, "Viewport X0,Y0: %d %d\n", *priv->pViewPort_X0, *priv->pViewPort_Y0);
        DBGOUT(2, "Viewport X1,Y1: %d %d\n", *priv->pViewPort_X1, *priv->pViewPort_Y1);
        DBGOUT(2, "MaxValue H,V: %d %d\n", pScrn->maxHValue, pScrn->maxVValue);



        priv->screen_width = screenInfo.screens[priv->screen_num]->width;
        priv->screen_height = screenInfo.screens[priv->screen_num]->height;


        if (InitPtrFeedbackClassDeviceStruct((DeviceIntPtr) dev, 
                                             ControlProc) == FALSE) 
        {
                ErrorF("Unable to init ptr feedback for EVTouch\n");
                return !Success;
        }

        /* 
         * Device reports button press for 3 buttons.
         */
        /* FIXME: we should probably set the labels here */

	InitPointerDeviceStruct((DevicePtr)dev, map,
				3,
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 7
                                btn_labels,
#endif
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) == 0
				miPointerGetMotionEvents,
#elif GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 3
                                GetMotionHistory,
#endif
                                ControlProc,
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) == 0
				miPointerGetMotionBufferSize()
#else
                                GetMotionHistorySize(), 2
#endif
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 7
                                , axes_labels
#endif
                                );

        if (InitButtonClassDeviceStruct (dev, 3, 
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 7
            btn_labels, 
#endif
            map) == FALSE)
        {
                ErrorF("Unable to allocate EVTouch touchscreen ButtonClassDeviceStruct\n");
                return !Success;
        }

        if (InitFocusClassDeviceStruct(dev) == FALSE) {
                ErrorF("Unable to allocate EVTouch touchscreen FocusClassDeviceStruct\n");
                return !Success;
        }

        /* 
         * Device reports motions on 2 axes in absolute coordinates.
         * Axes min and max values are reported in raw coordinates.
         */
        if (InitValuatorClassDeviceStruct(dev, 2, 
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 7
                                          axes_labels,
#elif GET_ABI_MAJOR(ABI_XINPUT_VERSION) < 4
                                        xf86GetMotionEvents,
#endif
                                          local->history_size, Absolute) == FALSE)
        {
                ErrorF ("Unable to allocate EVTouch touchscreen ValuatorClassDeviceStruct\n");
                return !Success;
        }
        else
        {
                InitValuatorAxisStruct (dev, 0, 
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 7
                                        axes_labels[0], 
#endif
                                        priv->min_x, priv->max_x,
                                        1024,
                                        EV_AXIS_MIN_RES /* min_res */ ,
                                        EV_AXIS_MAX_RES /* max_res */ );
                InitValuatorAxisStruct (dev, 1, 
#if GET_ABI_MAJOR(ABI_XINPUT_VERSION) >= 7
                                        axes_labels[1], 
#endif
                                        priv->min_y, priv->max_y,
                                        1024,
                                        EV_AXIS_MIN_RES /* min_res */ ,
                                        EV_AXIS_MAX_RES /* max_res */ );
        }

        /* Initial position of pointer on screen: Centered */
        priv->cur_x=(priv->max_x - priv->min_x)/2;
        priv->cur_y=(priv->max_y - priv->min_y)/2;
        
        if (InitProximityClassDeviceStruct (dev) == FALSE)
        {
                ErrorF ("Unable to allocate EVTouch touchscreen ProximityClassDeviceStruct\n");
                return !Success;
        }

        /* 
         * Allocate the motion events buffer.
         */
        xf86MotionHistoryAllocate (local);


        return (Success);
}




static void
ReadInput (LocalDevicePtr local)
{
        struct input_event *ev; /* packet being/just read */

        EVTouchPrivatePtr priv = (EVTouchPrivatePtr) (local->private);


        /* 
         * set blocking to -1 on the first call because we know there is data to
         * read. Xisb automatically clears it after one successful read so that
         * succeeding reads are preceeded buy a select with a 0 timeout to prevent
         * read from blocking infinately.
         */
        XisbBlockDuration (priv->buffer, -1);
        while (EVTouchGetPacket (priv) == Success)
        {
                ev = &priv->ev;

                xf86XInputSetScreen (local, 
                                     priv->screen_num, 
                                     priv->cur_x, 
                                     priv->cur_y);
                
                xf86PostProximityEvent (local->dev, 1, 0, 2, 
                                        priv->cur_x, 
                                        priv->cur_y);


                priv->old_x = priv->cur_x;
                priv->old_y = priv->cur_y;
                switch (ev->type) {
                case EV_ABS:
                        EVTouchProcessAbs(priv);
                        break;
                case EV_REL:
                        EVTouchProcessRel(priv);
                        break;
                }

                DBGOUT( 2, "setting (x/y)=(%d/%d)\n", 
                        priv->cur_x, priv->cur_y);
                
                /* 
                 * Send events.
                 *
                 * We *must* generate a motion before a button change if pointer
                 * location has changed as DIX assumes this. This is why we always
                 * emit a motion, regardless of the kind of packet processed.
                 */
                xf86PostMotionEvent (local->dev, TRUE, 0, 2, 
                                     priv->cur_x, 
                                     priv->cur_y);

                if (ev->type == EV_KEY) 
                        EVTouchProcessKey(priv);
        }
}




static void
ControlProc(DeviceIntPtr device, PtrCtrl *ctrl)
{
        DBGOUT(2, "ControlProc\n");
}




static void
CloseProc (LocalDevicePtr local)
{
        xf86ErrorFVerb(2, "CLOSEPROC\n" );
}




static int
SwitchMode (ClientPtr client, DeviceIntPtr dev, int mode)
{
        LocalDevicePtr local = (LocalDevicePtr) dev->public.devicePrivate;
        EVTouchPrivatePtr priv = (EVTouchPrivatePtr) (local->private);


        if ((mode == TS_Raw) || (mode == TS_Scaled))
        {
                priv->reporting_mode = mode;
                return (Success);
        }
        else if ((mode == SendCoreEvents) || (mode == DontSendCoreEvents))
        {
                xf86XInputSetSendCoreEvents (local, (mode == SendCoreEvents));
                return (Success);
        }
        else
                return (!Success);
}




static Bool
ConvertProc ( LocalDevicePtr local,
              int first,
              int num,
              int v0,
              int v1,
              int v2,
              int v3,
              int v4,
              int v5,
              int *x,
              int *y )
{
        int max_x, max_y;
        EVTouchPrivatePtr priv = (EVTouchPrivatePtr) (local->private);  

        DBGOUT(2, "FIRST: v0=%d   v1=%d\n", v0, v1);

        v1 = v1 - priv->min_y;
        v0 = v0 - priv->min_x;
        
        max_x = priv->max_x - priv->min_x;
        max_y = priv->max_y - priv->min_y;

        v0 = ( ((float)v0/max_x)*priv->phys_width );
        v1 = ( priv->phys_height - ((float)v1/max_y)*priv->phys_height );

        DBGOUT(2, "FINAL: v0=%d   v1=%d\n", v0, v1);

        *x = v0;
        *y = v1;

        return (TRUE);
}




static Bool
QueryHardware (LocalDevicePtr local)
{
        xf86ErrorFVerb(2, "QUERY HARDWARE\n" );

        return Success;
}




static void
EVTouchNewPacket (EVTouchPrivatePtr priv)
{
        memset(&priv->ev, 0, sizeof(struct input_event));
        priv->packeti = 0;
        priv->binary_pkt = FALSE;
}




static unsigned char
EVTouchRead(EVTouchPrivatePtr priv)
{
        unsigned char c;
        XisbBlockDuration (priv->buffer, EV_TIMEOUT);
        c = XisbRead(priv->buffer);
        return (c);
}




static Bool
EVTouchGetPacket (EVTouchPrivatePtr priv)
{
        static int count = 0;
        int c;

        if (count==0) EVTouchNewPacket(priv);

        while ((c = XisbRead (priv->buffer)) >= 0) {

                ((char *)&priv->ev)[count] = c;
                count ++;


                if (sizeof(priv->ev) == count) {
                        count = 0;
                        EVTouchDumpPacketToLog(priv);
                        
                        return Success;
                }
        }
        return (!Success);
}



#ifdef _0_
static void
EVTouchPrintIdent (unsigned char *packet)
{
        xf86Msg( X_PROBED, " EVTouch Testdevice" );
}
#endif


