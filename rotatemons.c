struct ClientListItem;
typedef struct ClientListItem ClientListItem;
struct ClientListItem {
	Client* c;
	ClientListItem* next;
};

void
changemon(Client *c, Monitor *m) {
	int xo = 0, yo = 0;

	if(c->mon == m)
		return;
	if(c->isfloating) {
		xo = m->mx - c->mon->mx;
		yo = m->my - c->mon->my;
	}
	detach(c);
	detachstack(c);
	c->x += xo;
	c->y += yo;
	c->mon = m;
	attach(c);
	attachstack(c);
}

void
movetomon (const unsigned int views, Monitor *msrc, Monitor *mdst) {
	Client* c;
	ClientListItem* nextitem = NULL;
	ClientListItem* baseitem = NULL;
	ClientListItem* item = NULL;

	for(c = msrc->stack; c; c = c->snext)
		if (c->tags & views) {
			item = (ClientListItem*)calloc(1, sizeof(ClientListItem));
			item->c = c;
			item->next = nextitem;
			nextitem = item;
		}
	baseitem = item;
	for(item = baseitem; item && item->c; item = item->next) {
		changemon(item->c, mdst);
	}

	mdst->sel = msrc->sel;
	
	item = baseitem;
	while (item) {
		nextitem = item->next;
		free(item);
		item = nextitem;
	}

	mdst->topbar = msrc->topbar;
	mdst->clock = msrc->clock;

	if (views == ~0)
		mdst->vs = msrc->vs;
}

void
swapmonitors(const unsigned int views, Monitor *mon1, Monitor *mon2, Monitor *sparem) {
	movetomon(views, mon1, sparem);
	movetomon(views, mon2, mon1);
	movetomon(views, sparem, mon2);
}

void
rotatemonitor(const Arg* arg) {
	Monitor *m, *nextm, *lastm, *sparem = createmon();
	Bool allviews = (arg->i != 0);
	const ViewStack *vs;
	const unsigned int views = allviews ? ~0 : selmon->vs->tagset;
	int i;

	rotatingMons = True;
	/* rotate clients */
	for(m = mons, nextm = mons->next; nextm; m = m->next, nextm = nextm->next) {
		if (!nextm)
			nextm = mons;
		swapmonitors(views, m, nextm, sparem);
	}
	free(sparem);
	/* tidy monitors (num, tagset, lt) */
	for(lastm = mons; lastm && lastm->next; lastm = lastm->next) ;
	for (i = 0, m = mons; m; m = m->next, ++i) {
		m->num = i;
		if(!allviews) {
			if (hasclientson(m, views)) {
				monview(m, views);
				vs = getviewstackof(lastm, views);
				copyviewstack(m->vs, vs);
			}
			else if (!hasclientson(m, m->vs->tagset))
				monview(m, 0);
		}
		restorebar(m);
		arrange(m);
		lastm = m;
	}
	updatecurrentdesktop();
	rotatingMons = False;
}

