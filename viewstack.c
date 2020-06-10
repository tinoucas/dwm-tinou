Bool
hasclientson(Monitor *m, unsigned int tagset) {
	Client *c;
	unsigned int nc;

	for(c = m->clients, nc = 0; c; c = c->next)
		if(!c->nofocus && (c->tags & tagset) && c->tags != TAGMASK)
			++nc;
	return nc > 0;

}

Bool
movetostacktop(Monitor *m, unsigned int ui) {
	ViewStack *v = m->vs;
	ViewStack *vprev = NULL;

	while (v) {
		if (v->tagset == ui) {
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
	int i;

	vdst->tagset = vsrc->tagset;
	vdst->curlt = vsrc->curlt;
	vdst->lt[0] = vsrc->lt[0];
	vdst->lt[1] = vsrc->lt[1];
	vdst->mfact = vsrc->mfact;
	vdst->msplit = vsrc->msplit;
	for (i = 0; i < 3; ++i)
		vdst->ltaxis[i] = vsrc->ltaxis[i];
	vdst->showbar = vsrc->showbar;
	vdst->showdock = vsrc->showdock;
}

int
tagcount(unsigned int ui) {
	int n, i;

	for(n = 0, i = 0; i < numtags; ++i)
		if(ui & (1 << i))
			++n;
	return n;
}

ViewStack*
createviewstack (const ViewStack *vref, unsigned int ui) {
	ViewStack *v = (ViewStack*)calloc(1, sizeof(ViewStack));
	int i;

	if (vref != NULL)
		copyviewstack(v, vref);
	else {
		v->lt[0] = &layouts[initlayout];
		v->lt[1] = &layouts[FLOAT];
		v->showbar = showbar;
		v->showdock = showdock;
		v->mfact = mfact;
		for (i = 0; i < 3; ++i)
			v->ltaxis[i] = layoutaxis[i];
	}
	v->tagset = ui;
	if(v->lt[0] == &layouts[VARIMONO]) {
		v->msplit = tagcount(ui);
		if(v->msplit == 0)
			v->msplit = 1;
		else if(v->msplit > 2)
			v->msplit = 2;
	}
	else
		v->msplit = 1;
	return v;
}

ViewStack*
getviewstackof(Monitor* m, const unsigned int tagset) {
	ViewStack **pvs;

	for(pvs = &m->vs; *pvs && (*pvs)->tagset != tagset; pvs = &(*pvs)->next) ;
	if (!*pvs) {
		*pvs = createviewstack(m->vs, tagset);
	}
	return *pvs;
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
		v = createviewstack(newview ? NULL : vref, ui);
		v->next = m->vs;
		m->vs = v;
	}
}

void
storestackviewlayout (Monitor *m, unsigned int ui, const Layout* lt, Bool showdock) {
	ViewStack *v;
	ViewStack **pv = &m->vs;

	for(v = m->vs; v && v->tagset != ui; pv = &v->next, v = v->next) ;
	if (!v) {
		*pv = createviewstack(m->vs, ui);
		v = *pv;
	}
	if (v->lt[v->curlt] != lt)
		v->curlt ^= 1;
	v->lt[v->curlt] = lt;
	v->showdock = showdock;
}

void
rewindstack (const Arg *arg) {
	ViewStack *v;

	if (selmon->vs && selmon->vs->next) {
		v = selmon->vs;
		free(selmon->vs);
		selmon->vs = v;
		arrange(selmon);
	}
}

void
settagsetlayout(Monitor* m, unsigned int tagset, const Layout *lt)
{
	ViewStack *vs = getviewstackof(m, tagset);

	if (lt != vs->lt[vs->curlt])
		vs->curlt ^= 1;
	vs->lt[vs->curlt] = lt;
}

#if DEBUG_TAGSETS
void
printtagset (unsigned int ui) {
	int i;

	for(i = 0; i < numtags; ++i) {
		if (ui & (1 << i))
			fprintf(stderr, "%d ", i);
	}
	fprintf(stderr, "\n");
}
#endif

unsigned int
findtoggletagset (Monitor *m) {
	unsigned int toggletags = m->vs->tagset;
	unsigned int curseltags = m->vs->tagset;
	unsigned int tag = 0;
	ViewStack* v = m->vs;

#if DEBUG_TAGSETS
	fprintf(stderr, "stack:\n");
	while (v) {
		printtagset(v->tagset);
		v = v->next;
	}
#endif

	/* rewind stack for different non-empty tagset that share some tags */
	v = m->vs;
	while (toggletags == curseltags && v) {
		if ((v->tagset & curseltags) == 0 && hasclientson(m, v->tagset))
		{
			toggletags = v->tagset;
		}
		v = v->next;
	}
	/* rewind stack for different non-empty tagset that share some tags */
	v = m->vs;
	while (toggletags == curseltags && v) {
		if (v->tagset != curseltags && hasclientson(m, v->tagset))
		{
			toggletags = v->tagset;
		}
		v = v->next;
	}
	/* try any tag different than current */
	while (toggletags == curseltags && tag < numtags) {
		if (((1 << tag) & curseltags) == 0 && hasclientson(m, 1 << tag))
		{
			toggletags = (1 << tag);
		}
		++tag;
	}
	/* rewind stack for tagset (empty) with no tags in common */
	v = m->vs;
	while (toggletags == curseltags && v) {
		if ((v->tagset & curseltags) == 0)
		{
			toggletags = v->tagset;
		}
		v = v->next;
	}
	/* rewind stack for different tagset (empty) that share some tags */
	v = m->vs;
	while (toggletags == curseltags && v) {
		if (v->tagset != curseltags)
		{
			toggletags = v->tagset;
		}
		v = v->next;
	}
	return toggletags;
}

Bool
hasvclients (Monitor *m) {
	return hasclientson(m, vtag);
}
