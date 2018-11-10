struct ClientListItem;
typedef struct ClientListItem ClientListItem;
struct ClientListItem {
	Client* c;
	ClientListItem* next;
};

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
movetomon (unsigned int views, Monitor *msrc, Monitor *mdst) {
	Client* c;
	ClientListItem* nextitem = NULL;
	ClientListItem* baseitem = NULL;
	ClientListItem* item;


	for(c = msrc->stack; c; c = c->snext)
		if (c->tags & views) {
			fprintf(stderr, "add client '%s' to list\n", c->name);
			item = (ClientListItem*)calloc(1, sizeof(ClientListItem));
			item->c = c;
			item->next = nextitem;
			nextitem = item;
		}
	baseitem = item;
	for(item = baseitem; item && item->c; item = item->next) {
		fprintf(stderr, "changeing mon of client '%s'\n", item->c->name);
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
	mdst->hasclock = msrc->hasclock;

	if (views == ~0)
		mdst->vs = msrc->vs;
	else if (views == msrc->vs->tagset)
		monview(mdst, msrc->vs->tagset);
	monsetlayout(mdst, msrc->vs->lt[msrc->vs->curlt]);
	if (views != ~0)
		copyviewstack(mdst->vs, msrc->vs);
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
	unsigned int views = allviews ? ~0 : selmon->vs->tagset;
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
		if(!allviews && !hasclientson(m, m->vs->tagset))
			monview(m, 0);
		restorebar(m);
		arrange(m);
	}
	rotatingMons = False;
}

