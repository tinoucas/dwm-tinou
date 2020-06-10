
void
varimono(Monitor *m) {
	Client *c;
	unsigned int n, i;
	int x1 = m->wx, y1 = m->wy, h1 = m->wh, w1 = m->ww;
	int x2 = m->wx, y2 = m->wy, h2 = m->wh, w2 = m->ww;
	int ns = m->vs->msplit;
	int h, w;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), ++n);
	if(n == 0)
		return;
	if(ns > n)
		ns = n;
	if(ns > 1) {
		w1 *= m->vs->mfact;
		w2 -= w1;
		x2 += w1;
		for(i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), ++i)
			if(i > n - ns) {
				w = w2 - 2 * c->bw;
				h = h2 / (ns - 1) - 2 * c->bw;
				resize(c, x2, y2, w, h, False);
				y2 = c->y + HEIGHT(c);
			}
	}
	for(i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), ++i)
		if(i <= n - ns)
			resize(c, x1, y1, w1 - 2 * c->bw, h1 - 2 * c->bw, False);
	snprintf(m->ltsymbol, sizeof m->ltsymbol, "{%d}", n - ns);
}
