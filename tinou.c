typedef struct ViewStack_s {
	struct ViewStack_s* next;
	struct ViewStack_s* previous;
	int view;
} ViewStack;

static ViewStack* viewstack = 0;
static ViewStack* viewstackbase = 0;
static int viewstackcount = 0;
static int viewstacksize = 1024;

static void
viewstackadd (int i) {
	ViewStack* newtip;
	ViewStack* oldbase = viewstackbase;

	/* Avoid consecutive duplicates */
	if (viewstack && ((viewstack->next && viewstack->next->view) == i
				|| viewstack->view == i))
		return;
	newtip = (ViewStack*)malloc(sizeof(ViewStack));
	newtip->view = i;
	newtip->next = viewstack ? viewstack->next : 0;
	newtip->previous = viewstack;

	if (viewstack) {
		if (viewstack->next)
			viewstack->next->previous = newtip;
		viewstack->next = newtip;
	}
	viewstack = newtip;
	if (!viewstackbase)
		viewstackbase = viewstack;
	if (viewstackcount < viewstacksize)
		++viewstackcount;
	else {
		if (oldbase == viewstack->previous)
			viewstack->previous = 0;
		viewstackbase = viewstackbase->next;
		viewstackbase->previous = 0;
		free(oldbase);
	}
}

static void
jumpviewstackin (const Arg *arg) {
	if (viewstack && viewstack->next) {
		viewstack = viewstack->next;
		selmon->tagset[selmon->seltags] = viewstack->view;
		arrange(selmon);
	}
}

static void
jumpviewstackout (const Arg *arg) {
	if (viewstack && viewstack->previous) {
		viewstack = viewstack->previous;
		selmon->tagset[selmon->seltags] = viewstack->view;
		arrange(selmon);
	}
}

static void
rotateclients (const Arg *arg) {
	Client *base = selmon->clients;
	Client **pc;
	Client *c;
	Client *loopbase = base;

	do
	{
		if (base) {
			if (selmon->lt[selmon->sellt]->arrange != &monocle) {
				if (arg->i > 0) {
					selmon->clients = base->next;
					pc = &selmon->clients;
					while (*pc)
						pc = &(*pc)->next;
					*pc = base;
					base->next = NULL;
				}
				else {
					c = selmon->clients;
					while (c->next)
						c = c->next;
					detach(c);
					attach(c);
				}
			}
			else {
				focusstack(arg);
			}
		}
		base = selmon->clients;
		if (base == loopbase)
			break;
	} while (base && (!ISVISIBLE(base) || base->nofocus));
	arrange(selmon);
}

#if 0
static void
togglemonocle(const Arg *arg) {
	/*static void (*arrange)(Monitor *) = NULL;*/
	static Layout* layout;
	Arg layoutarg;

	if (selmon->lt[selmon->sellt]->arrange != &monocle) {
		/*arrange = selmon->lt[selmon->sellt]->arrange;*/
		layout = (Layout*)selmon->lt[selmon->sellt];
		layoutarg.v = &layouts[MONOCLE];
		setlayout(&layoutarg);
	}
	else {
		layoutarg.v = layout;
		setlayout(&layoutarg);
	}
}
#endif

static void
ttbarclick(const Arg *arg) {
		focusstack(arg);
}

static void
viewscroll(const Arg *arg) {
	if (selmon->lt[selmon->sellt]->arrange != &monocle)
		setmfact(arg);
}

static void
opacitychange(const Arg *arg) {
	if (selmon->sel)
	{
		double opacity = selmon->sel->opacity;
		selmon->sel->opacity = MAX(0, MIN(1, opacity + arg->f));
		client_opacity_set(selmon->sel, selmon->sel->opacity);
	}
}

static void
focuslast(const Arg *arg) {
	if (lastclient)
		focus(lastclient);
}

static void
toggleviews(const Arg *arg) {
	toggleview(arg);
	cleartags(selmon);
	drawbar(selmon);
}

void
allnonfloat(const Arg *arg) {
	Client* c;

	for(c = selmon->clients; c; c = c->next)
		c->isfloating = False;
	arrange(selmon);
}
