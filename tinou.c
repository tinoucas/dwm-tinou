static void
rhtoggle(const Arg *arg) {
	resizehints ^= 1;
	arrange();
}

static void
ttbarclick(const Arg *arg) {
	if (selmon->lt[selmon->sellt]->arrange == &monocle && arg)
		focusstack(arg);
	else
		setmfact(arg);
}

static void
kbmvresize(const Arg *arg) {
	if(!selmon->sel)
		return;
	if (!selmon->sel->isfloating)
		togglefloating(NULL);
	resize(selmon->sel, selmon->sel->x + ((int *)arg->v)[0],
			selmon->sel->y + ((int *)arg->v)[1],
			selmon->sel->w + ((int *)arg->v)[2],
			selmon->sel->h + ((int *)arg->v)[3], True);
}
