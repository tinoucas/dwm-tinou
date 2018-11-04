Bool
movetostacktop(Monitor *m, unsigned int ui) {
	ViewStack *v = m->vs;
	ViewStack *vprev = NULL;

	while (v) {
		if (v->view == ui) {
			if (v != m->vs) {
				if (vprev)
					vprev->next = v->next;
				v->next = m->vs;
				m->vs = v;
			}
			return True;
		}
		else {
			vprev = v;
			v = v->next;
		}
	}
	return False;
}

void
copyviewstack(ViewStack *vdst, const ViewStack *vsrc) {
	vdst->view = vsrc->view;
	vdst->curlt = vsrc->curlt;
	vdst->lt[0] = vsrc->lt[0];
	vdst->lt[1] = vsrc->lt[1];
	vdst->showbar = vsrc->showbar;
}

ViewStack*
createviewstack (Monitor *m, const ViewStack *vref) {
	ViewStack *v = (ViewStack*)calloc(1, sizeof(ViewStack));

	v->lt[0] = &layouts[initlayout];
	v->lt[1] = &layouts[1 % LENGTH(layouts)];
	if (vref != NULL)
		copyviewstack(v, vref);
	else {
		v->showbar = showbar;
		v->view = 1;
	}
	return v;
}

void
cleanupviewstack(ViewStack *v) {
	ViewStack *vnext;

	while(v) {
		vnext = v->next;
		free(v);
		v = vnext;
	}
}

void
movetoptoend(Monitor *m) {
	ViewStack** pv;

	for(pv = &m->vs; *pv; pv = &(*pv)->next) ;
	*pv = m->vs;
	m->vs = m->vs->next;
	(*pv)->next = NULL;
}

void
viewstackadd(Monitor *m, unsigned int ui, Bool newview) {
	ViewStack* v;
	ViewStack* vref = m->vs;

	if (!newview) {
		movetoptoend(m);
	}
	if (!movetostacktop(m, ui)) {
		v = createviewstack(m, vref);
		v->next = m->vs;
		m->vs = v;
		m->vs->view = ui;
	}
}

void
storestackviewlayout (Monitor *m, unsigned int ui, const Layout* lt) {
	ViewStack *v;
	ViewStack **pv = &m->vs;

	for(v = m->vs; v && v->view != ui; pv = &v->next, v = v->next) ;
	if (!v) {
		*pv = createviewstack(m, m->vs);
		v = *pv;
		v->view = ui;
	}
	if (v->lt[v->curlt] == lt)
		v->curlt ^= 1;
	v->lt[v->curlt] = lt;
}

void
rewindstack (const Arg *arg) {
	ViewStack *v;

	if (selmon->vs && selmon->vs->next) {
		v = selmon->vs;
		free(selmon->vs);
		selmon->vs = v;
		selmon->tagset = selmon->vs->view;
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
			if (selmon->vs->lt[selmon->vs->curlt]->arrange != &monocle) {
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
	if (selmon->vs->lt[selmon->vs->curlt]->arrange != &monocle)
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

void
duplicateviewstack(Monitor *mdst, Monitor *msrc) {
	ViewStack *vdst = createviewstack(mdst, msrc->vs), *vsrc;
	ViewStack **pv = &mdst->vs;

	vsrc = msrc->vs;
	cleanupviewstack(mdst->vs);
	while (vsrc) {
		*pv = vdst;
		vdst = *pv;
		copyviewstack(vdst, vsrc);
		vsrc = vsrc->next;
		vdst->next = createviewstack(mdst, vsrc);
		pv = &vsrc->next;
	}
	cleanupviewstack(vdst);
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
	if (views == ~0) {
		duplicateviewstack(mdst, msrc);
	}
	monsetlayout(mdst, msrc->vs->lt[msrc->vs->curlt]);
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
		restorebar(m);
		if(m->tagset == vtag && !hasclientson(m, vtag))
			monview(m, 0);
		arrange(m);
	}
	rotatingMons = False;
}

void
setseltags(Monitor *m, unsigned int newtagset, Bool newview) {
	viewstackadd(m, newtagset, newview);
	m->tagset = newtagset;
}

unsigned int
findtoggletagset (Monitor *m) {
	unsigned int toggletags = m->tagset;
	unsigned int curseltags = m->tagset;
	unsigned int tag = 0;
	ViewStack* v = m->vs;

	while (toggletags == curseltags && v) {
		if ((v->view & curseltags) == 0 && hasclientson(m, v->view))
			toggletags = v->view;
		v = v->next;
	}
	v = m->vs;
	while (toggletags == curseltags && v) {
		if (v->view != curseltags && hasclientson(m, v->view))
			toggletags = v->view;
		v = v->next;
	}
	v = m->vs;
	while (toggletags == curseltags && v) {
		if (v->view != curseltags)
			toggletags = v->view;
		v = v->next;
	}
	while (toggletags == curseltags && tag < LENGTH(tags)) {
		if (((1 << tag) & curseltags) == 0 && hasclientson(m, 1 << tag))
			toggletags = (1 << tag);
		++tag;
	}
	return toggletags;
}

Bool
hasvclients (Monitor *m) {
	return hasclientson(m, vtag);
}
