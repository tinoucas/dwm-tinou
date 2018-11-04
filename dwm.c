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
#include <X11/Xft/Xft.h>
#include <pango/pango.h>
#include <pango/pangoxft.h>
#include <pango/pango-font.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */

/* macros */
#define BARSHOWN(m)             (m->by != -bh)
#define BUTTONMASK				(ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)			(mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)			((C->tags & C->mon->vs->tagset))
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
#define TAGMASK					((1 << LENGTH(tags)) - 1)
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

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast };		/* cursor */
enum { ColBorder, ColFG, ColBG, ColLast };				/* color */
	   
enum { NetSupported, NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation,
	   NetWMName, NetWMState, NetWMFullscreen, NetWMPID, NetActiveWindow, NetWMWindowType,
	   NetWMWindowTypeDialog, NetWMStateSkipTaskbar, NetClientList, NetLast }; /* EWMH atoms */
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
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
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct {
	unsigned int mousebuttonfrom;
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
	Bool isfixed, isfloating, isurgent, neverfocus, oldstate, noborder, nofocus, isfullscreen;
	Client *snext;
	Monitor *mon;
	Window win;
	double opacity;
	Bool rh;
	const Remap* remap;
};

typedef struct {
	int x, y, w, h;
	unsigned long norm[ColLast];
	unsigned long sel[ColLast];
	Drawable drawable;
	GC gc;

	XftColor  xftnorm[ColLast];
	XftColor  xftsel[ColLast];
	XftDraw  *xftdrawable;

	PangoContext *pgc;
	PangoLayout  *plo;
	PangoFontDescription *pfd;

	struct {
		int ascent;
		int descent;
		int height;
	} font;
} DC; /* draw context */

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
	int borderpx;
} Layout;

struct ViewStack;
typedef struct ViewStack ViewStack;
struct ViewStack {
	struct ViewStack* next;
	unsigned int tagset;
	const Layout *lt[2];
	Bool showbar;
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
	Bool topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
	Bool hasclock;
	ViewStack* vs;
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	unsigned int tags;
	Bool isfloating;
	double istransparent;
	Bool nofocus;
	Bool noborder;
	Bool rh;
	int monitor;
	const Remap* remap;
	const Layout* preflayout;
	Bool istransient;
	const char* procname;
} Rule;

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
static void applyrules(Client *c);
static Bool applysizehints(Client *c, int *x, int *y, int *w, int *h, Bool interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachabove(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void buttonrelease(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void cleartags(Monitor *m);
static void clearurgent(Client *c);
static void clientmessage(XEvent *e);
static void client_opacity_set(Client *c, double opacity);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void createclocks(void);
static void centerclient(Client *c);
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
static void drawtext(const char *text, unsigned long col[ColLast], Bool invert);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static unsigned long getcolor(const char *colstr, XftColor *color);
static Client *getclientunderpt(int x, int y);
static Bool getrootptr(int *x, int *y);
static long getstate(Window w);
static unsigned int getsystraywidth();
static Bool gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, Bool focused);
static void grabkeys(Window window);
static void grabremap(Client *c, Bool focused);
static void initfont(const char *fontstr);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void removesystrayicon(Client *i);
static void resize(Client *c, int x, int y, int w, int h, Bool interact);
static void resizebarwin(Monitor *m);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizefast(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void resizerequest(XEvent *e);
static void restack(Monitor *m);
static void restorebar(Monitor *m);
static void run(void);
static void scan(void);
static Bool sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, Bool fullscreen);
static void monsetlayout(Monitor *m, const void* v);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(void);
static void showhide(Client *c);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static void spawnimpl(const Arg *arg, Bool waitdeath);
static Monitor *systraytomon(Monitor *m);
static void swap(Client *c1, Client *c2);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static int textnw(const char *text, unsigned int len);
static void tile(Monitor *);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, Bool setfocus);
static void unmanage(Client *c, Bool destroyed);
static void unmapnotify(XEvent *e);
static Bool updateborderwidth(Monitor *m, int* nc);
static void updatecolors(const Arg *arg);
static void updategeom(void);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
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
static void window_opacity_set(Window win, double opacity);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static Client *wintosystrayicon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);

/* variables */
static Systray *systray = NULL;
static unsigned long systrayorientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
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
static Client* lastclient = NULL;
static Bool startup = True;
static Bool rotatingMons = False;
static unsigned int statuscommutator = 0;

/* configuration, allows nested code to access above variables */
#include "config.h"

static char ooftraysbl[OOFTRAYLEN];

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

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

void
centerclient (Client *c)
{
	int x, y;
	if (c && c->mon) {
		x = c->mon->mx + c->mon->mw / 2 - c->w / 2 - c->bw;
		y = c->mon->my + c->mon->mh / 2 - c->h / 2 - c->bw;
		resize(c, x, y, c->w, c->h, False);
	}
}

