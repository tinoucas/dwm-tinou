void
removefromstack(Monitor *m, unsigned int ui) {
	ViewStack *v = m->viewstack;
	ViewStack *vdup, *vprev = NULL;

	while (v) {
		if (v->view == ui) {
			vdup = v;
			if (vprev)
				vprev->next = v->next;
			else
				m->viewstack = v->next;
			v = v->next;
			free(vdup);
		}
		else {
			vprev = v;
			v = v->next;
		}
	}
}

void
viewstackadd (Monitor *m, unsigned int ui, Bool newview) {
	if (newview || m->viewstack == NULL) {
		ViewStack* v = (ViewStack*)calloc(1, sizeof(ViewStack));

		removefromstack(m, ui);
		v->next = m->viewstack;
		m->viewstack = v;
	}
	m->viewstack->view = ui;
}

void
rewindstack (const Arg *arg) {
	ViewStack *v;

	if (selmon->viewstack && selmon->viewstack->next) {
		v = selmon->viewstack;
		free(selmon->viewstack);
		selmon->viewstack = v;
		selmon->tagset = selmon->viewstack->view;
		arrange(selmon);
	}
}

void
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
	int i, j;

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
		monview(mdst, msrc->tagset);
	monsetlayout(mdst, msrc->lt[msrc->sellt]);

	mdst->showbar = msrc->showbar;
	mdst->topbar = msrc->topbar;
	mdst->mfact = msrc->mfact;
	mdst->msplit = msrc->msplit;
	for(i = 0; i < LENGTH(tags) + 1; ++i) {
		mdst->msplits[i] = msrc->msplits[i];
		for (j = 0; j < 3; ++j)
			mdst->ltaxes[i][j] = msrc->ltaxes[i][j];
	}
	for (j = 0; j < 3; ++j)
		mdst->ltaxis[j] = msrc->ltaxis[j];
	mdst->hasclock = msrc->hasclock;
	mdst->viewstack = msrc->viewstack;
}

void
swapmonitors(unsigned int views, Monitor *mon1, Monitor *mon2, Monitor *sparem) {
	movetomon(views, mon1, sparem);
	movetomon(views, mon2, mon1);
	movetomon(views, sparem, mon2);
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
rotatemonitor(const Arg* arg) {
	Monitor* sparem = createmon();
	Monitor* m;
	Monitor* nextm;
	Bool allviews = (arg->i != 0);
	unsigned int views = allviews ? ~0 : selmon->tagset;
	int i;

	rotatingMons = True;
	for(m = mons, nextm = mons->next; nextm; m = m->next, nextm = nextm->next) {
		if (!nextm)
			nextm = mons;
		swapmonitors(views, m, nextm, sparem);
	}
	free(sparem);
	for (i = 0, m = mons; m; m = m->next, ++i) {
		m->num = i;
		if(m->showbar != m->showbars[m->curtag])
			montogglebar(m);
		if(m->tagset == vtag && !hasclientson(m, vtag))
			monview(m, 0);
		arrange(m);
	}
	rotatingMons = False;
}

void
setseltags(Monitor *m, unsigned int newtagset, Bool newview) {
	viewstackadd(m, m->tagset, newview);
	m->tagset = newtagset;
}

unsigned int
findsparetagset (Monitor *m) {
	unsigned int sparetags = 0;
	unsigned int curseltags = m->tagset;
	ViewStack* v = m->viewstack;
	int i = 0;

	while (sparetags == 0 && v)
	{
		if (v->view != curseltags && hasclientson(m, v->view))
			sparetags = v->view;
		v = v->next;
		++i;
	}
	if (sparetags != 0)
		return sparetags;
	fprintf(stderr, "findsparetagset stuck, stack size is %d\n", i);
	return m->tagset;
}

Bool
hasvclients (Monitor *m) {
	return hasclientson(m, vtag);
}
