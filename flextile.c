/* See LICENSE file for copyright and license details. */
/* © 2010 joten <joten@freenet.de> */

/* function declarations */
static void mirrorlayout(const Arg *arg);
static void rotatelayoutaxis(const Arg *arg);
static void shiftmastersplit(const Arg *arg);

void
mirrorlayout(const Arg *arg) {
	if(!selmon->vs->lt[selmon->vs->curlt]->arrange)
		return;
	selmon->vs->ltaxis[0] *= -1;
	arrange(selmon);
}

void
rotatelayoutaxis(const Arg *arg) {
	if(!selmon->vs->lt[selmon->vs->curlt]->arrange)
		return;
	if(arg->i == 0) {
		if(selmon->vs->ltaxis[0] > 0)
			selmon->vs->ltaxis[0] = selmon->vs->ltaxis[0] + 1 > 2 ? 1 : selmon->vs->ltaxis[0] + 1;
		else
			selmon->vs->ltaxis[0] = selmon->vs->ltaxis[0] - 1 < -2 ? -1 : selmon->vs->ltaxis[0] - 1;
	} else
		selmon->vs->ltaxis[arg->i] = selmon->vs->ltaxis[arg->i] + 1 > 3 ? 1 : selmon->vs->ltaxis[arg->i] + 1;
	arrange(selmon);
}

void
shiftmastersplitimpl(unsigned int i) {
	unsigned int n;
	Client *c;

	for(n = 0, c = nexttiled(selmon->clients); c; c = nexttiled(c->next), n++);
	if(!selmon->vs->lt[selmon->vs->curlt]->arrange || selmon->vs->msplit + i < 1 || selmon->vs->msplit + i > n)
		return;
	selmon->vs->msplit += i;
	arrange(selmon);
}

void
shiftmastersplit(const Arg *arg) {
	if(arg)
		shiftmastersplitimpl(arg->i);
}

void
tile(Monitor *m) {
	char sym1 = 61, sym2 = 93, sym3 = 61, sym;
	int x1 = m->wx, y1 = m->wy, h1 = m->wh, w1 = m->ww, X1 = x1 + w1, Y1 = y1 + h1;
	int x2 = m->wx, y2 = m->wy, h2 = m->wh, w2 = m->ww, X2 = x2 + w2, Y2 = y2 + h2;
	unsigned int i, n, n1, n2;
	Client *c;
	int h, w;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if(m->vs->msplit > n)
		m->vs->msplit = (n == 0) ? 1 : n;
	/* layout symbol */
	if(abs(m->vs->ltaxis[0]) == m->vs->ltaxis[1])    /* explicitly: ((abs(m->vs->ltaxis[0]) == 1 && m->vs->ltaxis[1] == 1) || (abs(m->vs->ltaxis[0]) == 2 && m->vs->ltaxis[1] == 2)) */
		sym1 = 124;
	if(abs(m->vs->ltaxis[0]) == m->vs->ltaxis[2])
		sym3 = 124;
	if(m->vs->ltaxis[1] == 3)
		sym1 = (n == 0) ? 0 : m->vs->msplit;
	if(m->vs->ltaxis[2] == 3)
		sym3 = (n == 0) ? 0 : n - m->vs->msplit;
	if(m->vs->ltaxis[0] < 0) {
		sym = sym1;
		sym1 = sym3;
		sym2 = 91;
		sym3 = sym;
	}
	if(m->vs->msplit == 1) {
		if(m->vs->ltaxis[0] > 0)
			sym1 = 91;
		else
			sym3 = 93;
	}
	if(m->vs->msplit > 1 && m->vs->ltaxis[1] == 3 && m->vs->ltaxis[2] == 3)
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "%d%c%d", sym1, sym2, sym3);
	else if((m->vs->msplit > 1 && m->vs->ltaxis[1] == 3 && m->vs->ltaxis[0] > 0) || (m->vs->ltaxis[2] == 3 && m->vs->ltaxis[0] < 0))
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "%d%c%c", sym1, sym2, sym3);
	else if((m->vs->ltaxis[2] == 3 && m->vs->ltaxis[0] > 0) || (m->vs->msplit > 1 && m->vs->ltaxis[1] == 3 && m->vs->ltaxis[0] < 0))
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "%c%c%d", sym1, sym2, sym3);
	else
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "%c%c%c", sym1, sym2, sym3);
	if(n == 0)
		return;
	/* master and stack area */
	if(abs(m->vs->ltaxis[0]) == 1 && n > m->vs->msplit) {
		w1 *= m->vs->mfact;
		w2 -= w1;
		x1 += (m->vs->ltaxis[0] < 0) ? w2 : 0;
		x2 += (m->vs->ltaxis[0] < 0) ? 0 : w1;
		X1 = x1 + w1;
		X2 = x2 + w2;
	} else if(abs(m->vs->ltaxis[0]) == 2 && n > m->vs->msplit) {
		h1 *= m->vs->mfact;
		h2 -= h1;
		y1 += (m->vs->ltaxis[0] < 0) ? h2 : 0;
		y2 += (m->vs->ltaxis[0] < 0) ? 0 : h1;
		Y1 = y1 + h1;
		Y2 = y2 + h2;
	}
	/* master */
	n1 = (m->vs->ltaxis[1] != 1 || w1 / m->vs->msplit < bh) ? 1 : m->vs->msplit;
	n2 = (m->vs->ltaxis[1] != 2 || h1 / m->vs->msplit < bh) ? 1 : m->vs->msplit;
	for(i = 0, c = nexttiled(m->clients); i < m->vs->msplit; c = nexttiled(c->next), i++) {
		resize(c, x1, y1, 
			(m->vs->ltaxis[1] == 1 && i + 1 == m->vs->msplit) ? X1 - x1 - 2 * c->bw : w1 / n1 - 2 * c->bw, 
			(m->vs->ltaxis[1] == 2 && i + 1 == m->vs->msplit) ? Y1 - y1 - 2 * c->bw : h1 / n2 - 2 * c->bw, False);
		if(n1 > 1)
			x1 = c->x + WIDTH(c);
		if(n2 > 1)
			y1 = c->y + HEIGHT(c);
	}
	/* stack */
	if(n > m->vs->msplit) {
		n1 = (m->vs->ltaxis[2] != 1 || w2 / (n - m->vs->msplit) < bh) ? 1 : n - m->vs->msplit;
		n2 = (m->vs->ltaxis[2] != 2 || h2 / (n - m->vs->msplit) < bh) ? 1 : n - m->vs->msplit;
		for(i = 0; c; c = nexttiled(c->next), i++) {
			w = (m->vs->ltaxis[2] == 1 && i + 1 == n - m->vs->msplit) ? X2 - x2 - 2 * c->bw : w2 / n1 - 2 * c->bw;
			h = (m->vs->ltaxis[2] == 2 && i + 1 == n - m->vs->msplit) ? Y2 - y2 - 2 * c->bw : h2 / n2 - 2 * c->bw;
			resize(c, x2, y2, w, h, False);
			if(n1 > 1)
				x2 = c->x + WIDTH(c);
			if(n2 > 1)
				y2 = c->y + HEIGHT(c);
		}
	}
}