Bool
clientmatchesrule (Client *c, const char* class, const char* instance, Bool istransient, const char* wincmdline, const Rule *r) {
	if (strstr(class, "broken") && r->procname)
	{
		fprintf(stderr, "cmdline:'%s', class:'%s', instance:'%s', trans:'%s'\n",
				wincmdline ? wincmdline : "NULL",
				class ? class : "NULL",
				instance ? instance : "NULL",
				istransient ? "True" : "False");
	}
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
	if (c->isfloating)
		centerclient(c);
	c->nofocus = r->nofocus;
	c->noborder = r->noborder;
	c->tags |= r->tags;
	c->opacity = r->istransparent;
	c->rh = r->rh;
	c->remap = r->remap;
	if (istransient)
		c->isfixed = True;
	for(m = mons; m && m->num != r->monitor; m = m->next);
	if(m)
		c->mon = m;
}

long getwinpid (Client* c) {
	long pid = getatomprop(c, netatom[NetWMPID]);
	return pid;
}

char* getwincmdline (Client* c) {
	long pid = getwinpid(c);

	char procfile[1024];
	char* cmdline = calloc(1024, sizeof(char));
	int nread = 0;

	if (pid != 0) {
		snprintf(procfile, 1023, "/proc/%ld/cmdline", pid);
	
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
		fprintf(stderr, "pid: %ld, cmdline: %s\n", pid, cmdline);
	}
	return cmdline;
}

