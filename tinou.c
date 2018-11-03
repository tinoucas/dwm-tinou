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

void
allnonfloat(const Arg *arg) {
	Client* c;

	for(c = selmon->clients; c; c = c->next)
		c->isfloating = False;
	arrange(selmon);
}

void
togglepreview(const Arg *arg) {
	const int previewTags = (1 << 5 | 1 << 6);
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (previewTags & TAGMASK);
	const Arg viewArg = { .ui = newtagset };

	view(&viewArg);
	cleartags(selmon);
}

void
changemon(Client *c, Monitor *m) {
	if(c->mon == m)
		return;
	detach(c);
	detachstack(c);
	c->mon = m;
	attach(c);
	attachstack(c);
}

struct ClientListItem;
typedef struct ClientListItem ClientListItem;
struct ClientListItem {
	Client* c;
	ClientListItem* next;
};

void
movetomon (Bool allviews, Monitor *msrc, Monitor *mdst) {
	Client* c;
	ClientListItem* movingclients = (ClientListItem*)calloc(1, sizeof(ClientListItem));
	ClientListItem* item = movingclients;
	int i;

	for(c = msrc->stack; c; c = c->snext)
		if (allviews || (c->tags & c->mon->tagset[c->mon->seltags])) {
			item->c = c;
			item->next = (ClientListItem*)calloc(1, sizeof(ClientListItem));
			item = item->next;
		}
	for(item = movingclients; item && item->c; item = item->next)
		changemon(item->c, mdst);
	item = movingclients;
	while (item) {
		movingclients = item->next;
		free(item);
		item = movingclients;
	}
	mdst->seltags = msrc->seltags;
	mdst->sellt = msrc->sellt;
	for(i = 0; i < 2; ++i)
		mdst->tagset[i] = msrc->tagset[i];
}

void
swapmonitors(Bool allviews, Monitor *mon1, Monitor *mon2, Monitor *sparem) {
	movetomon(allviews, mon1, sparem);
	movetomon(allviews, mon2, mon1);
	movetomon(allviews, sparem, mon2);
}

void
rotatemonitor(const Arg* arg) {
	Monitor* sparem = createmon();
	Monitor* m;
	Monitor* nextm;
	Bool allviews = True; // (arg->i != 0);

	for(m = mons, nextm = mons->next; nextm; m = m->next, nextm = nextm->next) {
		if (!nextm)
			nextm = mons;
		swapmonitors(allviews, m, nextm, sparem);
	}
	free(sparem);
	arrange(NULL);
}

Bool
hasclientson(unsigned int tagset) {
	Client *c;
	unsigned int nc;

	for(c = selmon->clients, nc = 0; c; c = c->next)
		if(!c->nofocus && (c->tags & tagset))
			++nc;
	return nc > 0;

}

void
setseltags(unsigned int newtagset, Bool newset) {
	int i;

	selmon->tagset[selmon->seltags] = newtagset;
	if (hasclientson(newtagset)) {
		if (!newset)
			selmon->sparetagset[selmon->selsparetags^1] = newtagset;
		else {
			for(i = 0; i < 2; ++i)
				if (selmon->sparetagset[selmon->selsparetags^(i^1)] != newtagset) {
					selmon->sparetagset[selmon->selsparetags^i] = newtagset;
					break;
				}
			if (i < 2)
				selmon->selsparetags = (selmon->selsparetags^(i^1));
		}

	}
}

unsigned int
findsparetagset () {
	int i;
	unsigned int sparetags = 0;

	selmon->selsparetags ^= 1;
	for (i = 0; i < 2 && sparetags == 0; ++i) {
		if (hasclientson(selmon->sparetagset[selmon->selsparetags^i]) && selmon->sparetagset[selmon->selsparetags^i] != selmon->tagset[selmon->seltags^1])
			sparetags = selmon->sparetagset[selmon->selsparetags^i];
	}
	if (sparetags != 0)
		return sparetags;
	return selmon->tagset[selmon->seltags];
}
