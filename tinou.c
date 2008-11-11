
static void
ttbarclick(const Arg *arg) {
	if (lt[sellt]->arrange == &monocle && arg)
		focusstack(arg);
	else
	{
		if (arg && arg->i == 1) { // XXX right button click on title bar, from config.h : { ClkWinTitle, 0, Button3, ttbarclick, {.i = +1 } },
			togglefloating(NULL);
		}
		else {
			zoom(NULL);
		}
	}
}

static void
kbmvresize(const Arg *arg) {
	if(!sel) // || (lt[sellt]->arrange) && !sel->isfloating)) -> move floatings ?
		return;

	if (!sel->isfloating)
		togglefloating(NULL);

	resize(sel, sel->x + ((int *)arg->v)[0],
	            sel->y + ((int *)arg->v)[1],
	            sel->w + ((int *)arg->v)[2],
	            sel->h + ((int *)arg->v)[3], True);
}

static void
rhtoggle(const Arg *arg) {
	resizehints ^= 1;
	arrange();
}
