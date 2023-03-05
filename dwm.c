/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance.  Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag.	Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>
#include <xcb/res.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pango.h>
#include <pango/pango-font.h>
#include <pango/pangocairo.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */

/* macros */
#define BARSHOWN(m)             (m->by != -bh)
#define BUTTONMASK				(ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)			(mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)			((C->tags & C->mon->vs->tagset) || C->tags == C->mon->vs->tagset)
#define ISFLOATING(C)           ((!C->mon->vs->lt[selmon->vs->curlt]->arrange || C->isfloating))
#define LENGTH(X)				(sizeof X / sizeof X[0])
#ifndef MAX
#define MAX(A, B)				((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A, B)				((A) < (B) ? (A) : (B))
#endif
#define MOUSEMASK				(BUTTONMASK|PointerMotionMask)
#define WIDTH(X)				((X)->w + 2 * (X)->bw)
#define HEIGHT(X)				((X)->h + 2 * (X)->bw)
#define TAGMASK					((1 << numtags) - 1)
#define TAGSLENGTH              (numtags)
#define TEXTW(X)				(textnw(X, strlen(X)) + dc.font.height)

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10

#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_WINDOW_DEACTIVATE    2

#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

#define OPACITY_BYTES(o) ((double)o * (unsigned int)-1)

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast };		/* cursor */
enum { ColBorder, ColFG, ColBG, ColLast };				/* color */
	   
enum { NetSupported, NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation, NetSystemTrayOrientationHorz,
	   NetWMName, NetWMState, NetWMFullscreen, NetWMMaximized, NetActiveWindow, NetCloseWindow, NetWMWindowType,
	   NetWMWindowTypeDialog, NetWMWindowTypeDock, NetWMWindowTypeDesktop, NetWMWindowTypeNotification,
	   NetWMWindowTypeKDEOSD, NetWMWindowTypeKDEOVERRIDE, NetWMStateSkipTaskbar, NetWMDesktop, NetWMOpacity,
	   NetClientList, NetDesktopNames, NetDesktopViewport, NetDesktopGeometry, NetNumberOfDesktops, 
	   NetCurrentDesktop, NetLast }; /* EWMH atoms */
enum { Manager, Xembed, XembedInfo, MotifWMHints, XLast }; /* Xembed atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast };		 /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
	   ClkClientWin, ClkRootWin, ClkLast };				/* clicks */

#define CONFIG_HEAD
#include "config.h"
#undef CONFIG_HEAD

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
	KeySym keysym;
	char *shcmd;
} Arg;

typedef struct Button {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	Arg arg;
	struct Button *next;
} Button;

typedef struct {
	unsigned int mousebuttonfrom;
	unsigned int click;
	KeySym keysymfrom;
	KeySym keysymto;
	int modifier;
} Remap;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	Client *next;
	Bool isfixed, isfloating, isurgent, neverfocus, oldstate, noborder, nofocus, exfocus, isterminal, isosd, isoverride, picomfreeze, isbackwin;
	unsigned int isfullscreen;
	pid_t pid;
	Client *snext;
	Monitor *mon;
	Window win;
	double opacity;
	unsigned int actual_opacity;
	Bool rh;
	const Remap* remap;
};

typedef struct {
	int x, y, w, h;
	unsigned long norm[ColLast];
	unsigned long sel[ColLast];
	Drawable drawable;
	GC gc;

	PangoColor  pangonorm[ColLast];
	PangoColor  pangosel[ColLast];

	struct {
		cairo_t *context;
		cairo_surface_t *surface;
		PangoFontMap *fontmap;
		cairo_font_options_t *font_options;
		PangoContext *pangocontext;
		PangoLayout *layout;
		PangoFontDescription *fontdesc;
	} cairo;

	struct {
		int ascent;
		int descent;
		int height;
		int padding;
	} font;
} DC; /* draw context */

typedef struct Key {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	Arg arg;
	struct Key *next;
	char* pending;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
	const char *name;
	int borderpx;
	Bool showdock;
} Layout;

struct ViewStack;
typedef struct ViewStack ViewStack;
struct ViewStack {
	struct ViewStack* next;
	unsigned int tagset;
	const Layout *lt[2];
	Bool showbar;
	Bool showdock;
	int curlt;
	float mfact;
	unsigned int msplit;
	int ltaxis[3];
};

struct Monitor {
	char ltsymbol[16];
	int num;
	int by;				  /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	int wxo, wyo, who, wwo;
	Bool topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	Window clock;
	ViewStack *vs;
	Window backwin;
};


typedef struct Rule Rule;
struct Rule {
	char *class;
	char *instance;
	char *title;
	unsigned int tags;
	Bool isfloating;
    Bool iscenter;
	int isterminal;
	double istransparent;
	Bool nofocus;
	Bool exfocus;
	Bool noborder;
	Bool rh;
	int monitor;
	const Remap* remap;
	const Layout* preflayout;
	Bool istransient;
	Bool isfullscreen;
	int showdock;
	char* procname;
	Bool picomfreeze;
	Rule *next;
};

typedef struct {
	const char* process_name;
	unsigned int tags;
	Bool isfloating;
	double istransparent;
	Bool nofocus;
	Bool noborder;
	Bool rh;
	int monitor;
	const Remap* remap;
	const Layout* preflayout;
} TransientRule;

typedef struct Systray   Systray;
struct Systray {
	Window win;
	Client *icons;
};

/* function declarations */
static void applylastruleviews(Client *c);
static void applyrules(Client *c);
static Bool applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachabove(Client *c);
static void attachend(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void cleartags(Monitor *m);
static void clearurgent(Client *c);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static unsigned int counttiledclients (Monitor* m);
static void createclocks(void);
static Bool clientmatchesrule (Client *c, const char* class, const char* instance, Bool istransient, const char* wincmdline, const Rule *r);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static void die(const char *errstr, ...);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void drawsquare(Bool filled, Bool empty, Bool invert, unsigned long col[ColLast]);
static void drawtext(const char *text, unsigned long col[ColLast], Bool invert, Bool centre);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void setmonitorfocus(Monitor *m);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static Atom* getatomprops(Client *c, Atom prop, int* numatoms);
static unsigned long getcolor(const char *colstr, PangoColor *color);
static Client *getclientunderpt(int x, int y);
static Bool getrootptr(int *x, int *y);
static long getstate(Window w);
static unsigned int getsystraywidth();
static Bool gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, Bool focused);
static void grabkeys(Window window);
static void grabremap(Client *c, Bool focused);
static void initfont(const char *fontstr);
static Bool ismasterclient(Client *c);
static Bool istiled(Client *c);
static void keypress(XEvent *e);
static void keyrelease(XEvent *e);
static void killclient(const Arg *arg);
static void killclientimpl(Client *c);
static void killclocks(void);
static void maketagtext(char* text, int maxlength, int i);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static unsigned int intersecttags(Client *c, Monitor *m);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *c);
static void push(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resetprimarymonitor();
static void resize(Client *c, int x, int y, int w, int h, Bool interact);
static void resizebackwins();
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void resizerequest(XEvent *e);
static void restack(Monitor *m);
static void restackwindows();
static void restorebar(Monitor *m);
static void run(void);
static void scan(void);
static Bool sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void sendmon(Client *c, Monitor *m);
static void selectmon(Monitor* m);
static void setclientstate(Client *c, long state);
static void setdesktopnames(void);
static void setclientopacity(Client *c);
static void setfocus(Client *c);
static void setfullscreen(Client *c, Bool fullscreen);
static void monsetlayout(Monitor *m, const void* v);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void updateopacities(Monitor *m);
static void sendselkey(const Arg *arg);
static void setnumdesktops(void);
static void setup(void);
static void setviewport(void);
static void setgeometry();
static Bool shouldbeopaque(Client *c, Client *tiledsel);
static void showhide(Client *c);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static void systrayaddwindow (Window win);
static void spawnimpl(const Arg *arg, Bool waitdeath, Bool useshcmd);
static void spawnterm(const Arg *arg);
static Monitor *systraytomon(Monitor *m);
static void swap(Client *c1, Client *c2);
static void tabview(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static long tagsettonum (unsigned int tagset);
static int textnw(const char *text, unsigned int len);
static void tile(Monitor *);
static void togglebar(const Arg *arg);
static void toggledock(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglefoldtags(const Arg *arg);
static void toggletag(const Arg *arg);
static void togglevarilayout(const Arg *arg);
static void toggleview(const Arg *arg);
static void togglewindowgap(const Arg *arg);
static void unfocus(Client *c, Bool setfocus);
static void unmanage(Client *c, Bool destroyed);
static void unmapnotify(XEvent *e);
static void updateclientdesktop(Client *c);
static void updatecurrentdesktop(void);
static void updateborderswidth(Monitor *m);
static void updatecolors(const Arg *arg);
static void updategeom(void);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static void updatedockpos(Monitor *m);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatesystray(void);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static void updatewindowtype(Client *c);
static void updatetitle(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static void monview(Monitor* m, unsigned int ui);
static void monshowbar(Monitor* m, Bool show);
static void monshowdock(Monitor* m, Bool show);
static void window_opacity_set(Window win, unsigned int opacitybyte);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);
static void noop(const Arg *arg);
static void updatetagshortcuts();
static pid_t winpid(Window w);

/* variables */
static Systray *systray = NULL;
static const char broken[] = "broken";
static char stext[256];
static int screen;
static int sw, sh;			 /* X display screen geometry width, height */
static int bh, blw = 0;		 /* bar geometry */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ButtonRelease] = buttonrelease,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[KeyRelease] = keyrelease,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[ResizeRequest] = resizerequest,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast];
static Bool running = True;
static Cursor cursor[CurLast];
static Display *dpy;
static DC dc;
static Monitor *mons = NULL, *selmon = NULL;
static Window root;
static Window dockwin = 0;
static xcb_connection_t *xcon;
static Client* lastclient = NULL;
static Bool startup = True;
static Bool rotatingMons = False;
static unsigned int statuscommutator = 0;
static const Rule *lastruleapplied = NULL;

/* configuration, allows nested code to access above variables */
#include "config.h"

static char ooftraysbl[OOFTRAYLEN];
static int windowgap = defaultwindowgap;

Window getWindowParent (Window winId) {
	Window wroot, parent, *children = NULL;
	unsigned int num_children;

	if(XQueryTree(dpy, winId, &wroot, &parent, &children, &num_children)) {
		if (children)
			XFree((char *)children);
	}
	else {
		parent = None;
	}

	return parent;
}

Bool
clientmatchesrule (Client *c, const char* class, const char* instance, Bool istransient, const char* wincmdline, const Rule *r) {
	return (!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance))
		&& (!r->procname || wincmdline && strstr(wincmdline, r->procname))
		&& r->istransient == istransient;
}

void
applyclientrule (Client *c, const Rule *r, Bool istransient) {
	Monitor *m;

	c->isfloating = r->isfloating;
	c->isterminal = r->isterminal;
	if (r->nofocus)
		c->nofocus = True;
	else if (r->exfocus)
		c->exfocus = True;
	if (r->noborder)
		c->noborder = True;
	if (r->picomfreeze)
		c->picomfreeze = True;
	c->tags |= r->tags;
	if (r->istransient != 1.)
		c->opacity = r->istransparent;
	c->rh = r->rh;
	if (r->remap != NULL)
		c->remap = r->remap;
	if (istransient)
		c->isfixed = True;
	if (r->isfullscreen)
		c->isfullscreen = 1;
    if (r->iscenter) {
        c->x = (c->mon->ww - c->w) / 2;
        c->y = (c->mon->wh - c->h) / 2;
    }
	for(m = mons; m && m->num != r->monitor; m = m->next);
	if(m)
		c->mon = m;
}

char* getwincmdline (Client* c) {
	pid_t pid = winpid(c->win);

	char procfile[1024];
	char* cmdline = calloc(1024, sizeof(char));
	int nread = 0;

	if (pid != 0) {
		snprintf(procfile, 1023, "/proc/%d/cmdline", pid);
	
		FILE* fd = fopen(procfile, "r");
		if (fd != NULL) {
			nread = fread(cmdline, sizeof(char), 1023, fd);
			fclose(fd);
		}
		else
		{
			fprintf(stderr, "proc file not found: %s\n", procfile);
		}
		cmdline[nread] = 0;
	}
	return cmdline;
}

unsigned int
numvisibleclients(Monitor* m) {
	unsigned int nc = 0;
	Client *c;

	for(c = m->clients; c; c = c->next)
		if(ISVISIBLE(c) && !c->nofocus && c->tags != TAGMASK)
			++nc;
	return nc;
}

