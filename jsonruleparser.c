static void copystring (char** pstr, const struct nx_json *js) {
	int length = strlen(js->text_value);

	*pstr = calloc(length + 1, sizeof(char));
	if (*js->text_value)
		strncpy(*pstr, js->text_value, length);
	else
		**pstr = 0;
}

static void classparser (const struct nx_json *js, Rule *rule) {
	copystring(&rule->class, js);
}

static void instanceparser (const struct nx_json *js, Rule *rule) {
	copystring(&rule->instance, js);
}

static void titleparser (const struct nx_json *js, Rule *rule) {
	copystring(&rule->title, js);
}

static void tagparser (const struct nx_json *js, Rule *rule) {
	int i;

	switch(js->type) {
	case NX_JSON_INTEGER:
		if (js->int_value == 0)
			rule->tags = ~0;
		else
			rule->tags = (1 << (js->int_value - 1));
		break;
	case NX_JSON_STRING:
		for (i = 0; i < numtags; ++i)
			if (!strcmp(js->text_value, tags[i])) {
				rule->tags = (1 << i);
				break;
			}
		break;
	default:
		fprintf(stderr, "Unsupported tag type '%d'\n", (int)js->type);
		break;
	}
}

static void isfloatingparser (const struct nx_json *js, Rule *rule) {
	rule->isfloating = (js->int_value != 0);
}

static void isterminalparser (const struct nx_json *js, Rule *rule) {
	rule->isterminal = js->int_value;
}

static void istransparentparser (const struct nx_json *js, Rule *rule) {
	rule->istransparent = js->dbl_value;
}

static void nofocusparser (const struct nx_json *js, Rule *rule) {
	rule->nofocus = (js->int_value != 0);
}

static void noborderparser (const struct nx_json *js, Rule *rule) {
	rule->noborder = (js->int_value != 0);
}

static void rhparser (const struct nx_json *js, Rule *rule) {
	rule->rh = (js->int_value != 0);
}

static void monitorparser (const struct nx_json *js, Rule *rule) {
	rule->monitor = js->int_value;
}

static void remapparser (const struct nx_json *js, Rule *rule) {
	int i;

	for (i = 0; i < LENGTH(remaps); ++i)
		if (!strcmp(remaps[i].name, js->text_value)) {
			rule->remap = remaps[i].remap;
			break;
		}
	if (i == LENGTH(remaps))
		fprintf(stderr, "Unknown remap '%s'\n", js->text_value);
}

static void preflayoutparser (const struct nx_json *js, Rule *rule) {
	int i;

	for (i = 0; i < LENGTH(layouts); ++i)
		if (!strcmp(layouts[i].name, js->text_value)) {
			rule->preflayout = &layouts[i];
			break;
		}
	if (i == LENGTH(layouts))
		fprintf(stderr, "Unknown layout '%s'\n", js->text_value);
}

static void istransientparser (const struct nx_json *js, Rule *rule) {
	rule->istransient = (js->int_value != 0);
}

static void procnameparser (const struct nx_json *js, Rule *rule) {
	copystring(&rule->procname, js);
}

static void isfullscreenparser (const struct nx_json *js, Rule *rule) {
	rule->isfullscreen = (js->int_value != 0);
}

static void showdockparser (const struct nx_json *js, Rule *rule) {
	rule->showdock = js->int_value;
}

static void picomfreezeparser (const struct nx_json *js, Rule *rule) {
	rule->picomfreeze = (js->int_value != 0);
}

static void iscenterparser (const struct nx_json *js, Rule *rule) {
    rule->iscenter = (js->int_value != 0);
}

static void exfocusparser (const struct nx_json *js, Rule *rule) {
	rule->exfocus = (js->int_value != 0);
}
