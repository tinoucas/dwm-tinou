static int nmasters[LENGTH(tags) + 1];

static void
setnmaster(const Arg *arg) {
	int n;

	if (!nmasters[curtag])
		nmasters[curtag] = NMASTER;
	if(arg->i) {
		n = nmasters[curtag] + arg->i;
		if(n < 1 || wh / n <= 2 * borderpx)
			return;
		nmasters[curtag] = n;
	}
	else
		nmasters[curtag] = NMASTER;
	arrange();
}

static Client *
ntilecol(Client* c, unsigned int n, int x, int y, int w, int h) {
	unsigned int i, d;

	for(i = 0; c && i < n; c = nexttiled(c->next), ++i) {
		resize(c, x, y, w - 2 * c->bw, h / (n - i) - ((i + 1 == n) ? 2 * c->bw : 0), resizehints[curtag]);
		d = c->h + 2 * c->bw;
		y += d;
		h -= d;
	}
	return c;
}

static void
ntile(void) {
	Client *c;
	unsigned int n, mw;

	if (!nmasters[curtag])
		nmasters[curtag] = NMASTER;
	for(n = 0, c = nexttiled(clients); c; c = nexttiled(c->next), ++n);
	c = nexttiled(clients);
	if (n == 0)
		return ;
	else if (n == 1)
		resize(c, wx - 1, wy - 1, ww, wh, resizehints[curtag]);
	else if(n <= nmasters[curtag])
		ntilecol(c, n, wx, wy, ww, wh);
	else {
		mw = mfact * ww;
		ntilecol(ntilecol(c, nmasters[curtag], wx, wy, mw, wh), n - nmasters[curtag], wx + mw, wy, ww - mw, wh);
	}
}
