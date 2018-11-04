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
		monview(mdst, msrc->vs->tagset);

	mdst->topbar = msrc->topbar;
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
		restorebar(m);
		if(m->vs->tagset == vtag && !hasclientson(m, vtag))
			monview(m, 0);
		arrange(m);
	}
	rotatingMons = False;
}