/* function implementations */
void
applyrules(Client *c) {
	const char *class, *instance;
	char* wincmdline = NULL;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { 0 };
	Bool found = False;
	Bool istransient = False;
	unsigned int currenttagset = c->mon->vs->tagset;
	unsigned int nc = 0;

	lastruleapplied = NULL;
	/* rule matching */
	c->isfloating = c->tags = 0;
	c->rh = True;

	Window trans = None;
	if (XGetTransientForHint(dpy, c->win, &trans))
		istransient = (trans != None);

	wincmdline = getwincmdline(c);
	if(XGetClassHint(dpy, c->win, &ch) || wincmdline) {
		class = ch.res_class ? ch.res_class : broken;
		instance = ch.res_name ? ch.res_name : broken;
		i = 0;
		for (r = rules; r; r = r->next) {
			if (clientmatchesrule(c, class, instance, istransient, wincmdline, r))
			{
				applyclientrule(c, r, istransient);
				lastruleapplied = r;
				found = True;
			}
			++i;
		}
		if (clientmatchesrule(c, class, instance, istransient, wincmdline, &clockrule)) {
			applyclientrule(c, &clockrule, istransient);
			for(m = mons; m && m->clock; m = m->next);
			if (m) {
				m->clock = c->win;
				c->mon = m;
			}
		}
		if(ch.res_class)
			XFree(ch.res_class);
		if(ch.res_name)
			XFree(ch.res_name);
		if (wincmdline)
			free(wincmdline);
	}
	else
	{
		i = 0;
		for (r = rules; r; r = r->next) {
			if(r->title && strstr(c->name, r->title)) {
				applyclientrule(c, r, False);
				for(m = mons; m && m->num != r->monitor; m = m->next);
				if(m)
					c->mon = m;
				found = True;
				lastruleapplied = r;
			}
			++i;
		}
		if(!found) {
			r = &defaultrule;
			applyclientrule(c, r, False);
			for(m = mons; m && m->num != r->monitor; m = m->next);
			if(m)
				c->mon = m;
		}
	}
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->vs->tagset;
	if (c->mon) {
		if (!startup) {
			if (intersecttags(c, c->mon) != 0 && !c->nofocus && c->tags != TAGMASK) {
				if (c->tags == vtag)
					viewstackadd(c->mon, vtag, True);
				else if (c->isfloating)
					viewstackadd(c->mon, c->tags | currenttagset, True);
				else {
					nc = numvisibleclients(c->mon);
					viewstackadd(c->mon, c->tags, True);
					if (nc > 0)
						viewstackadd(c->mon, currenttagset, True);
				}
				updatecurrentdesktop();
			}
		}
	}
}

void
applylastruleviews(Client *c) {
	if(lastruleapplied) {
		if(lastruleapplied->preflayout)
			storestackviewlayout(c->mon, c->tags, lastruleapplied->preflayout, c->mon->vs->showdock);
		if(lastruleapplied->showdock >= 0)
			getviewstackof(c->mon, c->tags)->showdock = (lastruleapplied->showdock != 0);
		if(c->tags == c->mon->vs->tagset)
			arrange(c->mon);
		lastruleapplied = NULL;
	}
	if(!startup && !c->nofocus && (c->tags & c->mon->vs->tagset) == 0) {
		if(c->tags && c->tags != TAGMASK && c->tags != ~0)
			moveviewstacksecond(c->mon, c->tags);
		if((c->tags & c->mon->vs->tagset) == 0)
			drawbar(c->mon);
	}
}

