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
movetomon (unsigned int views, Monitor *msrc, Monitor *mdst) {
	Client* c;
	ClientListItem* movingclients = (ClientListItem*)calloc(1, sizeof(ClientListItem));
	ClientListItem* item = movingclients;

	for(c = msrc->stack; c; c = c->snext)
		if (c->tags & views) {
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

	if (views != ~0)
		monview(mdst, views);
	else
		monview(mdst, msrc->tagset[msrc->seltags]);
	monsetlayout(mdst, msrc->lt[msrc->sellt]);
}

void
swapmonitors(unsigned int views, Monitor *mon1, Monitor *mon2, Monitor *sparem) {
	movetomon(views, mon1, sparem);
	movetomon(views, mon2, mon1);
	movetomon(views, sparem, mon2);
}

void
rotatemonitor(const Arg* arg) {
	Monitor* sparem = createmon();
	Monitor* m;
	Monitor* nextm;
	Bool allviews = (arg->i != 0);
	unsigned int views = allviews ? ~0 : selmon->tagset[selmon->seltags];

	for(m = mons, nextm = mons->next; nextm; m = m->next, nextm = nextm->next) {
		if (!nextm)
			nextm = mons;
		swapmonitors(views, m, nextm, sparem);
	}
	free(sparem);
	for (m = mons; m; m = m->next) {
		if(m->showbar != m->showbars[m->curtag])
			montogglebar(m);
		arrange(m);
	}
}

Bool
hasclientson(Monitor *m, unsigned int tagset) {
	Client *c;
	unsigned int nc;

	for(c = m->clients, nc = 0; c; c = c->next)
		if(!c->nofocus && (c->tags & tagset))
			++nc;
	return nc > 0;

}

void
setseltags(Monitor *m, unsigned int newtagset, Bool newset) {
	int i;
	Bool subset = (!newset || (m->tagset[m->seltags^1] & newtagset));

	m->tagset[m->seltags] = newtagset;
	if (hasclientson(m, newtagset)) {
		if (subset)
			m->sparetagset[m->selsparetags^1] = newtagset;
		else {
			for(i = 0; i < 2; ++i)
				if (m->sparetagset[m->selsparetags^(i^1)] != newtagset) {
					m->sparetagset[m->selsparetags^i] = newtagset;
					break;
				}
			if (i < 2)
				m->selsparetags = (m->selsparetags^(i^1));
		}

	}
}

unsigned int
findsparetagset (Monitor *m) {
	int i;
	unsigned int sparetags = 0;

	m->selsparetags ^= 1;
	for (i = 0; i < 2 && sparetags == 0; ++i) {
		if (hasclientson(m, m->sparetagset[m->selsparetags^i]) && m->sparetagset[m->selsparetags^i] != m->tagset[m->seltags^1])
			sparetags = m->sparetagset[m->selsparetags^i];
	}
	if (sparetags != 0)
		return sparetags;
	return m->tagset[m->seltags];
}