/* function implementations */
void
applyrules(Client *c) {
	const char *class, *instance;
	char* wincmdline = NULL;
	unsigned int i;
	const Rule *r;
	const Rule *lastr = NULL;
	Monitor *m;
	XClassHint ch = { 0 };
	Bool found = False;
	Bool istransient = False;

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
		for(i = 0; i < LENGTH(rules); i++) {
			r = &rules[i];
			if (clientmatchesrule(c, class, instance, istransient, wincmdline, r))
			{
				applyclientrule(c, r, istransient);
				lastr = r;
				fprintf(stderr, "Applying rule %d: name == '%s', class == '%s', instance == '%s'\n",
						i, c->name ? c->name : "NULL", class ? class : "NULL", instance ? instance : "NULL");
				found = True;
			}
		}
		if (clientmatchesrule(c, class, instance, istransient, wincmdline, &clockrule)) {
			applyclientrule(c, &clockrule, istransient);
			for(m = mons; m && m->hasclock; m = m->next);
			if (m) {
				m->hasclock = True;
				c->mon = m;
			}
		}
		if (!found)
			fprintf(stderr, "No rule applied for: name == '%s', class == '%s', instance == '%s'\n",
					c->name ? c->name : "NULL", class ? class : "NULL", instance ? instance : "NULL");
		if(ch.res_class)
			XFree(ch.res_class);
		if(ch.res_name)
			XFree(ch.res_name);
		if (wincmdline)
			free(wincmdline);
	}
	else
	{
		for(i = 0; i < LENGTH(rules); i++) {
			r = &rules[i];
			if(r->title && strstr(c->name, r->title))
			{
				c->isfloating = r->isfloating;
				c->nofocus = r->nofocus;
				c->noborder = r->noborder;
				c->tags |= r->tags;
				c->opacity = r->istransparent;
				c->rh = r->rh;
				c->remap = r->remap;
				for(m = mons; m && m->num != r->monitor; m = m->next);
				if(m)
					c->mon = m;
				found = True;
				lastr = r;
				fprintf(stderr, "Applying rule %d ('%s'): name == '%s'\n",
						i, r->title ? r->title : "NULL", c->name ? c->name : "NULL");
			}
		}
		if(!found) {
			fprintf(stderr, "No rule applied for: name == '%s'\n", c->name ? c->name : "NULL");
			r = &defaultrule;
			c->isfloating = r->isfloating;
			c->nofocus = r->nofocus;
			c->noborder = r->noborder;
			c->tags |= r->tags;
			c->opacity = r->istransparent;
			c->rh = r->rh;
			c->remap = r->remap;
			for(m = mons; m && m->num != r->monitor; m = m->next);
			if(m)
				c->mon = m;
		}
	}
	if (c->isfloating)
		centerclient(c);
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->vs->tagset;
	if (c->mon)
	{
		unsigned int nc = 0;
		Client *cother;

		if (!startup) {
			Bool alone = True;

			for(cother = c->mon->clients; alone && cother; cother = cother->next) {
				if (cother != c && (cother->tags & c->tags) && !cother->nofocus)
				{
					alone = False;
				}
			}
			if (alone && !c->nofocus && c->tags != ~0)
			{
				if (c->tags == vtag)
					viewstackadd(c->mon, vtag, True);
				else
					viewstackadd(c->mon, c->mon->vs->tagset | (c->tags), False);
				if (c->tags & c->mon->vs->tagset)
					arrange(c->mon);
			}
		}
		if(lastr && lastr->preflayout) {
			if (lastr->preflayout != monoclelayout)
				for(cother = c->mon->clients; cother; cother = cother->next)
					if (cother != c && (cother->tags & c->tags) && !cother->nofocus)
						++nc;
			if(nc == 0) {
				if (c->tags & c->mon->vs->tagset) {
					monsetlayout(c->mon, lastr->preflayout);
					arrange(c->mon);
				}
				else {
					storestackviewlayout(c->mon, c->tags, lastr->preflayout);
				}
			}
		}
		else if (!startup && !c->nofocus )
		{
			c->isurgent = True;
			if ((c->tags & c->mon->vs->tagset) == 0)
				drawbar(c->mon);
		}
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
	focus(NULL);
	if(m) {
		arrangemon(m);
		restack(m);
	} else for(m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m) {
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
attachstack(Client *c) {
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
buttonpress(XEvent *e) {
	unsigned int i, x, click;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	click = ClkRootWin;
	/* focus monitor if necessary */
	if((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, True);
		selmon = m;
		focus(NULL);
	}
	if(ev->window == selmon->barwin) {
		i = x = 0;
		do
			x += TEXTW(tags[i]);
		while(ev->x >= x && ++i < LENGTH(tags));
		if(i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		}
		else if(ev->x < x + blw)
			click = ClkLtSymbol;
		else if(ev->x > selmon->ww - TEXTW(stext))
			click = ClkStatusText;
		else
			click = ClkWinTitle;
	}
	else if((c = wintoclient(ev->window))) {
		focus(c);
		click = ClkClientWin;
	}
	for(i = 0; i < LENGTH(buttons); i++)
		if(click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
buttonrelease(XEvent *e) {
	unsigned int i, click;
	Client* c;
	XButtonReleasedEvent *ev = &e->xbutton;

	click = ClkRootWin;
	if((c = wintoclient(ev->window))) {
		click = ClkClientWin;
	}
	if (selmon && (c = selmon->sel) && c->win && c->remap) {
		for(i = 0; c->remap[i].keysymto; i++)
			if(click == ClkClientWin
					&& c->remap[i].mousebuttonfrom
					&& c->remap[i].mousebuttonfrom == ev->button)
				sendKey(XKeysymToKeycode(dpy, c->remap[i].keysymto), c->remap[i].modifier);
	}
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
		if(ISVISIBLE(c) && !c->nofocus) {
			if (c->tags != ~0)
				newtags |= (c->tags & m->vs->tagset);
			++nc;
		}
	if(newtags && newtags != m->vs->tagset) {
		m->vs->tagset = newtags;
		arrange(m);
	}
	if (nc == 0) {
		monsetlayout(m, &layouts[initlayout]);
		if (m->vs->tagset == vtag)
			monview(m, 0);
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
	XWindowAttributes wa;
	XSetWindowAttributes swa;
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if(showsystray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
		/* add systray icons */
		fprintf(stderr, "Creating systray icon client.\n");

		if(cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
			if(!(c = (Client *)calloc(1, sizeof(Client))))
				die("fatal: could not malloc() %u bytes\n", sizeof(Client));
			c->win = cme->data.l[2];
			c->mon = selmon;
			c->next = systray->icons;
			systray->icons = c;
			XGetWindowAttributes(dpy, c->win, &wa);
			c->x = c->oldx = c->y = c->oldy = 0;
			c->w = c->oldw = wa.width;
			c->h = c->oldh = wa.height;
			fprintf(stderr, "new systray icon, size: %dx%d\n", c->w, c->h);
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
			swa.border_pixel  = dc.norm[ColBG];
			swa.background_pixmap = ParentRelative;
			XChangeWindowAttributes(dpy, c->win, CWBackPixel|CWBorderPixel|CWBackPixmap, &swa);
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
		if(!ISVISIBLE(c)) {
			c->isurgent = True;
			drawbars();
		}
#if 0
		pop(c);
#else
		focus(c);
		arrange(c->mon);
#endif
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
		if(dc.drawable != 0)
			XFreePixmap(dpy, dc.drawable);
		dc.drawable = XCreatePixmap(dpy, root, sw, bh, DefaultDepth(dpy, screen));
		updatebars();
		for(m = mons; m; m = m->next) {
			for(c = m->clients; c; c = c->next)
				if(c->isfullscreen)
					resizeclient(c, m->mx, m->my, m->mw, m->mh);
			resizebarwin(m);
		}
		focus(NULL);
		arrange(NULL);
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

	if (c && c->isfloating)
		centerclient(c);
}

Monitor *
createmon(void) {
	Monitor *m;

	if(!(m = (Monitor *)calloc(1, sizeof(Monitor))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Monitor));
	m->vs = createviewstack(m, NULL);
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
		for(t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
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

	resizebarwin(m);
	if(showsystray && m == systraytomon(m)) {
		m->ww -= getsystraywidth();
	}
	for(c = m->clients; c; c = c->next) {
		if(c->tags != TAGMASK && !c->nofocus)
			occ |= c->tags;
		if(c->isurgent)
			urg |= c->tags;
	}
	if ((occ & vtag) && statuscommutator) {
		for(c = m->clients; !hasfullscreenv && c; c = c->next)
			if (c->tags & vtag && c->isfullscreen)
				hasfullscreenv = True;
		if (hasfullscreenv)
			occ &= ~vtag;
	}
	dc.x = 0;
	for(i = 0; i < LENGTH(tags); i++) {
		dc.w = TEXTW(tags[i]);
		col = m->vs->tagset & 1 << i ? dc.sel : dc.norm;
		drawtext(tags[i], col, urg & 1 << i);
		drawsquare(m == selmon && selmon->sel && !selmon->sel->nofocus && selmon->sel->tags & 1 << i || (1 << i) == vtag && hasfullscreenv,
				   occ & 1 << i, urg & 1 << i, col);
		dc.x += dc.w;
	}
	dc.w = blw = TEXTW(m->ltsymbol);
	drawtext(m->ltsymbol, dc.norm, False);
	dc.x += dc.w;
	x = dc.x;
	if(m == selmon || statusallmonitor) { /* status is only drawn on selected monitor */
		if (m != selmon)
		{
			dc.x = m->ww - (int)getsystraywidth();
			dc.w = (int)getsystraywidth();
			drawtext(ooftraysbl, dc.norm, False);
		}
		dc.w = TEXTW(stext);
		dc.x = m->ww - dc.w - (m == selmon ? 0 : getsystraywidth());
		if(dc.x < x) {
			dc.x = x;
			dc.w = m->ww - x;
		}
		drawtext(stext, dc.norm, False);
	}
	else
		dc.x = m->ww;
	if((dc.w = dc.x - x) > bh) {
		dc.x = x;
		col = m == selmon ? dc.sel : dc.norm;
		if(m->sel) {
			drawtext(m->sel->name, col, False);
			drawsquare(m->sel->isfixed, m->sel->isfloating, False, col);
		}
		else {
			drawtext(NULL, col, False);
		}
	}
	if(showsystray && m == systraytomon(m)) {
		m->ww += getsystraywidth();
	}
	XCopyArea(dpy, dc.drawable, m->barwin, dc.gc, 0, 0, m->ww, bh, 0, 0);
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
drawtext(const char *text, unsigned long col[ColLast], Bool invert) {
	char buf[256];
	int i, x, y, h, len, olen;
	XRectangle r = { dc.x, dc.y, dc.w, dc.h };

	XSetForeground(dpy, dc.gc, col[invert ? ColFG : ColBG]);
	XFillRectangles(dpy, dc.drawable, dc.gc, &r, 1);
	if(!text)
		return;
	olen = strlen(text);
	h = dc.font.ascent + dc.font.descent;
	y = dc.y + (dc.h / 2) - (h / 2);
	x = dc.x + (h / 2);
	/* shorten text if necessary */
	for(len = MIN(olen, sizeof buf); len && textnw(text, len) > dc.w - h; len--);
	if(!len)
		return;
	memcpy(buf, text, len);
	if(len < olen)
		for(i = len; i && i > len - 3; buf[--i] = '.');
	pango_layout_set_text(dc.plo, text, len);
	pango_xft_render_layout(dc.xftdrawable, (col==dc.norm?dc.xftnorm:dc.xftsel)+(invert?ColBG:ColFG), dc.plo, x * PANGO_SCALE, y * PANGO_SCALE);
}

void
enternotify(XEvent *e) {
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	if((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, True);
		selmon = m;
	}
	else if(c == selmon->sel || c == NULL)
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

#define OPAQUE	0xffffffff
#define OPACITY	"_NET_WM_WINDOW_OPACITY"

void
window_opacity_set(Window win, double opacity)
{
	if(opacity >= 0. && opacity <= 1.)
	{
		unsigned int copacity = (unsigned int)(opacity * OPAQUE);
		XChangeProperty(dpy, win, XInternAtom(dpy, OPACITY, False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &copacity, 1L);
  XSync(dpy, False);
	}
	XSync(dpy, False);
}

void
focus(Client *c) {
	Client *inc = c;

	if(!c || !ISVISIBLE(c))
		for(c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	/* was if(selmon->sel) */
	if(selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, False);
	if(c) {
		if(c->mon != selmon)
			selmon = c->mon;
		if(c->isurgent)
			clearurgent(c);
		detachstack(c);
		attachstack(c);
	}
	while (!inc && c && c->nofocus)
		c = c->next;
	if (c && !ISVISIBLE(c))
		c = NULL;
	if (c) {
		grabbuttons(c, True);
		XSetWindowBorder(dpy, c->win, dc.sel[ColBorder]);
		setfocus(c);
	}
	else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
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
	unfocus(selmon->sel, False);
	selmon = m;
	focus(NULL);
	if (selmon->sel)
		centerMouseInWindow(selmon->sel);
	else
		centerMouseInMonitor(selmon);
}

void
focusstack(const Arg *arg) {
	Client *c = NULL, *i;

	if(!selmon->sel)
		return;
	if(arg->i > 0) {
		c = selmon->sel->next;
		while (c)
		{
			if (ISVISIBLE(c) && !c->nofocus)
				break;
			c = c->next;
		}
		if(!c)
		{
			c = selmon->clients;
			while (c != selmon->sel)
			{
				if (ISVISIBLE(c) && !c->nofocus)
					break;
				c = c->next;
			}
		}
	}
	else {
		for(i = selmon->clients; i != selmon->sel; i = i->next)
			if(ISVISIBLE(i) && !i->nofocus)
				c = i;
		if(!c)
			for(; i; i = i->next)
				if(ISVISIBLE(i) && !i->nofocus)
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
	else if (prop == netatom[NetWMPID])
		req = XA_CARDINAL;

	if(XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
						  &da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		if(da == xatom[XembedInfo] && dl == 2)
			atom = ((Atom *)p)[1];
		XFree(p);
	}
	return atom;
}

unsigned long
getcolor(const char *colstr, XftColor *color) {
	Colormap cmap = DefaultColormap(dpy, screen);
	Visual *vis = DefaultVisual(dpy, screen);

	if(!XftColorAllocName(dpy,vis,cmap,colstr, color))
		die("error, cannot allocate color '%s'\n", colstr);
	return color->pixel;
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
	if (c && c->remap && c->win) {
		for(i = 0; c->remap[i].keysymto; ++i)
			for(j = 0; j < LENGTH(modifiers); j++) {
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
	}
}

void
grabbuttons(Client *c, Bool focused) {
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if(focused) {
			for(i = 0; i < LENGTH(buttons); i++)
				if(buttons[i].click == ClkClientWin)
					for(j = 0; j < LENGTH(modifiers); j++)
						XGrabButton(dpy, buttons[i].button,
									buttons[i].mask | modifiers[j],
									c->win, False, BUTTONMASK,
									GrabModeAsync, GrabModeSync, None, None);
			if (c->remap)
				for(i = 0; c->remap[i].keysymto; i++)
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
}

void
grabkeys(Window window) {
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		KeyCode code;

		XUngrabKey(dpy, AnyKey, AnyModifier, window);
		for(i = 0; i < LENGTH(keys); i++)
			if((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				for(j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], window,
						 True, GrabModeAsync, GrabModeAsync);
	}
}

void
initfont(const char *fontstr) {
	PangoFontMetrics *metrics;

	dc.pgc = pango_xft_get_context(dpy, screen);
	dc.pfd = pango_font_description_from_string(fontstr);

	metrics = pango_context_get_metrics(dc.pgc, dc.pfd, pango_language_from_string(setlocale(LC_CTYPE, "")));
	dc.font.ascent = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
	dc.font.descent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;

	pango_font_metrics_unref(metrics);

	dc.plo = pango_layout_new(dc.pgc);
	pango_layout_set_font_description(dc.plo, dc.pfd);

	dc.font.height = dc.font.ascent + dc.font.descent;
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

	ev = &e->xkey;
	keysym = XkbKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0, 0);
	for(i = 0; i < LENGTH(keys); i++)
		if(keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			keys[i].func(&(keys[i].arg));
	if (selmon->sel && selmon->sel->remap) {
		for(i = 0; selmon->sel->remap[i].keysymto; ++i)
			if (selmon->sel->remap[i].keysymfrom && selmon->sel->remap[i].keysymfrom == keysym)
				sendKey(XKeysymToKeycode(dpy, selmon->sel->remap[i].keysymto), selmon->sel->remap[i].modifier);
	}
}

void
killclient(const Arg *arg) {

	if(!selmon->sel)
		return;
	if(!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0 , 0, 0)) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
manage(Window w, XWindowAttributes *wa) {
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;
	int nc = 0;
	int bpx = 0;

	fprintf(stderr, "Creating client\n");
	if(!(c = calloc(1, sizeof(Client))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Client));
	c->win = w;
	updatetitle(c);
	if(XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
		applyrules(c);
	}
	else {
		c->mon = selmon;
		applyrules(c);
	}
	/* geometry */
	c->x = c->oldx = wa->x + c->mon->wx;
	c->y = c->oldy = wa->y + c->mon->wy;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;
	if(c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
		c->x = c->mon->mx + c->mon->mw - WIDTH(c);
	if(c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
		c->y = c->mon->my + c->mon->mh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx)
				&& (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);
	for(Client* cother = c->mon->clients; cother; cother = cother->next)
		if (ISVISIBLE(cother) && !cother->nofocus)
			++nc;
	if (nc > 0)
		bpx = c->mon->vs->lt[c->mon->vs->curlt]->borderpx;
	c->bw = c->noborder ? 0 : bpx;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, dc.norm[ColBorder]);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, False);
	if(!c->isfloating) {
		c->isfloating = c->oldstate = trans != None || c->isfixed;
		if (c->isfloating)
			centerclient(c);
	}
	if (c->nofocus)
		XLowerWindow(dpy, c->win);
	else if(c->isfloating)
		XRaiseWindow(dpy, c->win);
	attachabove(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
			(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	XMapWindow(dpy, c->win);
	setclientstate(c, NormalState);
	arrange(c->mon);
	grabremap(c, True);
	if (c->opacity != 1. && (!c->isfloating || c->nofocus))
		client_opacity_set(c, c->opacity);
	if (c->nofocus)
		unmanage(c, False);
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

void
monocle(Monitor *m) {
	unsigned int n = 0;
	Client *c;

	for(c = m->clients; c; c = c->next)
		if(ISVISIBLE(c) && !c->nofocus)
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
		selmon = m;
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

	if(!(c = selmon->sel))
		return;
	if(c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
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
				nx = ocx + (ev.xmotion.x - x);
				ny = ocy + (ev.xmotion.y - y);
				if(nx >= selmon->wx && nx <= selmon->wx + selmon->ww
						&& ny >= selmon->wy && ny <= selmon->wy + selmon->wh) {
					if(abs(selmon->wx - nx) < snap)
						nx = selmon->wx;
					else if(abs((selmon->wx + selmon->ww) - (nx + c->w + 2 * c->bw)) < snap)
						nx = selmon->wx + selmon->ww;
					if(abs(selmon->wy - ny) < snap)
						ny = selmon->wy;
					else if(abs((selmon->wy + selmon->wh) - (ny + c->h + 2 * c->bw)) < snap)
						ny = selmon->wy + selmon->wh;
				}
				resizefast(c, nx, ny, c->w, c->h);
			}
			if(!selmon->vs->lt[selmon->vs->curlt]->arrange || c->isfloating)
				resizefast(c, nx, ny, c->w, c->h);
			break;
		}
	} while(ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

Client *
nexttiled(Client *c) {
	for(; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
pop(Client *c) {
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
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
				if (c->isfloating)
					centerclient(c);
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
client_opacity_set(Client *c, double opacity)
{
	window_opacity_set(c->win, opacity);
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
resize(Client *c, int x, int y, int w, int h, Bool interact) {
	int halfgap = windowgap / 2;
	int nc;
	Bool changed = False;

	if (!c->isfloating) {
		changed = updateborderwidth(c->mon, &nc);
		if(nc > 1 && c->mon->vs->lt[c->mon->vs->curlt]->arrange && c->mon->vs->lt[c->mon->vs->curlt]->arrange != &monocle)
		{
			x += halfgap;
			y += halfgap;
			w -= windowgap;
			h -= windowgap;
		}
	}
	if (changed)
		arrange(c->mon);
	else
		if(applysizehints(c, &x, &y, &w, &h, interact))
			resizeclient(c, x, y, w, h);
}

void
resizebarwin(Monitor *m) {
	unsigned int w = m->ww;
	if(showsystray && m == systraytomon(m))
		w -= getsystraywidth();
	XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, w, bh);
}

void
resizefast(Client *c, int x, int y, int w, int h) {
	XWindowChanges wc;
	XEvent ev;
	Bool hadMotionEvents = False;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	while (XCheckMaskEvent(dpy, PointerMotionMask, &ev))
		hadMotionEvents = True;
	if (hadMotionEvents)
		XPutBackEvent(dpy, &ev);
	XSync(dpy, False);
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
				resizefast(c, c->x, c->y, nw, nh);
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
		selmon = m;
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
restack(Monitor *m) {
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if(!m->sel) {
		for(c = m->clients; c; c = c->next)
			if(!c->nofocus && ISVISIBLE(c))
				break;
		if (c && m == selmon)
			focus(c);
		else
			m->sel = c;
	}
	if(!m->sel)
		return;
	if(m->sel->nofocus)
		XLowerWindow(dpy, m->sel->win);
	else if(m->sel->isfloating || !m->vs->lt[m->vs->curlt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if(m->vs->lt[m->vs->curlt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for(c = m->stack; c; c = c->snext)
			if(!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while(XCheckMaskEvent(dpy, EnterWindowMask, &ev));
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
		if(handler[ev.type])
			handler[ev.type](&ev); /* call handler */
}

void
scan(void) {
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if(XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for(i = 0; i < num; i++) {
			if(!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if(wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
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
	if(c->mon == m)
		return;
	unfocus(c, True);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = m->vs->tagset; /* assign tags of target monitor */
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
	for(m = mons; m; m = m->next)
		cleartags(m);
}

void
setclientstate(Client *c, long state) {
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
			PropModeReplace, (unsigned char *)data, 2);
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
setfocus(Client *c) {
	if(!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
						XA_WINDOW, 32, PropModeReplace,
						(unsigned char *) &(c->win), 1);
	}
	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
}

void
setfullscreen(Client *c, Bool fullscreen) {
	Client *cother;

	if(fullscreen) {
		for(cother = c->mon->clients; cother; cother = cother->next)
			if (cother != c && cother->isfullscreen)
				return ;
		window_opacity_set(c->win, 1.);
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
						PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = True;
		c->oldstate = c->isfloating;
		c->oldbw = c->bw;
		c->bw = 0;
		c->isfloating = True;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	}
	else {
		window_opacity_set(c->win, c->opacity);
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
						PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = False;
		c->isfloating = c->oldstate;
		if (c->isfloating)
			centerclient(c);
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
	}
}

void
monsetlayout(Monitor *m, const void* v) {
	if(!v || v != m->vs->lt[m->vs->curlt])
		m->vs->curlt ^= 1;
	if(v)
		m->vs->lt[m->vs->curlt] = (Layout *)v;
	strncpy(m->ltsymbol, m->vs->lt[m->vs->curlt]->symbol, sizeof m->ltsymbol);
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

	spawnimpl(arg, True);
	readcolors();

	dc.norm[ColBG] = getcolor(normbgcolor, dc.xftnorm+ColBG);
	dc.norm[ColFG] = getcolor(normfgcolor, dc.xftnorm+ColFG);
	dc.sel[ColBG] = getcolor(selbgcolor, dc.xftsel+ColBG);
	dc.sel[ColFG] = getcolor(selfgcolor, dc.xftsel+ColFG);

	for(m = mons; m; m = m->next)
		drawbar(m);
	updatesystray();
	createclocks();
}

void
setup(void) {
	XSetWindowAttributes wa;

	/* clean up any zombies immediately */
	sigchld(0);

	/* read colors */
	readcolors();

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
	bh = dc.h = dc.font.height + 2;
	updategeom();
	/* init atoms */
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMPID] = XInternAtom(dpy, "_NET_WM_PID", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetWMStateSkipTaskbar] = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
	xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
	xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
	/* init cursors */
	cursor[CurNormal] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(dpy, XC_fleur);
	/* init appearance */
	dc.norm[ColBorder] = getcolor(normbordercolor, dc.xftnorm+ColBorder);
	dc.norm[ColBG] = getcolor(normbgcolor, dc.xftnorm+ColBG);
	dc.norm[ColFG] = getcolor(normfgcolor, dc.xftnorm+ColFG);
	dc.sel[ColBorder] = getcolor(selbordercolor, dc.xftsel+ColBorder);
	dc.sel[ColBG] = getcolor(selbgcolor, dc.xftsel+ColBG);
	dc.sel[ColFG] = getcolor(selfgcolor, dc.xftsel+ColFG);
	dc.drawable = XCreatePixmap(dpy, root, DisplayWidth(dpy, screen), bh, DefaultDepth(dpy, screen));
	dc.gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, dc.gc, 1, LineSolid, CapButt, JoinMiter);
	dc.xftdrawable = XftDrawCreate(dpy, dc.drawable, DefaultVisual(dpy,screen), DefaultColormap(dpy,screen));
	if(!dc.xftdrawable)
		printf("error, cannot create drawable\n");

	/* init system tray */
	updatesystray();
	updatebars();
	updatestatus();
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
			PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select for events */
	wa.cursor = cursor[CurNormal];
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask
					|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys(root);
}

void
showhide(Client *c) {
	if(!c)
		return;
	if(ISVISIBLE(c)) { /* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if((!c->mon->vs->lt[c->mon->vs->curlt]->arrange || c->isfloating) && (!c->isfullscreen || rotatingMons))
			resize(c, c->x, c->y, c->w, c->h, False);
		showhide(c->snext);
	}
	else { /* hide clients bottom up */
		showhide(c->snext);
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
spawnimpl(const Arg *arg, Bool waitdeath) {
	int childpid = fork();
	if(childpid == 0) {
		if(dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
	if (waitdeath) {
		waitpid(childpid, NULL, 0);
	}
}

void
spawn(const Arg *arg) {
	spawnimpl(arg, False);
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

int
textnw(const char *text, unsigned int len) {
	PangoRectangle r;
	pango_layout_set_text(dc.plo, text, len);
	pango_layout_get_extents(dc.plo, 0, &r);
	return r.width / PANGO_SCALE;
}

void
monshowbar(Monitor* m, Bool show) {
	m->vs->showbar = show;
	fprintf(stderr, "setting showbar @%d to %s\n", m->vs->tagset, m->vs->showbar ? "True" : "False");
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
togglebar(const Arg *arg) {
	monshowbar(selmon, !selmon->vs->showbar);
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
}

void
toggleview(const Arg *arg) {
	unsigned int newtagset = selmon->vs->tagset ^ (arg->ui & TAGMASK);

	/*if(newtagset) {*/
	if (selmon->vs->tagset != newtagset)
	{
		viewstackadd(selmon, newtagset, False);
		arrange(selmon);
	}
	/*}*/
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
		fprintf(stderr, "unmapped icon\n");
		removesystrayicon(c);
		resizebarwin(selmon);
		updatesystray();
	}
}

Bool
updateborderwidth(Monitor* m, int* nc) {
	Client* c;
	Client* first = NULL;
	Bool changed = False;

	*nc = 0;
	for(c = m->clients; m->vs->lt[m->vs->curlt] && c; c = c->next)
		if (ISVISIBLE(c) && !c->nofocus)
		{
			if (first == NULL)
			{
				if (!c->isfloating)
				{
					first = c;
					if (first->bw != 0)
					{
						changed = True;
						first->bw = 0;
					}
				}
			}
			else
			{
				if (!first->noborder)
				{
					if (changed)
						changed = False;
					first->bw = m->vs->lt[m->vs->curlt]->borderpx;
				}
				if (!c->noborder && c->bw != m->vs->lt[m->vs->curlt]->borderpx)
				{
					c->bw = m->vs->lt[m->vs->curlt]->borderpx;
					changed = True;
				}
			}
			if (!c->isfloating)
				++(*nc);
			configure(c);
		}
	return changed;
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
		window_opacity_set(m->barwin, barOpacity);
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
killclocks(void) {
	Monitor *m;
	const Arg arg = {.v = killclockscmd };
	spawnimpl(&arg, True);

	for(m = mons; m; m = m->next)
		m->hasclock = False;
}

void
createclocks(void) {
	Monitor* m;

	killclocks();
	for(m = mons; m; m = m->next) {
		const Arg arg = {.v = clockcmd };
		spawn(&arg);
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
		Monitor *m, *oldmons = mons;
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
			m->mx = m->wx = unique[i].x_org;
			m->my = m->wy = unique[i].y_org;
			m->mw = m->ww = unique[i].width;
			m->mh = m->wh = unique[i].height;
			updatebarpos(m);
		}
		free(unique);
		/* re-attach old clients and cleanup old monitor structure */
		while(oldmons) {
			m = oldmons;
			while(m->clients) {
				c = m->clients;
				m->clients = c->next;
				detachstack(c);
				c->mon = mons;
				attach(c);
				attachstack(c);
			}
			oldmons = m->next;
			cleanupmon(m);
		}
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
	selmon = mons;
	centerMouseInMonitorIndex(focusmonstart);
	selmon = wintomon(root);
	createclocks();
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
		const Arg arg = SHCMD("exec ~/hacks/scripts/updateDwmColor.sh");

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
		if (applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False))
			fprintf(stderr, "Size hint change to: %d %d %d %d\n", i->x, i->y, i->w, i->h);
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
		wa.border_pixel  = dc.norm[ColBG];
		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)&systrayorientation, 1);
		XChangeWindowAttributes(dpy, systray->win, CWEventMask|CWOverrideRedirect|CWBackPixel|CWBorderPixel, &wa);
		XMapRaised(dpy, systray->win);
		window_opacity_set(systray->win, barOpacity);
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
		wa.border_pixel  = dc.norm[ColBG];
		wa.background_pixmap = ParentRelative;
		XChangeWindowAttributes(dpy, i->win, CWBackPixel|CWBorderPixel|CWBackPixmap, &wa);
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

void
updatewindowtype(Client *c) {
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if(state == netatom[NetWMFullscreen])
		setfullscreen(c, True);

	if(wtype == netatom[NetWMWindowTypeDialog] || state == netatom[NetWMStateSkipTaskbar]) {
		c->isfloating = True;
		centerclient(c);
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
}

void
view(const Arg *arg) {
	monview(selmon, arg->ui);
	restorebar(selmon);
	arrange(selmon);
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

	if(w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for(m = mons; m; m = m->next)
		if(w == m->barwin)
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
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
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
	if(c == nexttiled(selmon->clients))
		if(!c || !(c = nexttiled(c->next)))
			return;
	pop(c);
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
	checkotherwm();
	setup();
	scan();
	startup = False;
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