Bool
applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact) {
	Bool baseismin;
	Monitor *m = c->mon;
	int worig = *w;
	int horig = *h;
	int n, nc;
	Client* c1;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if(interact) {
		if(*x >= sw)
			*x = sw - WIDTH(c);
		if(*y >= sh)
			*y = sh - HEIGHT(c);
		if(*x + *w + 2 * c->bw <= 0)
			*x = 0;
		if(*y + *h + 2 * c->bw <= 0)
			*y = 0;
	}
	else {
		if(*x >= m->mx + m->mw)
			*x = m->mx + m->mw - WIDTH(c);
		if(*y >= m->my + m->mh)
			*y = m->my + m->mh - HEIGHT(c);
		if(*x + *w + 2 * c->bw <= m->mx)
			*x = m->mx;
		if(*y + *h + 2 * c->bw <= m->my)
			*y = m->my;
	}
	if(*h < bh)
		*h = bh;
	if(*w < bh)
		*w = bh;
	if(c->rh || c->isfloating || !c->mon->vs->lt[c->mon->vs->curlt]->arrange) {
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if(!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if(c->mina > 0 && c->maxa > 0) {
			if(c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if(c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if(baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if(c->incw)
			*w -= *w % c->incw;
		if(c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if(c->maxw)
			*w = MIN(*w, c->maxw);
		if(c->maxh)
			*h = MIN(*h, c->maxh);
		if (!c->isfloating && m->vs->lt[m->vs->curlt]->arrange)
		{
			n = 0;
			nc = 0;
			if(m->vs->lt[m->vs->curlt]->arrange == &tile)
				for(c1 = nexttiled(m->clients); c1; c1 = nexttiled(c1->next), n++) {
					if(c1 == c)
						nc = n;
				}
			if(n == m->vs->msplit + 1 || nc < m->vs->msplit || m->vs->ltaxis[2] == 2)
				*x += (worig - *w) / 2;
			if(n == m->vs->msplit + 1 || nc < m->vs->msplit || m->vs->ltaxis[2] == 1)
				*y += (horig - *h) / 2;
		}
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m) {
	if(m)
		showhide(m->stack);
	else for(m = mons; m; m = m->next)
		showhide(m->stack);
	if(m)
		arrangemon(m);
	else for(m = mons; m; m = m->next)
		arrangemon(m);
	if (m)
		updateopacities(m);
	else for(m = mons; m; m = m->next)
		updateopacities(m);
}

void
arrangemon(Monitor *m) {
	Client *c;

	updateborderswidth(m);
	for (c = m->clients; c; c = c->next)
		updateclientdesktop(c);
	strncpy(m->ltsymbol, m->vs->lt[m->vs->curlt]->symbol, sizeof m->ltsymbol);
	if(m->vs->lt[m->vs->curlt]->arrange)
		m->vs->lt[m->vs->curlt]->arrange(m);
	restack(m);
}

void
attach(Client *c) {
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachabove(Client *c) {
	if(c->mon->sel == NULL || c->mon->sel == c->mon->clients || c->mon->sel->isfloating) {
		attach(c);
		return;
	}

	Client *at;
	for (at = c->mon->clients; at->next != c->mon->sel; at = at->next);
	c->next = at->next;
	at->next = c;
}

void
attachend(Client *c) {
	Client **pc = &c->mon->clients;

	while (*pc != NULL)
		pc = &(*pc)->next;
	c->next = NULL;
	*pc = c;
}

void
attachstack(Client *c) {
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
buttonpress(XEvent *e) {
	unsigned int i, x, click;
	Arg arg = {0};
	Button *button = buttons;
	Client *c, *cfocus;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;
	Bool sendevent = False, remapped = False;
	unsigned int occ = 0;
	char text[32];

	click = ClkRootWin;
	/* focus monitor if necessary */
	if((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, True);
		selectmon(m);
		focus(NULL);
	}
	cfocus = selmon ? selmon->sel : NULL;
	if(ev->window == selmon->barwin) {
		i = x = 0;
		for(c = m->clients; c; c = c->next)
			if(c->tags != TAGMASK && !c->nofocus && c->tags != TAGMASK)
				occ |= c->tags;
		do
			if(!foldtags || occ & 1 << i || m->vs->tagset & 1 << i) {
				maketagtext(text, 31, i);
				x += TEXTW(text);
			}
		while(ev->x >= x && ++i < numtags);
		if(i < numtags) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		}
		else if(ev->x < x + blw)
			click = ClkLtSymbol;
		else if(ev->x > selmon->ww - TEXTW(stext) - getsystraywidth())
			click = ClkStatusText;
		else
			click = ClkWinTitle;
	}
	else if((c = wintoclient(ev->window))) {
		focus(c);
		click = ClkClientWin;
        sendevent = True;
		cfocus = c;
	}
	if(cfocus && cfocus->remap)
		for(i = 0; cfocus->remap[i].keysymto; i++)
			if(click == cfocus->remap[i].click
					&& cfocus->remap[i].mousebuttonfrom
					&& cfocus->remap[i].mousebuttonfrom == ev->button) {
				remapped = True;
				sendKey(XKeysymToKeycode(dpy, cfocus->remap[i].keysymto), cfocus->remap[i].modifier);
			}
	if(!remapped) {
		while(button) {
			if(click == button->click && button->func && button->button == ev->button
			&& CLEANMASK(button->mask) == CLEANMASK(ev->state))
				button->func(click == ClkTagBar && button->arg.i == 0 ? &arg : &button->arg);
			button = button->next;
		}
		if (sendevent)
			XSendEvent(dpy, c->win, False, ButtonPressMask, e);
	}
}

void
buttonrelease(XEvent *e) {
	unsigned int i, click;
	Client *c, *cfocus = selmon ? selmon->sel : NULL;
	XButtonReleasedEvent *ev = &e->xbutton;
	Bool sendevent = False;

	click = ClkRootWin;
	if((c = wintoclient(ev->window))) {
		click = ClkClientWin;
		sendevent = True;
		cfocus = c;
	}
	if(cfocus && cfocus->remap)
		for(i = 0; cfocus->remap[i].keysymto; i++)
			if(click == cfocus->remap[i].click
					&& cfocus->remap[i].mousebuttonfrom
					&& cfocus->remap[i].mousebuttonfrom == ev->button)
				sendevent = False;
	if(sendevent)
		XSendEvent(dpy, c->win, False, ButtonReleaseMask, e);
}

void
checkotherwm(void) {
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanrule (Rule *rule) {
	free(rule->class);
	free(rule->instance);
	free(rule->title);
	free(rule->procname);
	free(rule);
}

void
cleanrules(void) {
	Rule *r;

	while (rules) {
		r = rules;
		rules = r->next;
		cleanrule(r);
	}
}

void
cleankey(Key* key) {
	if(key->func == &spawn)
		free(key->arg.shcmd);
	free(key);
}

void
cleankeys(void) {
	Key *k;

	while (keys) {
		k = keys;
		keys = k->next;
		cleankey(k);
	}
}

void
cleanbutton(Button *button) {
	if(button->func == &spawn)
		free(button->arg.shcmd);
	free(button);
}

void
cleanbuttons(void) {
	Button *b;

	while(buttons) {
		b = buttons;
		buttons = b->next;
		cleanbutton(b);
	}
}

void
cleantags(void) {
	int i;

	for (i = 0; tags[i]; ++i)
		free(tags[i]);
	free(tags);
}

void
cleanupconfig() {
	int i;

	cleanrules();
	cleankeys();
	cleanbuttons();
	cleantags();
	free(font);
	free(terminal[0]);
	for(i = 0; i < LENGTH(tagkeys); ++i)
		if(tagkeys[i])
			free(tagkeys[i]);
}

void
cleanup(void) {
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	Monitor *m;

	view(&a);
	selmon->vs->lt[selmon->vs->curlt] = &foo;
	for(m = mons; m; m = m->next)
		while(m->stack)
			unmanage(m->stack, False);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	XFreePixmap(dpy, dc.drawable);
	XFreeGC(dpy, dc.gc);
	XFreeCursor(dpy, cursor[CurNormal]);
	XFreeCursor(dpy, cursor[CurResize]);
	XFreeCursor(dpy, cursor[CurMove]);
	while(mons)
		cleanupmon(mons);
	if(showsystray) {
		XUnmapWindow(dpy, systray->win);
		XDestroyWindow(dpy, systray->win);
		free(systray);
	}
	cleanupconfig();
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
}

void
cleanupmon(Monitor *mon) {
	Monitor *m;

	if(mon == mons)
		mons = mons->next;
	else {
		for(m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	cleanupviewstack(mon->vs);
	free(mon);
}

void
cleartags(Monitor *m){
	Client *c;
	unsigned int newtags = 0;
	unsigned int nc = 0;

	for(c = m->clients; c; c = c->next)
		if(ISVISIBLE(c) && !c->nofocus && c->tags != TAGMASK) {
			if (c->tags != TAGMASK)
				newtags |= (c->tags & m->vs->tagset);
			++nc;
		}
	if(newtags && newtags != m->vs->tagset) {
		monview(m, newtags);
	}
	if (nc == 0) {
		monsetlayout(m, &layouts[initlayout]);
		if (m->vs->tagset == vtag && !hasclientson(m, vtag)){
			monview(m, 0);
		}
	}
}

void
clearurgent(Client *c) {
	XWMHints *wmh;

	c->isurgent = False;
	if(!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags &= ~XUrgencyHint;
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
clientmessage(XEvent *e) {
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if(showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
		/* add systray icons */
		fprintf(stderr, "Creating systray icon client.\n");

		if(cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
			systrayaddwindow(cme->data.l[2]);
		}
		return;
	}
	if(!c)
		return;
	if(cme->message_type == netatom[NetWMState]) {
		if(cme->data.l[1] == netatom[NetWMFullscreen] || cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD	  */
						  || (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
	}
	else if(cme->message_type == netatom[NetActiveWindow]) {
		if(c != c->mon->sel && c->tags != vtag) {
			if(!ISVISIBLE(c) && c->tags != 0) {
				monview(c->mon, c->tags);
			}
			focus(c);
			arrange(c->mon);
		}
	}
	else if (cme->message_type == netatom[NetCloseWindow]) {
		killclientimpl(c);
	}
}

void
configure(Client *c) {
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e) {
	Client *c;
	Monitor *m;
	XConfigureEvent *ev = &e->xconfigure;

	if(ev->window == root) {
		sw = ev->width;
		sh = ev->height;
		if(dc.cairo.surface != NULL) {
			cairo_destroy(dc.cairo.context);
			cairo_surface_destroy(dc.cairo.surface);
		}
		if(dc.drawable != 0)
			XFreePixmap(dpy, dc.drawable);
		dc.drawable = XCreatePixmap(dpy, root, sw, bh, DefaultDepth(dpy, screen));
		dc.cairo.surface = cairo_xlib_surface_create(dpy, dc.drawable, DefaultVisual(dpy, screen), DisplayWidth(dpy, screen), bh);
		dc.cairo.context = cairo_create(dc.cairo.surface);
		updategeom();
		updatebars();
		for(m = mons; m; m = m->next) {
			for(c = m->clients; c; c = c->next)
				if(c->isfullscreen)
					resizeclient(c, m->mx, m->my, m->mw, m->mh);
			resizebarwin(m);
		}
		focus(NULL);
		arrange(NULL);
		restackwindows();
		killclocks();
		createclocks();
	}
}

void
configurerequest(XEvent *e) {
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if((c = wintoclient(ev->window))) {
		if(ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if(c->isfloating || !selmon->vs->lt[selmon->vs->curlt]->arrange) {
			m = c->mon;
			if(ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if(ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if(ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if(ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if((c->x + c->w) > m->mx + m->mw && c->isfloating) {
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			}
			if((c->y + c->h) > m->my + m->mh && c->isfloating) {
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			}
			if((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if(ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		}
		else
			configure(c);
	}
	else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

unsigned int
counttiledclients (Monitor* m) {
	unsigned int n;
	int ns = m->vs->msplit;
	Bool ismonocle = m->vs->lt[m->vs->curlt]->arrange == &monocle;
	Bool isvarimono = m->vs->lt[m->vs->curlt]->arrange == varimono;
	Client* c;

	if(ismonocle)
		return nexttiled(m->clients) ? 1 : 0;
	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), ++n);
	if(isvarimono) {
		if(ns > n)
			ns = n;
		return ns;
	}
	return n;
}

Monitor *
createmon(void) {
	Monitor *m;

	if(!(m = (Monitor *)calloc(1, sizeof(Monitor))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Monitor));
	m->vs = createviewstack(NULL, 1);
	m->topbar = topbar;
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);

	return m;
}

void
destroynotify(XEvent *e) {
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if((c = wintoclient(ev->window)))
		unmanage(c, True);
	else if(dockwin && ev->window == dockwin)
		dockwin = 0;
	else if((c = wintosystrayicon(ev->window))) {
		removesystrayicon(c);
		resizebarwin(selmon);
		updatesystray();
	}
}

void
detach(Client *c) {
	Client **tc;

	for(tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c) {
	Client **tc, *t;

	for(tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if(c == c->mon->sel) {
		for(t = c->mon->stack; t && (!ISVISIBLE(t) || t->nofocus); t = t->snext);
		c->mon->sel = t;
	}
}

void
die(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

Monitor *
dirtomon(int dir) {
	Monitor *m = NULL;

	if(dir > 0) {
		if(!(m = selmon->next))
			m = mons;
	}
	else if(selmon == mons)
		for(m = mons; m->next; m = m->next);
	else
		for(m = mons; m->next != selmon; m = m->next);
	return m;
}

void
drawbar(Monitor *m) {
	int x;
	unsigned int i, occ = 0, urg = 0;
	unsigned long *col;
	Client *c;
	Bool hasfullscreenv = False;
	char text[32];

	resizebarwin(m);
	if(showsystray && m == systraytomon(m)) {
		m->wwo -= getsystraywidth();
	}
	for(c = m->clients; c; c = c->next) {
		if(c->tags != TAGMASK && !c->nofocus && c->tags != TAGMASK)
			occ |= c->tags;
		if(c->isurgent && c->tags != TAGMASK)
			urg |= c->tags;
	}
	if ((occ & vtag) && statuscommutator && !foldtags) {
		for(c = m->clients; !hasfullscreenv && c; c = c->next)
			if (c->tags & vtag && c->isfullscreen)
				hasfullscreenv = True;
		if (hasfullscreenv)
			occ &= ~vtag;
	}
	dc.x = 0;
	for(i = 0; i < numtags; i++) {
		if(!foldtags || occ & 1 << i || m->vs->tagset & 1 << i) {
			col = m->vs->tagset & 1 << i ? dc.sel : dc.norm;
			maketagtext(text, 31, i);
			dc.w = TEXTW(text);
			drawtext(text, col, urg & 1 << i, False);
			if(!foldtags)
				drawsquare(m == selmon && selmon->sel && !selmon->sel->nofocus && selmon->sel->tags != TAGMASK && selmon->sel->tags & 1 << i || (1 << i) == vtag && hasfullscreenv,
						occ & 1 << i, urg & 1 << i, col);
			else if (vtag & 1 << i && occ & 1 << i)
				drawsquare(statuscommutator, 1, 0, col);
			dc.x += dc.w;
		}
	}
	dc.w = blw = TEXTW(m->ltsymbol);
	drawtext(m->ltsymbol, dc.norm, False, False);
	dc.x += dc.w;
	x = dc.x;
	if(m == selmon || statusallmonitor) {
		if (m != selmon) {
			dc.x = m->wwo - (int)getsystraywidth();
			dc.w = (int)getsystraywidth();
			drawtext(ooftraysbl, dc.norm, False, False);
		}
		dc.w = TEXTW(stext);
		dc.x = m->wwo - dc.w - (m == selmon ? 0 : getsystraywidth());
		if(dc.x < x) {
			dc.x = x;
			dc.w = m->wwo - x;
		}
		drawtext(stext, dc.norm, False, False);
	}
	else
		dc.x = m->wwo;
	if((dc.w = dc.x - x) > bh) {
		dc.x = x;
		col = m == selmon ? dc.sel : dc.norm;
		if(m->sel) {
			drawtext(m->sel->name, col, False, centretitle);
			drawsquare(m->sel->isfixed, m->sel->isfloating, False, col);
		}
		else {
			drawtext(NULL, col, False, False);
		}
	}
	if(showsystray && m == systraytomon(m)) {
		m->wwo += getsystraywidth();
	}
	XCopyArea(dpy, dc.drawable, m->barwin, dc.gc, 0, 0, m->wwo, bh, 0, 0);
	XSync(dpy, False);
}

void
drawbars(void) {
	Monitor *m;

	for(m = mons; m; m = m->next)
		drawbar(m);
	updatesystray();
}

void
drawsquare(Bool filled, Bool empty, Bool invert, unsigned long col[ColLast]) {
	int x;
	XGCValues gcv;
	XRectangle r = { dc.x, dc.y, dc.w, dc.h };

	gcv.foreground = col[invert ? ColBG : ColFG];
	XChangeGC(dpy, dc.gc, GCForeground, &gcv);
	x = (dc.font.ascent + dc.font.descent + 2) / 4;
	r.x = dc.x + 1;
	r.y = dc.y + 1;
	if(filled) {
		r.width = r.height = x + 1;
		XFillRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	}
	else if(empty) {
		r.width = r.height = x;
		XDrawRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	}
}

void
drawtext(const char *text, unsigned long col[ColLast], Bool invert, Bool centre) {
	char buf[256];
	int i, x, y, w, h, len, olen;
	XRectangle r = { dc.x, dc.y, dc.w, dc.h };
	PangoRectangle pr;
	PangoColor *color = (col == dc.norm ? dc.pangonorm : dc.pangosel) + (invert ? ColBG : ColFG);

	XSetForeground(dpy, dc.gc, col[invert ? ColFG : ColBG]);
	XFillRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	if(!text)
		return;
	olen = strlen(text);
	h = dc.font.ascent + dc.font.descent;
	/* shorten text if necessary */
	len = MIN(olen, sizeof(buf));
	memcpy(buf, text, len);
	while(len) {
		if ((len < 4 || (buf[len - 4] & (char)0x80) == 0) && textnw(buf, len) <= dc.w - h)
			break;
		--len;
	}
	if(!len)
		return;
	if(len < olen)
		for (i = len; i && i > len - MIN(olen - len, 3); buf[--i] = '.');
	pango_layout_set_text(dc.cairo.layout, buf, len);
	pango_layout_get_extents(dc.cairo.layout, 0, &pr);
	if(centre) {
		w = pr.width / PANGO_SCALE;
		if(w < dc.w)
			x = dc.x + (dc.w - w) / 2;
	}
	else
		x = dc.x + (h / 2);
	y = dc.y + (dc.h / 2) - (pr.height / PANGO_SCALE / 2);
	cairo_set_source_rgba (dc.cairo.context,
			 color->red / 65535.,
			 color->green / 65535.,
			 color->blue / 65535.,
			 1.);
	cairo_move_to(dc.cairo.context, x, y);
	pango_cairo_show_layout(dc.cairo.context, dc.cairo.layout);
}

void
enternotify(XEvent *e) {
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	if(c && c->actual_opacity == 0)
		return;
	if((m = wintomon(ev->window)) && m != selmon && ev->window != root) {
		unfocus(selmon->sel, True);
		selectmon(m);
	}
	else if(c == selmon->sel || c == NULL || selmon->sel && selmon->sel->isoverride)
		return;
	focus(c);
}

void
expose(XEvent *e) {
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if(ev->count == 0 && (m = wintomon(ev->window))) {
		drawbar(m);
		if(m == selmon)
			updatesystray();
	}
}

void
window_opacity_set(Window win, unsigned int opacitybyte) {
	XChangeProperty(dpy, win, netatom[NetWMOpacity], XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &opacitybyte, 1L);
	XSync(dpy, False);
}

void
setclientopacity(Client *c) {
	c->actual_opacity = OPACITY_BYTES(c->opacity);
	window_opacity_set(c->win, c->actual_opacity);
}

void
focus(Client *c) {
	Client *inc = c;
	Client *oc;
	Monitor *m;

	for(m = mons; m; m = m->next)
		for(oc = m->clients; oc; oc = oc->next)
			if (oc->exfocus && ISVISIBLE(oc))
				c = oc;
	if(!c || !ISVISIBLE(c))
		for(c = selmon->stack; c && (!ISVISIBLE(c) || c->tags == TAGMASK); c = c->snext);
	/* was if(selmon->sel) */
	if(selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, False);
	if(c) {
		if(c->isurgent)
			clearurgent(c);
		if(!ISFLOATING(c)) {
			detachstack(c);
			attachstack(c);
		}
	}
	while (!inc && c && c->nofocus)
		c = c->next;
	if (c && (!ISVISIBLE(c) || c->nofocus))
		c = NULL;
	if (c) {
		grabbuttons(c, True);
		XSetWindowBorder(dpy, c->win, dc.sel[ColBorder]);
		setfocus(c);
		c->mon->sel = c;
		restack(c->mon);
		updateopacities(c->mon);
	}
	else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
		if(selmon)
			restack(selmon);
	}
	drawbars();
	XSync(dpy, False);
}

void
focusin(XEvent *e) { /* there are some broken focus acquiring clients */
	XFocusChangeEvent *ev = &e->xfocus;

	if(selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
focusmon(const Arg *arg) {
	Monitor *m;

	if(!mons->next)
		return;
	if((m = dirtomon(arg->i)) == selmon)
		return;
	setmonitorfocus(m);
}

void
setmonitorfocus(Monitor* m) {
	if (selmon != m) {
		unfocus(selmon->sel, False);
		selectmon(m);
		focus(NULL);
		if (selmon->sel)
			centerMouseInWindow(selmon->sel);
		else
			centerMouseInMonitor(selmon);
	}
}

void
focusstack(const Arg *arg) {
	Client *c = NULL, *i;

	if(!selmon->sel)
		return;
	if(arg->i > 0) {
		c = selmon->sel->next;
		while (c) {
			if (ISVISIBLE(c) && !c->nofocus && c->tags != TAGMASK)
				break;
			c = c->next;
		}
		if(!c) {
			c = selmon->clients;
			while (c != selmon->sel) {
				if (ISVISIBLE(c) && !c->nofocus && c->tags != TAGMASK)
					break;
				c = c->next;
			}
		}
	}
	else {
		for(i = selmon->clients; i && i->next && i != selmon->sel; i = i->next)
			if(ISVISIBLE(i) && !i->nofocus && i->tags != TAGMASK)
				c = i;
		if(!c)
			for(; i; i = i->next)
				if(ISVISIBLE(i) && !i->nofocus && i->tags != TAGMASK)
					c = i;
	}
	if(c) {
		focus(c);
		restack(selmon);
	}
}

Client *
getclientunderpt(int x, int y) {
	Client *c;

	for(c = nexttiled(selmon->clients); c; c = nexttiled(c->next))
		if(x >= c->x && x < c->x+c->w && y >= c->y && y < c->y+c->h)
			return c;
	return 0;
}

Atom
getatomprop(Client *c, Atom prop) {
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;
	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	Atom req = XA_ATOM;
	if(prop == xatom[XembedInfo])
		req = xatom[XembedInfo];

	if(XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
						  &da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		if(da == xatom[XembedInfo] && dl == 2)
			atom = ((Atom *)p)[1];
		XFree(p);
	}
	return atom;
}

Atom*
getatomprops(Client *c, Atom prop, int* numatoms) {
	int di;
	unsigned long dl, ni;
	unsigned char *p = NULL;
	Atom da, *atoms = NULL;
	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	Atom req = XA_ATOM;

	if(XGetWindowProperty(dpy, c->win, prop, 0L, sizeof(Atom), False, req,
						  &da, &di, &ni, &dl, &p) == Success && p) {
		atoms = (Atom*)p;
		*numatoms = ni;
	}
	else
		*numatoms = 0;
	return atoms;
}

unsigned long
getcolor(const char *colstr, PangoColor *pangocolor) {
  int ret = pango_color_parse(pangocolor, colstr);
  Colormap cmap = DefaultColormap(dpy, screen);
  XColor xcolor;

  if(!ret)
	  fprintf(stderr, "failed to parse pangocolor %s\n", colstr);
  if(!XAllocNamedColor(dpy, cmap, colstr, &xcolor, &xcolor))
	  die("error, cannot allocate xcolor '%s'\n", colstr);
  return xcolor.pixel;
}

Bool
getrootptr(int *x, int *y) {
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w) {
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if(XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
						  &real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if(n != 0)
		result = *p;
	XFree(p);
	return result;
}

unsigned int
getsystraywidth() {
	unsigned int w = 0;
	Client *i;
	if(showsystray)
		for(i = systray->icons; i; w += i->w + systrayspacing, i = i->next) ;

	return w ? w + systrayspacing : 1;
}

Bool
gettextprop(Window w, Atom atom, char *text, unsigned int size) {
	char **list = NULL;
	int n;
	XTextProperty name;

	if(!text || size == 0)
		return False;
	text[0] = '\0';
	XGetTextProperty(dpy, w, &name, atom);
	if(!name.nitems)
		return False;
	if(name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else {
		if(XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return True;
}

void
grabremap(Client *c, Bool manage) {
	int i, j;
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
	KeyCode code;

	grabkeys(c->win);
	if (c && c->remap && c->win)
		for(i = 0; c->remap[i].keysymto; ++i)
			if(c->remap[i].click == ClkClientWin)
				for(j = 0; j < LENGTH(modifiers); j++)
					if (c->remap[i].keysymfrom && (code = XKeysymToKeycode(dpy, c->remap[i].keysymfrom))) {
						if (manage) {
							XGrabKey(dpy, code, modifiers[j], c->win,
									True, GrabModeSync, GrabModeAsync);
						}
						else {
							XUngrabKey(dpy, code, modifiers[j], c->win);
						}
					}
}

void
grabbuttons(Client *c, Bool focused) {
	unsigned int i, j;
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
	Button *button = buttons;

	updatenumlockmask();
	XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
	if(focused) {
		while(button) {
			if(button->click == ClkClientWin)
				for(j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, button->button,
							button->mask | modifiers[j],
							c->win, False, BUTTONMASK,
							GrabModeAsync, GrabModeSync, None, None);
			button = button->next;
		}
		if (c->remap)
			for(i = 0; c->remap[i].keysymto; i++)
				if(c->remap[i].click == ClkClientWin)
					for(j = 0; j < LENGTH(modifiers); j++)
						if (c->remap[i].mousebuttonfrom)
							XGrabButton(dpy, c->remap[i].mousebuttonfrom,
									modifiers[j],
									c->win, True, BUTTONMASK,
									GrabModeAsync, GrabModeSync, None, CurrentTime);
	}
	else
		XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
}

void
grabkeys(Window window) {
	unsigned int i, j;
	unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
	KeyCode code;
	Key *key = keys;

	updatenumlockmask();
	XUngrabKey(dpy, AnyKey, AnyModifier, window);
	while(key) {
		if((code = XKeysymToKeycode(dpy, key->keysym)))
			for(j = 0; j < LENGTH(modifiers); j++)
				XGrabKey(dpy, code, key->mod | modifiers[j], window,
						True, GrabModeAsync, GrabModeAsync);
		key = key->next;
	}
	for(i = 0; i < LENGTH(modkeysyms); ++i)
		if((code = XKeysymToKeycode(dpy, modkeysyms[i])))
			XGrabKey(dpy, code, AnyModifier, window,
				True, GrabModeAsync, GrabModeAsync);
}

void
initfont(const char *fontstr) {
	PangoFontMetrics *metrics;
	int dpi = (int)((double)DisplayWidth(dpy, screen)) / (((double)DisplayWidthMM(dpy, screen)) / 25.4);

	dc.cairo.fontmap = pango_cairo_font_map_new();
	pango_cairo_font_map_set_resolution(PANGO_CAIRO_FONT_MAP (dc.cairo.fontmap), dpi);
	dc.cairo.font_options = cairo_font_options_create();
	dc.cairo.pangocontext = pango_font_map_create_context(dc.cairo.fontmap);
	dc.cairo.fontdesc = pango_font_description_from_string(fontstr);

	metrics = pango_context_get_metrics(dc.cairo.pangocontext, dc.cairo.fontdesc, pango_language_from_string(setlocale(LC_CTYPE, "")));
	dc.font.ascent = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
	dc.font.descent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;

	pango_font_metrics_unref(metrics);

	dc.cairo.layout = pango_layout_new(dc.cairo.pangocontext);
	pango_layout_set_font_description(dc.cairo.layout, dc.cairo.fontdesc);
	pango_cairo_context_set_font_options(dc.cairo.pangocontext, dc.cairo.font_options);

	dc.font.height = dc.font.ascent + dc.font.descent;
}

Bool
ismasterclient(Client *c) {
	int ns = c->mon->vs->msplit;
	Client *o;
	int i = -1, n;

	if(c->mon->vs->lt[selmon->vs->curlt]->arrange == varimono) {
		for(n = 0, o = nexttiled(c->mon->clients); o; o = nexttiled(o->next), ++n)
			if(o == c)
				i = n;
		if(ns > n)
			ns = n;
		if(0 <= i && i <= n - ns)
			return True;
	}
	return False;
}

Bool
istiled(Client *c) {
	return !c->isfloating && ISVISIBLE(c);
}

#ifdef XINERAMA
static Bool
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info) {
	while (n--)
		/* treat origin (x, y) as fixpoint for uniqueness only, first screen wins */
		if(unique[n].x_org == info->x_org && unique[n].y_org == info->y_org)
			return False;
	return True;
}
#endif /* XINERAMA */

void
keypress(XEvent *e) {
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;
	Key *key = keys;

	updatetagshortcuts();
	ev = &e->xkey;
	keysym = XkbKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0, 0);
	while(key) {
		if(keysym == key->keysym
		&& CLEANMASK(key->mod) == CLEANMASK(ev->state)
		&& key->func) {
			key->func(&(key->arg));
		}
		key = key->next;
	}
	if (selmon->sel && selmon->sel->remap) {
		for(i = 0; selmon->sel->remap[i].keysymto; ++i)
			if (selmon->sel->remap[i].keysymfrom && selmon->sel->remap[i].keysymfrom == keysym)
				sendKey(XKeysymToKeycode(dpy, selmon->sel->remap[i].keysymto), selmon->sel->remap[i].modifier);
	}
}

void
keyrelease(XEvent *e) {
	updatetagshortcuts();
}

void
killclient(const Arg *arg) {
	if(!selmon->sel)
		return;
	killclientimpl(selmon->sel);
}

void
killclientimpl(Client *c) {
	if(c != selmon->sel || !sendevent(c->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, c->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
maketagtext(char* text, int maxlength, int i) {
	if(showtagshortcuts) {
		if(tagkeys[i])
			snprintf(text, maxlength, "%s:%s", tagkeys[i], tags[i]);
		else
			snprintf(text, maxlength, "%s", tags[i]);
	}
	else
		snprintf(text, maxlength, "%s", tags[i]);
}

void
manage(Window w, XWindowAttributes *wa) {
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;
	int bpx = 0;

	if(!(c = calloc(1, sizeof(Client))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Client));
	c->win = w;
	c->opacity = 1.;
	c->pid = winpid(w);
	updatetitle(c);
	/* geometry */
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	if(XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
		applyrules(c);
	}
	else {
		c->mon = selmon;
		applyrules(c);
	}
    if (c->x == 0 && c->y == 0) {
        c->x = c->oldx = wa->x + c->mon->wxo;
        c->y = c->oldy = wa->y + c->mon->wyo;
    }
	c->oldbw = wa->border_width;
	if(c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
		c->x = c->mon->mx + c->mon->mw - WIDTH(c);
	if(c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
		c->y = c->mon->my + c->mon->mh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx)
				&& (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);
	ViewStack* vs = getviewstackof(c->mon, c->tags);
	if (vs) {
		bpx = vs->lt[vs->curlt]->borderpx;
		if (vs->lt[vs->curlt]->arrange == &monocle || !hasclientson(c->mon, c->tags) || c->noborder)
			bpx = 0;
	}
	c->bw = bpx;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, dc.norm[ColBorder]);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	if(c->isoverride && c->isosd)
		XSelectInput(dpy, w, FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	else
		XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, False);
	if(!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
    /*
	 *if(c->isfloating || !c->mon->vs->lt[selmon->vs->curlt]->arrange)
	 *    attach(c);
	 *else
     */
		attachabove(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
			(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	XMapWindow(dpy, c->win);
	setclientstate(c, NormalState);
	if (!c->isfloating)
		arrange(c->mon);
	else
		updateopacities(c->mon);
	grabremap(c, True);
	if(!c->nofocus && !c->isosd && c->tags != TAGMASK) {
		if (c->mon == selmon)
			focus(c);
	}
	if(c->nofocus)
		unmanage(c, False);
	else {
		if(c->tags & c->mon->vs->tagset)
			cleartags(c->mon);
		if (c->exfocus)
			focus(c);
		applylastruleviews(c);
	}
}

void
mappingnotify(XEvent *e) {
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if(ev->request == MappingKeyboard)
		grabkeys(root);
}

void
maprequest(XEvent *e) {
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;
	Client *i;
	if((i = wintosystrayicon(ev->window))) {
		sendevent(i->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
		resizebarwin(selmon);
		updatesystray();
	}

	if(!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
	if(wa.override_redirect)
		return;
	if(!wintoclient(ev->window))
		manage(ev->window, &wa);
}

unsigned int
intersecttags(Client *c, Monitor *m) {
	unsigned int intertags = (c->tags|m->vs->tagset);
	Client *cother;

	if (c->tags != TAGMASK)
		for(cother = m->clients; cother; cother = cother->next)
			if (cother != c && (cother->tags & intertags) != 0 && !cother->nofocus && cother->tags != TAGMASK)
				intertags &= ~(cother->tags & intertags);
	return c->tags == TAGMASK ? 0 : intertags;
}

void
updateopacities(Monitor *m) {
	Client *c;
	Client *tiledsel = NULL;

	for(c = m->stack; c && !tiledsel; c = c->snext)
		if(ISVISIBLE(c) && !ISFLOATING(c))
			tiledsel = c;
	for(c = m->clients; c; c = c->next) {
		if(shouldbeopaque(c, tiledsel))
			setclientopacity(c);
		else
		{
			c->actual_opacity = 0;
			window_opacity_set(c->win, 0);
		}
	}
}

void
sendselkey(const Arg *arg) {
	sendKey(XKeysymToKeycode(dpy, arg->keysym), 0);
}

void
monocle(Monitor *m) {
	unsigned int n = 0;
	Client *c;

	for(c = m->clients; c; c = c->next)
		if(ISVISIBLE(c) && !c->nofocus && c->tags != TAGMASK)
			n++;
	if(n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n - 1);
	for(c = nexttiled(m->clients); c; c = nexttiled(c->next))
		resize(c, m->wx - c->bw, m->wy - c->bw, m->ww, m->wh, False);
}

void
motionnotify(XEvent *e) {
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if(ev->window != root)
		return;
	if((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, True);
		selectmon(m);
		focus(NULL);
	}
	mon = m;
}

void
movemouse(const Arg *arg) {
	int x, y, ocx, ocy, nx, ny;
	Client *c, *c2;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if(!(c = selmon->sel))
		return;
	if(c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	if(ISFLOATING(c)) {
		detachstack(c);
		attachstack(c);
	}
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
	None, cursor[CurMove], CurrentTime) != GrabSuccess)
		return;
	if(!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			XSync(dpy, False);
			if(selmon->vs->lt[selmon->vs->curlt]->arrange && !c->isfloating) {
				/* move within tesselation */
				nx = ev.xmotion.x;
				ny = ev.xmotion.y;
				c2 = getclientunderpt(nx, ny);
				if(c2 && c!=c2) {
					swap(c, c2);
					arrange(selmon);
				}
			} else {
				/* move floating window */
				if ((ev.xmotion.time - lasttime) <= (1000 / 60))
					continue;
				lasttime = ev.xmotion.time;

				nx = ocx + (ev.xmotion.x - x);
				ny = ocy + (ev.xmotion.y - y);
				if (abs(selmon->wx - nx) < snap)
					nx = selmon->wx;
				else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
					nx = selmon->wx + selmon->ww - WIDTH(c);
				if (abs(selmon->wy - ny) < snap)
					ny = selmon->wy;
				else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
					ny = selmon->wy + selmon->wh - HEIGHT(c);
				resize(c, nx, ny, c->w, c->h, True);
			}
			break;
		}
	} while(ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selectmon(m);
		focus(NULL);
	}
}

Client *
nexttiled(Client *c) {
	for(; c && !istiled(c); c = c->next);
	return c;
}

void
pop(Client *c) {
	if(c->mon->vs->lt[selmon->vs->curlt]->arrange == varimono)
		shiftmastersplitimpl(-1);
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void
push(Client *c) {
	if(c->mon->vs->lt[selmon->vs->curlt]->arrange == varimono)
		shiftmastersplitimpl(1);
	detach(c);
	attachend(c);
	arrange(c->mon);
	focus(nexttiled(c->mon->clients));
}

void
updatetagshortcuts() {
	Bool modkeyPressed = False;
	XkbStateRec r;

	XkbGetState(dpy, XkbUseCoreKbd, &r);
	modkeyPressed = r.mods & tagkeysmod;
	if(modkeyPressed != showtagshortcuts) {
		showtagshortcuts = modkeyPressed;
		drawbars();
	}
}

void
propertynotify(XEvent *e) {
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if((c = wintosystrayicon(ev->window))) {
		if(ev->atom == XA_WM_NORMAL_HINTS) {
			updatesizehints(c);
			updatesystrayicongeom(c, c->w, c->h);
		}
		else
			updatesystrayiconstate(c, ev);
		resizebarwin(selmon);
		updatesystray();
	}
	if((ev->window == root) && (ev->atom == XA_WM_NAME)) {
		statuscommutator ^= 1;
		updatestatus();
	}
	else if(ev->state == PropertyDelete)
		return; /* ignore */
	else if((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if(!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
			   (c->isfloating = (wintoclient(trans)) != NULL)) {
				arrange(c->mon);
			}
			break;
		case XA_WM_NORMAL_HINTS:
			updatesizehints(c);
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if(ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if(c == c->mon->sel)
				drawbar(c->mon);
		}
		if(ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

void
quit(const Arg *arg) {
	running = False;
}

Monitor *
recttomon(int x, int y, int w, int h) {
	Monitor *m, *r = selmon;
	int a, area = 0;

	for(m = mons; m; m = m->next)
		if((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
removesystrayicon(Client *i) {
	Client **ii;

	if(!showsystray || !i)
		return;
	for(ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if(ii)
		*ii = i->next;
	free(i);
}

void
resetprimarymonitor(void) {
	const Arg arg = {.v = xrandrcmd };

	spawnimpl(&arg, False, False);
}


void
resize(Client *c, int x, int y, int w, int h, Bool interact) {
	Bool isfloating = (c->isfloating || !c->mon->vs->lt[c->mon->vs->curlt]->arrange);
	unsigned int nc;

	if (!isfloating) {
		nc = counttiledclients(c->mon);
		if(nc > 1) {
			x += windowgap / 2;
			y += windowgap / 2;
			w -= windowgap;
			h -= windowgap;
		}
	}
	applysizehints(c, &x, &y, &w, &h, interact);
	resizeclient(c, x, y, w, h);
}

void
resizebackwins() {
	Monitor *m, *om;
	Client *c;
	XWindowChanges wc;
	XConfigureEvent ce;

	for(m = mons; m; m = m->next)
		m->backwin = 0;
	for(m = mons; m; m = m->next)
		for(c = m->clients; c; c = c->next)
			if(c->isbackwin)
				for(om = mons; om; om = om->next)
					if(!om->backwin) {
						om->backwin = c->win;
						c->tags = 0;
						break;
					}
	wc.border_width = 0;
	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.border_width = 0;
	ce.above = None;
	ce.override_redirect = False;
	for(m = mons; m; m = m->next) {
		if(m->backwin) {
			wc.x = m->mx;
			wc.y = m->my;
			wc.width = m->mw;
			wc.height = m->mh;
			ce.event = m->backwin;
			ce.window = m->backwin;
			ce.x = m->mx;
			ce.y = m->my;
			ce.width = m->mw;
			ce.height = m->mh;
			XConfigureWindow(dpy, m->backwin, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
			XSendEvent(dpy, m->backwin, False, StructureNotifyMask, (XEvent *)&ce);
		}
	}
}
void
resizebarwin(Monitor *m) {
	unsigned int w = m->wwo;
	if(showsystray && m == systraytomon(m))
		w -= getsystraywidth();
	XMoveResizeWindow(dpy, m->barwin, m->wxo, m->by, w, bh);
}

void
resizeclient(Client *c, int x, int y, int w, int h) {
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void
resizemouse(const Arg *arg) {
	int ocx, ocy;
	int nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;

	if(!(c = selmon->sel))
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if(XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
					None, cursor[CurResize], CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			if(c->mon->wx + nw >= selmon->wx
					&& c->mon->wx + nw <= selmon->wx + selmon->ww
					&& c->mon->wy + nh >= selmon->wy
					&& c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if(!c->isfloating && selmon->vs->lt[selmon->vs->curlt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if(!selmon->vs->lt[selmon->vs->curlt]->arrange || c->isfloating)
				resize(c, c->x, c->y, nw, nh, True);
			break;
		}
	} while(ev.type != ButtonRelease && False == XCheckMaskEvent(dpy, ButtonReleaseMask, &ev));
	if(!selmon->vs->lt[selmon->vs->curlt]->arrange || c->isfloating)
		resize(c, c->x, c->y, nw, nh, True);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selectmon(m);
		focus(NULL);
	}
}

void
resizerequest(XEvent *e) {
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		resizebarwin(selmon);
		updatesystray();
	}
}

void
restackwindows() {
	Monitor *m;
	Client *c;
	Window *windows;
	int nwindows = 0;
	int w = 0;

	XGrabServer(dpy);
	resizebackwins();
	for(m = mons; m; m = m->next) {
		if(m->barwin)
			++nwindows;
		if(m->clock)
			++nwindows;
		for(c = m->stack; c; c = c->snext, ++nwindows);
	}
	if(dockwin)
		++nwindows;
	if(nwindows > 1) {
		windows = (Window *)calloc(nwindows, sizeof(Window));
		// notifications
		for(m = mons; m; m = m->next)
			for(c = m->stack; c && w < nwindows; c = c->snext)
				if(c->isosd)
					windows[w++] = c->win;
		// visible floating
		for(m = mons; m && w < nwindows; m = m->next)
			for(c = m->stack; c; c = c->snext)
				if(ISVISIBLE(c) && ISFLOATING(c) && !c->nofocus && !c->isfullscreen && !c->isosd && c->win != c->mon->backwin)
					windows[w++] = c->win;
		// fullscreen window
		for(m = mons; m; m = m->next)
			for(c = m->stack; c && w < nwindows; c = c->snext)
				if(ISVISIBLE(c) && !c->nofocus && c->isfullscreen && c->win != m->barwin && c->win != dockwin && !c->isosd && c->win != c->mon->backwin)
					windows[w++] = c->win;
		// bar
		for(m = mons; m && w < nwindows; m = m->next)
			if(m->barwin)
				windows[w++] = m->barwin;
		// dock
		if(dockwin)
			windows[w++] = dockwin;
		// visible tiled sel
		for(m = mons; m; m = m->next)
			for(c = m->stack; c && w < nwindows; c = c->snext)
				if(ISVISIBLE(c) && !ISFLOATING(c) && c == m->sel && !c->nofocus && !c->isfullscreen && !c->isosd && c->win != c->mon->backwin)
					windows[w++] = c->win;
		// visible tiled non-sel
		for(m = mons; m; m = m->next)
			for(c = m->stack; c && w < nwindows; c = c->snext)
				if(ISVISIBLE(c) && !ISFLOATING(c) && c != m->sel && !c->nofocus && !c->isfullscreen && !c->isosd && c->win != c->mon->backwin)
					windows[w++] = c->win;
		// nofocus
		for(m = mons; m && w < nwindows; m = m->next)
			for(c = m->stack; c; c = c->snext)
				if(ISVISIBLE(c) && c != m->sel && c->nofocus && !c->isosd)
					windows[w++] = c->win;
		// desktop window (plasmashell)
		for(m = mons; m && w < nwindows; m = m->next)
			if(m->backwin)
				windows[w++] = m->backwin;
		// clock
		for(m = mons; m && w < nwindows; m = m->next)
			if(m->clock)
				windows[w++] = m->clock;
		// non-visible windows (other views)
		for(m = mons; m; m = m->next)
			for(c = m->stack; c && w < nwindows; c = c->snext)
				if(!ISVISIBLE(c) && !c->isosd && c->win != c->mon->backwin)
					windows[w++] = c->win;
		XRestackWindows(dpy, windows, w);
		free(windows);
	}
	XSync(dpy, False);
	XUngrabServer(dpy);
}

void
restack(Monitor *m) {
	Client *c;

	drawbar(m);
	if(!m->sel) {
		for(c = m->clients; c; c = c->next)
			if(!c->nofocus && ISVISIBLE(c) && c->tags != TAGMASK)
				break;
		if (c && c->mon == selmon)
			focus(c);
		else if (!c)
			m->sel = c;
	}
	restackwindows();
}

void
restorebar(Monitor* m) {
	if (m->vs->showbar != BARSHOWN(m))
		monshowbar(m, m->vs->showbar);
}

void
run(void) {
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	while(running && !XNextEvent(dpy, &ev))
		if(ev.type < LASTEvent && handler[ev.type])
			handler[ev.type](&ev); /* call handler */
}

Bool
isdesktop(Window win) {
	Atom wtype;
	int i, di, numtypes;
	unsigned long dl, ni;
	Atom req = XA_ATOM;
	Atom da, *atoms = NULL;
	unsigned char *p = NULL;
	Bool winisdesktop = False;

	if(XGetWindowProperty(dpy, win, netatom[NetWMWindowType], 0L, sizeof(Atom), False, req,
						  &da, &di, &ni, &dl, &p) == Success && p) {
		atoms = (Atom*)p;
		numtypes = ni;
	}
	else
		numtypes = 0;
	for(i = 0; i < numtypes; ++i) {
		wtype = atoms[i];
		if(wtype == netatom[NetWMWindowTypeDesktop]) {
			winisdesktop = True;
			break;
		}
	}
	if(atoms != NULL)
		XFree(atoms);
	return winisdesktop;
}

void
scan(void) {
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if(XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for(i = 0; i < num; i++) {
			char name[256];
			if(!gettextprop(wins[i], netatom[NetWMName], name, sizeof name))
				gettextprop(wins[i], XA_WM_NAME, name, sizeof name);
			if(name[0] == '\0') /* hack to mark broken clients */
				strcpy(name, broken);
		}
		for(i = 0; i < num; i++) {
			char name[256];
			if(!gettextprop(wins[i], netatom[NetWMName], name, sizeof name))
				gettextprop(wins[i], XA_WM_NAME, name, sizeof name);
			if(name[0] == '\0') /* hack to mark broken clients */
				strcpy(name, broken);
			if(strcmp(name, "Desktop — Plasma") == 0) {
				fprintf(stderr, "break here\n");
			}
			if(!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if(wa.map_state == IsViewable || getstate(wins[i]) == IconicState || isdesktop(wins[i]))
				manage(wins[i], &wa);
		}
		for(i = 0; i < num; i++) { /* now the transients */
			if(!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if(XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if(wins)
			XFree(wins);
	}
}

void
sendmon(Client *c, Monitor *m) {
	int xo = 0, yo = 0;
	ViewStack *vs;

	if(c->mon == m)
		return;
	unfocus(c, True);
	detach(c);
	detachstack(c);
	if (!hasclientson(m, c->tags)) {
		settagsetlayout(m, c->tags, c->mon->vs->lt[c->mon->vs->curlt]);
		vs = getviewstackof(m, c->tags);
		vs->showdock = c->mon->vs->showdock;
		vs->showbar = c->mon->vs->showbar;
	}
	if(c->isfloating) {
		xo = m->mx - c->mon->mx;
		yo = m->my - c->mon->my;
	}
	c->mon = m;
	c->x += xo;
	c->y += yo;
	attach(c);
	attachstack(c);
	if (!(m->vs->tagset & c->tags))
		monview(m, c->tags);
	selectmon(m);
	focus(c);
	arrange(NULL);
	for(m = mons; m; m = m->next)
		cleartags(m);
}

void
selectmon(Monitor* m) {
	selmon = m;
}

void
setclientstate(Client *c, long state) {
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
			PropModeReplace, (unsigned char *)data, 2);
}

void setdesktopnames(void) {
	XTextProperty text;
	Xutf8TextListToTextProperty(dpy, tags, TAGSLENGTH, XUTF8StringStyle, &text);
	XSetTextProperty(dpy, root, &text, netatom[NetDesktopNames]);
}

Bool
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4) {
	int n;
	Atom *protocols, mt;
	Bool exists = False;
	XEvent ev;

	if(proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
		mt = wmatom[WMProtocols];
		if(XGetWMProtocols(dpy, w, &protocols, &n)) {
			while(!exists && n--)
				exists = protocols[n] == proto;
			XFree(protocols);
		}
	}
	else {
		exists = True;
		mt = proto;
	}
	if(exists) {
		ev.type = ClientMessage;
		ev.xclient.window = w;
		ev.xclient.message_type = mt;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = d0;
		ev.xclient.data.l[1] = d1;
		ev.xclient.data.l[2] = d2;
		ev.xclient.data.l[3] = d3;
		ev.xclient.data.l[4] = d4;
		XSendEvent(dpy, w, False, mask, &ev);
	}
	return exists;
}

void
setnumdesktops(void) {
	long data[] = { TAGSLENGTH };
	XChangeProperty(dpy, root, netatom[NetNumberOfDesktops], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

void
setfocus(Client *c) {
	if(!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
						XA_WINDOW, 32, PropModeReplace,
						(unsigned char *) &(c->win), 1);
	}
	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
}

Client*
monhasfullscreenclient(Monitor *m) {
	Client *c;

	for(c = m->clients; c; c = c->next)
		if(c->isfullscreen && c->tags == vtag)
			return c;
	return NULL;
}

void
setfullscreen(Client *c, Bool fullscreen) {
	Monitor *m;
	Bool fullscreendenied = False;
	Bool attached;
	Client *fullc;

	if(fullscreen) {
		fullc = monhasfullscreenclient(c->mon);
		if(fullc && fullc != c) {
			fullscreendenied = True;
			for(m = mons; m; m = m->next)
				if(m != c->mon && !monhasfullscreenclient(m)) {
					attached = (c->snext != NULL && c->next != NULL);
					if(attached) {
						detach(c);
						detachstack(c);
					}
					c->mon = m;
					if(attached) {
						attach(c);
						attachstack(c);
					}
					fullscreendenied = False;
					break;
				}
		}
		if(fullscreendenied)
			return ;
		c->actual_opacity = (unsigned int)-1;
		window_opacity_set(c->win, c->actual_opacity);
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
						PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		if (!c->isfullscreen) {
			c->isfullscreen = c->tags;
			c->oldstate = c->isfloating;
			c->oldbw = c->bw;
		}
		c->bw = 0;
		c->isfloating = True;
        if (!(c->tags & vtag))
            c->tags = vtag;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		monview(c->mon, vtag);
	}
	else {
		setclientopacity(c);
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
						PropModeReplace, (unsigned char*)0, 0);
		if(c->isfullscreen)
			c->tags = c->isfullscreen;
		c->isfullscreen = 0;
		c->isfloating = c->oldstate;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		if (c->mon->vs->tagset == vtag && !hasclientson(c->mon, vtag)){
			monview(c->mon, c->tags);
			focus(c);
		}
	}
	restorebar(c->mon);
	arrange(c->mon);
}

void
monsetlayout(Monitor *m, const void* v) {
	Client *c;
	int gapw, gaph;
	Bool onewindowvisible;

	if (v != m->vs->lt[m->vs->curlt]) {
		if (!v)
			v = m->vs->lt[m->vs->curlt ^ 1];
		onewindowvisible = counttiledclients(m) == 1;
		if (onewindowvisible && !((Layout*)v)->arrange) {
			gapw = m->mw / 10;
			gaph = m->mh / 10;
			for (c = nexttiled(m->clients); c; c = nexttiled(c->next)) {
				c->bw = ((Layout*)v)->borderpx;
				resize(c, m->mx + gapw / 2, m->my + gaph / 2, m->mw - gapw, m->mh - gaph, True);
			}
		}
		m->vs->curlt ^= 1;
		m->vs->lt[m->vs->curlt] = (Layout *)v;
		m->vs->showdock = ((Layout *)v)->showdock;
		strncpy(m->ltsymbol, m->vs->lt[m->vs->curlt]->symbol, sizeof m->ltsymbol);
	}
}

void
setlayout(const Arg *arg) {
	monsetlayout(selmon, arg ? arg->v : NULL);
	if(selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

/* arg > 1.0 will set mfact absolutly */
void
setmfact(const Arg *arg) {
	float f;

	if(!arg || !selmon->vs->lt[selmon->vs->curlt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->vs->mfact : arg->f - 1.0;
	if(f < 0.1 || f > 0.9)
		return;
	selmon->vs->mfact = f;
	arrange(selmon);
}

void
readcolors() {
	const char* homedir = getenv("HOME");
	const char* relconfig = ".config/dwm/colors";
	char* colorFile = calloc(strlen(homedir) + strlen(relconfig) + 2, sizeof(char));
	int nummatch = 0;

	strcpy(colorFile, homedir);
	strcat(colorFile, "/");
	strcat(colorFile, relconfig);

	FILE* fd = fopen(colorFile, "r");

	if(fd != NULL) {
		fprintf(stderr, "Getting main colors from %s\n", colorFile);
		nummatch = fscanf(fd, "normbg \"%7s\", normfg \"%7s\", selbg \"%7s\", selfg \"%7s\"", normbgcolor, normfgcolor, selbgcolor, selfgcolor);
		fclose(fd);
	}
	else {
		fprintf(stderr, "Color file not found: %s\n", colorFile);
	}
	if (nummatch != 4) {
		fprintf(stderr, "Using default colors\n");
		strcpy(normbgcolor, defnormbgcolor);
		strcpy(normfgcolor, defnormfgcolor);
		strcpy(selbgcolor, defselbgcolor);
		strcpy(selfgcolor, defselfgcolor);
	}
	fprintf(stderr, "Colors set to:\n"
			"normal background: %s\n"
			"normal foreground: %s\n"
			"select background: %s\n"
			"select foreground: %s\n",
			normbgcolor, normfgcolor, selbgcolor, selfgcolor);
}

void
updatecolors(const Arg *arg) {
	Monitor *m;
	Client *icons, *c, *next;

	spawnimpl(arg, True, True);
	readcolors();

	dc.norm[ColBG] = getcolor(normbgcolor, dc.pangonorm+ColBG);
	dc.norm[ColFG] = getcolor(normfgcolor, dc.pangonorm+ColFG);
	dc.sel[ColBG] = getcolor(selbgcolor, dc.pangosel+ColBG);
	dc.sel[ColFG] = getcolor(selfgcolor, dc.pangosel+ColFG);

	for(m = mons; m; m = m->next)
		drawbar(m);
	if (systray) {
		if(showsystray) {
			icons = systray->icons;
			systray->icons = NULL;
			updatesystray();
			c = icons;
			icons = NULL;
			while (c) {
				next = c->next;
				c->next = icons;
				icons = c;
				c = next;
			}
			c = icons;
			while (c) {
				systrayaddwindow(c->win);
				c = c->next;
			}
			c = icons;
			while (c) {
				icons = c->next;
				free(c);
				c = icons;
			}
		}
	}
	createclocks();
}

void
startuserscript() {
	if (userscript) {
		const char* cmd[] = { "/bin/zsh", userscript, NULL };
		const Arg arg = {.v = cmd };
	
		spawnimpl(&arg, False, False);
	}
}

void
setup(void) {
	XSetWindowAttributes wa;

	/* clean up any zombies immediately */
	sigchld(0);

	/* read colors */
	readcolors();

	/* read config */
	readconfig();

	*ooftraysbl = 0;
	if (outoffocustraysymbol)
	{
		snprintf(ooftraysbl, 10, "%s", outoffocustraysymbol);
	}

	/* init screen */
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	initfont(font);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	dc.font.padding = 2;
	bh = dc.h = dc.font.height + dc.font.padding;
	updategeom();
	/* init atoms */
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetCloseWindow] = XInternAtom(dpy, "_NET_CLOSE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMMaximized] = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetWMWindowTypeDock] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
	netatom[NetWMWindowTypeDesktop] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	netatom[NetWMWindowTypeNotification] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);
	netatom[NetWMWindowTypeKDEOSD] = XInternAtom(dpy, "_KDE_NET_WM_WINDOW_TYPE_ON_SCREEN_DISPLAY", False);
	netatom[NetWMWindowTypeKDEOVERRIDE] = XInternAtom(dpy, "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE", False);
	netatom[NetWMStateSkipTaskbar] = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
	netatom[NetWMDesktop] = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
	netatom[NetWMOpacity] = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	netatom[NetDesktopViewport] = XInternAtom(dpy, "_NET_DESKTOP_VIEWPORT", False);
	netatom[NetDesktopGeometry] = XInternAtom(dpy, "_NET_DESKTOP_GEOMETRY", False);
	netatom[NetNumberOfDesktops] = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
	netatom[NetCurrentDesktop] = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
	netatom[NetDesktopNames] = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);
	xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
	xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
	xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
	xatom[MotifWMHints] = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
	/* init cursors */
	cursor[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(dpy, XC_fleur);
	/* init appearance */
	dc.norm[ColBorder] = getcolor(normbordercolor, dc.pangonorm+ColBorder);
	dc.norm[ColBG] = getcolor(normbgcolor, dc.pangonorm+ColBG);
	dc.norm[ColFG] = getcolor(normfgcolor, dc.pangonorm+ColFG);
	dc.sel[ColBorder] = getcolor(selbordercolor, dc.pangosel+ColBorder);
	dc.sel[ColBG] = getcolor(selbgcolor, dc.pangosel+ColBG);
	dc.sel[ColFG] = getcolor(selfgcolor, dc.pangosel+ColFG);
	dc.drawable = XCreatePixmap(dpy, root, DisplayWidth(dpy, screen), bh, DefaultDepth(dpy, screen));
	dc.gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, dc.gc, 1, LineSolid, CapButt, JoinMiter);

	dc.cairo.surface = cairo_xlib_surface_create(dpy, dc.drawable, DefaultVisual(dpy, screen), DisplayWidth(dpy, screen), bh);
	dc.cairo.context = cairo_create(dc.cairo.surface);

	/* init system tray */
	updatesystray();
	updatebars();
	updatestatus();
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
			PropModeReplace, (unsigned char *) netatom, NetLast);
	setnumdesktops();
	setdesktopnames();
	setviewport();
	setgeometry();
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select for events */
	wa.cursor = cursor[CurNormal];
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask
					|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys(root);
	startuserscript();
	killclocks();
	createclocks();
}

void
setviewport(void) {
	long data[] = { 0, 0 };
	XChangeProperty(dpy, root, netatom[NetDesktopViewport], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 2);
}

void
setgeometry() {
	long data[] = { mons->ww, mons->wh };
	XChangeProperty(dpy, root, netatom[NetDesktopGeometry], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 2);
}

Bool
shouldbeopaque(Client *c, Client *tiledsel) {
	int i, j, n;
	int ns = c ? c->mon->vs->msplit : 0;
	Client *o;
	Bool ismonocle = (c->mon->vs->lt[c->mon->vs->curlt]->arrange == &monocle);
	Window parent, *allwindows;
	unsigned int nwindows = 0;

	if(c->win == c->mon->backwin)
		return True;
	if(!ISVISIBLE(c))
		return False;
	if(ismonocle && c == tiledsel || c->isfloating)
		return True;
	if(c->mon->vs->lt[c->mon->vs->curlt]->arrange == varimono) {
		for(n = 0, o = nexttiled(c->mon->clients); o; o = nexttiled(o->next), ++n)
			if(o == c)
				i = n;
		if(ns > n)
			ns = n;
		if(i > n - ns)
			return True;
		if(c->mon->sel == c)
			return True;
		XQueryTree(dpy, root, &parent, &parent, &allwindows, &nwindows);
		for(j = nwindows - 1; j >= 0; --j) {
			for(i = 0, o = nexttiled(c->mon->clients); o && o->win != allwindows[j] && i <= n - ns; o = nexttiled(o->next), ++i);
			if(i <= n - ns) {
				XFree(allwindows);
				return o == c;
			}
		}
		XFree(allwindows);
	}
	return !ismonocle;
}

void
showhide(Client *c) {
	if(!c)
		return;
	if(ISVISIBLE(c)) { /* show clients top down */
		setclientstate(c, NormalState);
		if (!c->mon->backwin)
			XMoveWindow(dpy, c->win, c->x, c->y);
		if((!c->mon->vs->lt[c->mon->vs->curlt]->arrange || c->isfloating) && (!c->isfullscreen || rotatingMons))
			resize(c, c->x, c->y, c->w, c->h, False);
		showhide(c->snext);
	}
	else { /* hide clients bottom up */
		showhide(c->snext);
		setclientstate(c, WithdrawnState);
		if (!c->mon->backwin)
			XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
sigchld(int unused) {
	if(signal(SIGCHLD, sigchld) == SIG_ERR)
		die("Can't install SIGCHLD handler");
	while(0 < waitpid(-1, NULL, WNOHANG));
}

void
spawnimpl(const Arg *arg, Bool waitdeath, Bool useshcmd) {
	const char *shcmd[] = { "/bin/sh", "-c", arg->shcmd, NULL };
	const char **cmd = useshcmd ? shcmd : (const char**)arg->v;
	int childpid = fork();

	if(childpid == 0) {
		if(dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(cmd[0], (char **)cmd);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
	if (waitdeath) {
		waitpid(childpid, NULL, 0);
	}
}

void
spawnterm(const Arg* arg) {
	int childpid = fork();
	const char** termcmd = (const char**)terminal;

	if(childpid == 0) {
		if (termcmd[0] == NULL)
			termcmd = defaultterminal;
		execvp(termcmd[0], (char**)termcmd);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
}

void
spawn(const Arg *arg) {
	spawnimpl(arg, False, True);
}

void
systrayaddwindow (Window win) {
	XWindowAttributes wa;
	XSetWindowAttributes swa;
	Client* c;

	if(!(c = (Client *)calloc(1, sizeof(Client))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Client));
	c->mon = selmon;
	c->win = win;
	c->opacity = 1.;
	c->next = systray->icons;
	systray->icons = c;
	XGetWindowAttributes(dpy, c->win, &wa);
	c->x = c->oldx = c->y = c->oldy = 0;
	c->w = c->oldw = wa.width;
	c->h = c->oldh = wa.height;
	c->oldbw = wa.border_width;
	c->bw = 0;
	c->isfloating = True;
	/* reuse tags field as mapped status */
	c->tags = 1;
	updatesizehints(c);
	updatesystrayicongeom(c, c->w, c->h);
	XAddToSaveSet(dpy, c->win);
	XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
	XReparentWindow(dpy, c->win, systray->win, 0, 0);
	/* use parents background color */
	swa.background_pixel  = dc.norm[ColBG];
	XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
	sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
	/* FIXME not sure if I have to send these events, too */
	sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_FOCUS_IN, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
	sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
	sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_MODALITY_ON, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
	XSync(dpy, False);
	resizebarwin(selmon);
	updatesystray();
	setclientstate(c, NormalState);
}

Monitor *
systraytomon(Monitor *m) {
	Monitor *t;
	int i, n;
	if(!systraypinning) {
		if(!m)
			return selmon;
		return m == selmon ? m : NULL;
	}
	for(n = 1, t = mons; t && t->next; n++, t = t->next) ;
	for(i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next) ;
	if(systraypinningfailfirst && n < systraypinning)
		return mons;
	return t;
}

void
swap(Client *c1, Client *c2) {
	Client *tmp;
	Client **tc1 = 0, **tc2 = 0, **ttmp;

	for(ttmp = &selmon->clients; *ttmp; ttmp = &(*ttmp)->next) {
		if (*ttmp == c1) tc1 = ttmp;
		if (*ttmp == c2) tc2 = ttmp;
	}
	*tc1 = c2; *tc2 = c1;
	tmp = c2->next; c2->next = c1->next; c1->next = tmp;
}

void
tabview(const Arg *arg) {
	Arg a;

	if(arg->i == -1 || selmon->vs->tagset == arg->ui)
		a.ui = 0;
	else
		a.ui = arg->ui;
	view(&a);
}

void
tag(const Arg *arg) {
	if(selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		arrange(selmon);
	}
}

void
tagmon(const Arg *arg) {
	if(!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i));
}

long
tagsettonum (unsigned int tagset) {
	long i=0;

	if(0 < tagset && tagset < TAGMASK)
		do
			++i;
		while(tagset >> i);
	return i;
}

int
textnw(const char *text, unsigned int len) {
	PangoRectangle r;

	pango_layout_set_text(dc.cairo.layout, text, len);
	pango_layout_get_extents(dc.cairo.layout, 0, &r);
	return r.width / PANGO_SCALE;
}

void
monshowbar(Monitor* m, Bool show) {
	m->vs->showbar = show;
	updatebarpos(m);
	resizebarwin(m);
	if(showsystray) {
		XWindowChanges wc;
		if(!m->vs->showbar)
			wc.y = -bh;
		else if(m->vs->showbar) {
			wc.y = 0;
			if(!m->topbar)
				wc.y = m->mh - bh;
		}
		XConfigureWindow(dpy, systray->win, CWY, &wc);
	}
	arrange(m);
}

void
monshowdock(Monitor *m, Bool show) {
	m->vs->showdock = show;
	arrangemon(m);
}

void
togglebar(const Arg *arg) {
	monshowbar(selmon, !selmon->vs->showbar);
}

void
toggledock(const Arg *arg) {
	Monitor *m;

	for (m = mons; m && m->num != dockmonitor; m = m->next);
	if (m)
		monshowdock(m, !m->vs->showdock);
}

void
togglefloating(const Arg *arg) {
	if(!selmon->sel)
		return;
	if(selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if(selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			   selmon->sel->w, selmon->sel->h, False);
	arrange(selmon);
}

void
togglefoldtags(const Arg *arg) {
	foldtags = !foldtags;
	drawbars();
}

void
toggletag(const Arg *arg) {
	unsigned int newtags;

	if(!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if(newtags) {
		selmon->sel->tags = newtags;
		restorebar(selmon);
		arrange(selmon);
	}
	updatecurrentdesktop();
}

void
togglevarilayout(const Arg *arg) {
	const Layout *togglelayouts[2] =
	{
		(Layout*)arg->v,
		&layouts[VARIMONO],
	};
	int i = 0;

	if((Layout*)arg->v == selmon->vs->lt[selmon->vs->curlt])
		i = 1;
	monsetlayout(selmon, togglelayouts[i]);
	if(selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

void
toggleview(const Arg *arg) {
	unsigned int newtagset = selmon->vs->tagset ^ (arg->ui & TAGMASK);

	if(newtagset != 0) {
		if(selmon->vs->tagset != newtagset) {
			viewstackadd(selmon, newtagset, False);
			if(selmon->sel && !ISVISIBLE(selmon->sel))
				focus(NULL);
			arrange(selmon);
		}
		updatecurrentdesktop();
	}
}

void
togglewindowgap(const Arg *arg) {
	Bool multipletiled = False;
	Monitor *m;

	for(m = mons; !multipletiled && m; m = m->next)
		if (counttiledclients(m) > 1)
			multipletiled = True;
	if(multipletiled) {
		if(windowgap == 0) {
			windowgap = defaultwindowgap;
			arrange(NULL);
		}
		else if (windowgap > 0) {
			windowgap = 0;
			arrange(NULL);
		}
	}
}

void
unfocus(Client *c, Bool setfocus) {
	if(!c)
		return;
	grabbuttons(c, False);
	XSetWindowBorder(dpy, c->win, dc.norm[ColBorder]);
	if(setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
unmanage(Client *c, Bool destroyed) {
	Monitor *m = c->mon;
	XWindowChanges wc;

	/* The server grab construct avoids race conditions. */
	grabremap(c, False);
	detach(c);
	detachstack(c);
	if(!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	if(m == selmon)
		focus(NULL);
    updateclientlist();
    cleartags(m);
    arrange(m);
}

void
unmapnotify(XEvent *e) {
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if((c = wintoclient(ev->window)))
		unmanage(c, False);
	else if((c = wintosystrayicon(ev->window))) {
		XMapRaised(dpy, c->win);
		updatesystray();
	}
}

void
updateborderswidth(Monitor* m) {
	Client* c;
	Bool changed = False;
	unsigned int nc, bw;

	nc = counttiledclients(m);
	for(c = m->clients; c; c = c->next)
		if (ISVISIBLE(c) && !c->nofocus && !c->isfullscreen) {
			bw = 0;
			if ((nc > 1 || c->isfloating || !m->vs->lt[m->vs->curlt]->arrange || m->vs->showdock && m->num == dockmonitor) && !c->noborder)
				bw = m->vs->lt[m->vs->curlt]->borderpx;
			if (c->bw != bw) {
				c->bw = bw;
				configure(c);
				changed = True;
			}
		}
	if (changed)
		XSync(dpy, False);
	if (m == mons) {
		updatebarpos(m);
	}
}

void
updatebars(void) {
	unsigned int w;
	Monitor *m;

	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.background_pixel = dc.norm[ColBG],
		.event_mask = ButtonPressMask|ExposureMask
	};

	for(m = mons; m; m = m->next) {
		if(m->barwin)
			continue;
		w = m->ww;
		if(showsystray && m == systraytomon(m))
			w -= getsystraywidth();
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, w, bh, 0, DefaultDepth(dpy, screen),
								  CopyFromParent, DefaultVisual(dpy, screen),
								  CWOverrideRedirect|CWBackPixmap|CWBackPixel|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]);
		if(showsystray && m == systraytomon(m))
			XMapRaised(dpy, systray->win);
		XMapRaised(dpy, m->barwin);
		window_opacity_set(m->barwin, OPACITY_BYTES(barOpacity));
	}
}

void
updatebarpos(Monitor *m) {
	m->wy = m->my;
	m->wh = m->mh;
	if(m->vs->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	}
	else
		m->by = -bh;
	m->wyo = m->wy;
	m->who = m->wh;
	updatedockpos(m);
}

void
updateclientlist() {
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for(m = mons; m; m = m->next)
		for(c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
			                XA_WINDOW, 32, PropModeAppend,
			                (unsigned char *) &(c->win), 1);
}

void
updateclientdesktop(Client* c) {
	long data = tagsettonum(ISVISIBLE(c) ? c->mon->vs->tagset : c->tags);

	XChangeProperty(dpy, c->win, netatom[NetWMDesktop], XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&data, 1);
}

void
updatecurrentdesktop(void) {
	Monitor *m = mons;
	Client *c;
	long data;

	while (m && m->num != dockmonitor)
		m = m->next;
	if (m) {
		data = tagsettonum(m->vs->tagset);

		XChangeProperty(dpy, root, netatom[NetCurrentDesktop], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&data, 1);
		for(c = m->clients; c; c = c->next)
			updateclientdesktop(c);
	}
}
 
void
updatedockpos(Monitor *m) {
	if (m->vs->showdock && m->num == dockmonitor) {
		switch (dockposition)
		{
		case Top:
			m->wy = m->wyo + m->who * 45 / 1000;
		case Bottom:
			m->wh = m->who - m->who * 45 / 1000;
			break;
		case Left:
			m->wx = m->wxo + m->wwo * 26 / 1000;
		case Right:
			m->ww = m->wwo - m->wwo * 26 / 1000;
			break;
		}
	}
	else {
		m->wx = m->wxo;
		m->wy = m->wyo;
		m->ww = m->wwo;
		m->wh = m->who;
	}
}

void
killclocks(void) {
	Monitor *m;
	const Arg arg = {.v = killclockscmd };
	spawnimpl(&arg, True, False);

	for(m = mons; m; m = m->next)
		m->clock = 0;
}

void
createclocks(void) {
	Monitor* m;

	for(m = mons; m; m = m->next) {
		const Arg arg = {.v = clockcmd };
		spawnimpl(&arg, False, False);
	}
}

void
updategeom(void) {
	/* Starting with dwm 6.1 this function uses a new (simpler) strategy:
	 * whenever screen changes are reported, we destroy all monitors
	 * and recreate all unique origin monitors and add all clients to
	 * the first monitor, only. In several circumstances this may suck,
	 * but dealing with all corner-cases sucks even more.*/

#ifdef XINERAMA
	if(XineramaIsActive(dpy)) {
		int i, j, n;
		Client *c;
		Monitor *oldmons = mons;
		Monitor *m, *om = oldmons, *nm, *ocm, *cm;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &n);
		XineramaScreenInfo *unique = NULL;

		/* only consider unique geometries as separate screens */
		if(!(unique = (XineramaScreenInfo *)malloc(sizeof(XineramaScreenInfo) * n)))
			die("fatal: could not malloc() %u bytes\n", sizeof(XineramaScreenInfo) * n);
		for(i = 0, j = 0; i < n; i++)
			if(isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		/* create new monitor structure */
		n = j;
		mons = m = createmon(); /* new first monitor */
		for(i = 1; i < n; i++) {
			m->next = createmon();
			m = m->next;
		}
		for(i = 0, m = mons; i < n && m; m = m->next, i++) {
			m->num = i;
			m->wxo = m->mx = m->wx = unique[i].x_org;
			m->wyo = m->my = m->wy = unique[i].y_org;
			m->wwo = m->mw = m->ww = unique[i].width;
			m->who = m->mh = m->wh = unique[i].height;
			updatebarpos(m);
		}
		free(unique);
		/* re-attach old clients and cleanup old monitor structure */
		nm = mons;
		while(om) {
			m = om;
			while(m->clients) {
				c = m->clients;
				m->clients = c->next;
				detachstack(c);
				for(ocm = oldmons, cm = mons; ocm && cm && ocm != c->mon; ocm = ocm->next, cm = cm->next);
				if(cm)
					c->mon = cm;
				else
					c->mon = mons;
				attach(c);
				attachstack(c);
			}
			om = m->next;
			if (nm) {
				nm->vs = m->vs;
				m->vs = NULL;
				nm = nm->next;
			}
		}
		XGrabServer(dpy);
		for(; om; om = om->next)
			if(om->clock) {
				XKillClient(dpy, om->clock);
				XSync(dpy, False);
			}
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
		while (oldmons) {
			om = oldmons;
			XUnmapWindow(dpy, om->barwin);
			XDestroyWindow(dpy, om->barwin);
			oldmons = oldmons->next;
			free(om);
		}
		resetprimarymonitor();
	}
	else
#endif /* XINERAMA */
	/* default monitor setup */
	{
		if(!mons) /* only true if !XINERAMA compile flag */
			mons = createmon();
		if(mons->mw != sw || mons->mh != sh) {
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (selmon != mons) {
		selectmon(mons);
		centerMouseInMonitorIndex(focusmonstart);
	}
	selectmon(wintomon(root));
}

void
updatenumlockmask(void) {
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for(i = 0; i < 8; i++)
		for(j = 0; j < modmap->max_keypermod; j++)
			if(modmap->modifiermap[i * modmap->max_keypermod + j]
			   == XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c) {
	long msize;
	XSizeHints size;

	if(!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if(size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	}
	else if(size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	}
	else
		c->basew = c->baseh = 0;
	if(size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	}
	else
		c->incw = c->inch = 0;
	if(size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	}
	else
		c->maxw = c->maxh = 0;
	if(size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	}
	else if(size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	}
	else
		c->minw = c->minh = 0;
	if(size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	}
	else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->minw && c->maxh && c->minh
				 && c->maxw == c->minw && c->maxh == c->minh);
}

void
updatetitle(Client *c) {
	if(!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if(c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

long
getmodtimefor (const char* filepath)
{
	struct stat buf;
	int fd = open(filepath, O_RDONLY);
	int status;

	if (fd != -1)
	{
		status = fstat(fd, &buf);
		close(fd);
	
		if (status == 0)
			return buf.st_mtime;
	}
	return 0;
}

void
checkconfigtimes() {
	const char* homedir = getenv("HOME");
	const char* relconfig = ".config/dwm/colors";
	char* colorFile = calloc(strlen(homedir) + strlen(relconfig) + 2, sizeof(char));

	strcpy(colorFile, homedir);
	strcat(colorFile, "/");
	strcat(colorFile, relconfig);

	const char* relbg = ".fehbg";
	char* bgFile = calloc(strlen(homedir) + strlen(relbg) + 2, sizeof(char));

	strcpy(bgFile, homedir);
	strcat(bgFile, "/");
	strcat(bgFile, relbg);

	long configmodtime = getmodtimefor(colorFile);
	long bgmodtime = getmodtimefor(bgFile);

	if (configmodtime != 0 && bgmodtime != 0 && bgmodtime > configmodtime)
	{
		const Arg arg = {.shcmd = "exec ~/hacks/scripts/updateDwmColor.sh"};

		updatecolors(&arg);
	}
}

void
updatestatus(void) {
	Monitor* m;
	if(!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "dwm-"VERSION);
	if (statusallmonitor) {
		for(m = mons; m; m = m->next)
			drawbar(m);
	}
	else
		drawbar(selmon);
	checkconfigtimes();
}

void
updatesystrayicongeom(Client *i, int w, int h) {
	if(i) {
		i->h = bh;
		if(w == h)
			i->w = bh;
		else if(h == bh)
			i->w = w;
		else
			i->w = (int) ((float)bh * ((float)w / (float)h));
		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
		/* force icons into the systray dimenons if they don't want to */
		if(i->h > bh) {
			if(i->w == i->h)
				i->w = bh;
			else
				i->w = (int) ((float)bh * ((float)i->w / (float)i->h));
			i->h = bh;
		}
	}
}

void
updatesystrayiconstate(Client *i, XPropertyEvent *ev) {
	long flags;
	int code = 0;

	if(!showsystray || !i || ev->atom != xatom[XembedInfo] ||
			!(flags = getatomprop(i, xatom[XembedInfo])))
		return;

	if(flags & XEMBED_MAPPED && !i->tags) {
		i->tags = 1;
		code = XEMBED_WINDOW_ACTIVATE;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	}
	else if(!(flags & XEMBED_MAPPED) && i->tags) {
		i->tags = 0;
		code = XEMBED_WINDOW_DEACTIVATE;
		XUnmapWindow(dpy, i->win);
		setclientstate(i, WithdrawnState);
	}
	else
		return;
	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
}

void
updatesystray(void) {
	XSetWindowAttributes wa;
	XWindowChanges wc;
	Client *i;
	Monitor *m = systraytomon(NULL);
	unsigned int x = m->mx + m->mw;
	unsigned int w = 1;

	if(!showsystray)
		return;
	if(!systray) {
		/* init systray */
		if(!(systray = (Systray *)calloc(1, sizeof(Systray))))
			die("fatal: could not malloc() %u bytes\n", sizeof(Systray));
		systray->win = XCreateSimpleWindow(dpy, root, x, m->by, w, bh, 0, 0, dc.norm[ColBG]);
		wa.event_mask        = ButtonPressMask | ExposureMask;
		wa.override_redirect = True;
		wa.background_pixel  = dc.norm[ColBG];
		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)&netatom[NetSystemTrayOrientationHorz], 1);
		XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel, &wa);
		XMapRaised(dpy, systray->win);
		window_opacity_set(systray->win, OPACITY_BYTES(barOpacity));
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if(XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
			XSync(dpy, False);
		}
		else {
			fprintf(stderr, "dwm: unable to obtain system tray.\n");
			free(systray);
			systray = NULL;
			return;
		}
	}
	for(w = 0, i = systray->icons; i; i = i->next) {
		/* make sure the background color stays the same */
		wa.background_pixel  = dc.norm[ColBG];
		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
		XMapRaised(dpy, i->win);
		w += systrayspacing;
		i->x = w;
		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
		w += i->w;
		if(i->mon != m)
			i->mon = m;
	}
	w = w ? w + systrayspacing : 1;
	x -= w;
	XMoveResizeWindow(dpy, systray->win, x, m->by, w, bh);
	wc.x = x; wc.y = m->by; wc.width = w; wc.height = bh;
	wc.stack_mode = Above; wc.sibling = m->barwin;
	XConfigureWindow(dpy, systray->win, CWX|CWY|CWWidth|CWHeight|CWSibling|CWStackMode, &wc);
	XMapWindow(dpy, systray->win);
	XMapSubwindows(dpy, systray->win);
	/* redraw background */
	XSetForeground(dpy, dc.gc, dc.norm[ColBG]);
	XFillRectangle(dpy, systray->win, dc.gc, 0, 0, w, bh);
	XSync(dpy, False);
}

// From Xm/MwmUtil.h:
/* bit definitions for MwmHints.flags */
#define MWM_HINTS_FUNCTIONS	(1L << 0)
#define MWM_HINTS_DECORATIONS	(1L << 1)
#define MWM_HINTS_INPUT_MODE	(1L << 2)
#define MWM_HINTS_STATUS	(1L << 3)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL		(1L << 0)
#define MWM_FUNC_RESIZE		(1L << 1)
#define MWM_FUNC_MOVE		(1L << 2)
#define MWM_FUNC_MINIMIZE	(1L << 3)
#define MWM_FUNC_MAXIMIZE	(1L << 4)
#define MWM_FUNC_CLOSE		(1L << 5)

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL		(1L << 0)
#define MWM_DECOR_BORDER	(1L << 1)
#define MWM_DECOR_RESIZEH	(1L << 2)
#define MWM_DECOR_TITLE		(1L << 3)
#define MWM_DECOR_MENU		(1L << 4)
#define MWM_DECOR_MINIMIZE	(1L << 5)
#define MWM_DECOR_MAXIMIZE	(1L << 6)

void
updatemwmtype(Client *c) {
	XWindowChanges wc;
	int actual_format;
	Atom actual_type;
	unsigned long nitems, bytesafter;
	unsigned char *p = NULL;
	Atom* mwmhints;
	int bw = c->bw;
	enum {
		MwmFlags = 0,
		MwmFunctions,
		MwmDecorations,
		MwmInputmode,
		MwmStatus,
		MwmNumProps
	};

	if(XGetWindowProperty(dpy, c->win, xatom[MotifWMHints], 0L, 32L, False,
				xatom[MotifWMHints], &actual_type, &actual_format, &nitems,
				&bytesafter,(unsigned char **)&p) == Success && p) {

		mwmhints = (Atom*)p;

		if(nitems == MwmNumProps) {
			if(mwmhints[MwmFlags] & MWM_HINTS_DECORATIONS) {
				if(!(mwmhints[MwmDecorations] & (MWM_DECOR_ALL|MWM_DECOR_BORDER))) {
					c->bw = 0;
					c->noborder = True;
				}
			}
			if (mwmhints[MwmFlags] & MWM_HINTS_FUNCTIONS) {
				if (mwmhints[MwmFunctions] == 0) {
					c->isoverride = True;
				}
				else if ((unsigned int)mwmhints[MwmFunctions] == (MWM_FUNC_MOVE|MWM_FUNC_CLOSE)) {
					// FIXME: detect Wine tooltips
					c->isoverride = True;
					c->isosd = True;
				}
			}
		}
		XFree(mwmhints);
	}
	if(bw != c->bw) {
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
	}
}

void
updatewindowtype(Client *c) {
	int i, numtypes, bw = c->bw;
	const Atom state = getatomprop(c, netatom[NetWMState]);
	Atom* wtypes = getatomprops(c, netatom[NetWMWindowType], &numtypes);
	Atom wtype;
	XWindowChanges wc;

	if(state == netatom[NetWMFullscreen])
		setfullscreen(c, True);
    updatemwmtype(c);
	for(i = 0; i < numtypes; ++i) {
		wtype = wtypes[i];
		if(wtype == netatom[NetWMWindowTypeDock]) {
			c->nofocus = True;
			c->isfloating = True;
			c->tags = ~0;
			c->bw = 0;
			dockwin = c->win;
		}
		else if(wtype == netatom[NetWMWindowTypeDesktop]) {
			c->isfloating = True;
			c->tags = 0;
			c->bw = 0;
			c->nofocus = False;
			c->isfullscreen = True;
			c->opacity = SPCTR;
			c->isbackwin = True;
		}
		else if(wtype == netatom[NetWMWindowTypeKDEOSD]) {
			c->isfloating = True;
			c->bw = 0;
			c->tags = TAGMASK;
			c->isosd = True;
			c->noborder = True;
			c->x = c->mon->mx + c->mon->mw - WIDTH(c) - c->mon->mw / 40;
			c->y = c->mon->my + bh + c->mon->mh / 40;
		}
		else if(wtype == netatom[NetWMWindowTypeNotification]) {
			c->isfloating = True;
			c->bw = 0;
			c->tags = TAGMASK;
			c->isosd = True;
			c->noborder = True;
		}
		else if(wtype == netatom[NetWMWindowTypeDialog] || state == netatom[NetWMStateSkipTaskbar]) {
			c->isfloating = True;
		}
	}
	if(wtypes != NULL)
		XFree(wtypes);
	if(bw != c->bw) {
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
	}
}

void
updatewmhints(Client *c) {
	XWMHints *wmh;

	if((wmh = XGetWMHints(dpy, c->win))) {
		if(c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		}
		else
			c->isurgent = (wmh->flags & XUrgencyHint) ? True : c->isurgent;
		if(wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = False;
		XFree(wmh);
	}
}

void
monview(Monitor* m, unsigned int ui) {
	unsigned int newtagset;

	if((ui & TAGMASK) == m->vs->tagset)
		return;
	if(ui & TAGMASK)
		newtagset = ui & TAGMASK;
	else if (hasvclients(m) && (m->vs->tagset & vtag) == 0)
		newtagset = vtag;
	else
		newtagset = findtoggletagset(m);
	viewstackadd(m, newtagset, True);
	updatecurrentdesktop();
	focus(NULL);
}

void
view(const Arg *arg) {
	monview(selmon, arg->ui);
	focus(NULL);
	restorebar(selmon);
	arrange(selmon);
}

pid_t
winpid(Window w)
{
	pid_t result = 0;

	xcb_res_client_id_spec_t spec = {0};
	spec.client = w;
	spec.mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID;

	xcb_generic_error_t *e = NULL;
	xcb_res_query_client_ids_cookie_t c = xcb_res_query_client_ids(xcon, 1, &spec);
	xcb_res_query_client_ids_reply_t *r = xcb_res_query_client_ids_reply(xcon, c, &e);

	if (!r)
		return (pid_t)0;

	xcb_res_client_id_value_iterator_t i = xcb_res_query_client_ids_ids_iterator(r);
	for (; i.rem; xcb_res_client_id_value_next(&i)) {
		spec = i.data->spec;
		if (spec.mask & XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID) {
			uint32_t *t = xcb_res_client_id_value_value(i.data);
			result = *t;
			break;
		}
	}

	free(r);

	if (result == (pid_t)-1)
		result = 0;
	return result;
}

Client *
wintoclient(Window w) {
	Client *c;
	Monitor *m;

	for(m = mons; m; m = m->next)
		for(c = m->clients; c; c = c->next)
			if(c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w) {
	int x, y;
	Client *c;
	Monitor *m;

	if(w == root && getrootptr(&x, &y)) {
		m = recttomon(x, y, 1, 1);
		return m;
	}
	for(m = mons; m; m = m->next)
		if(w == m->barwin || m->backwin && w == m->backwin)
			return m;
	if((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

Client *
wintosystrayicon(Window w) {
	Client *i = NULL;

	if(!showsystray || !w)
		return i;
	for(i = systray->icons; i && i->win != w; i = i->next) ;
	return i;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's).  Other types of errors call Xlibs
 * default error handler, which may call exit.	*/
int
xerror(Display *dpy, XErrorEvent *ee) {
	if(ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable)
	|| (ee->request_code == 139 && ee->error_code == BadLength)
	|| (ee->request_code == X_CreateWindow && ee->error_code == BadValue))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
			ee->request_code, ee->error_code);

	const int textlen = 2048;
	char text[textlen];

	XGetErrorText(dpy, ee->error_code, text, textlen);

	fprintf(stderr, "%s\n", text);
	if (ee->request_code == X_ChangeWindowAttributes && ee->error_code == BadMatch)
		return 0;
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee) {
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee) {
	return -1;
}

void
zoom(const Arg *arg) {
	Client *c = selmon->sel;

	if(!selmon->vs->lt[selmon->vs->curlt]->arrange
	|| selmon->vs->lt[selmon->vs->curlt]->arrange == monocle
	|| (selmon->sel && selmon->sel->isfloating))
		return;
	if(!c)
		return ;
	if(c == nexttiled(selmon->clients) || ismasterclient(c))
		push(c);
	else
		pop(c);
}

void
noop(const Arg *arg) {
	fprintf(stderr, "NOOP\n");
	if(selmon && selmon->sel)
		fprintf(stderr, "selmon->sel->name: '%s'\n", selmon->sel->name);
}

int
main(int argc, char *argv[]) {
	if(argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION", © 2006-2012 dwm engineers, see LICENSE for details\n");
	else if(argc != 1)
		die("usage: dwm [-v]\n");
	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if(!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display\n");
	if (!(xcon = XGetXCBConnection(dpy)))
		die("dwm: cannot get xcb connection\n");
	checkotherwm();
	setup();
	scan();
	startup = False;
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
