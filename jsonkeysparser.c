#include <strings.h>

static const char *pendingtagkeys = "tagkeys";

static void modifiersparser(const struct nx_json *json, Key *key) {
#define MODIFIER(mod) { #mod, mod ## Mask }
	const struct
	{
		const char* name;
		unsigned int mod;
	}
	modifiers[] = {
		MODIFIER(Shift),
		MODIFIER(Lock),
		MODIFIER(Control),
		MODIFIER(Mod1),
		MODIFIER(Mod2),
		MODIFIER(Mod3),
		MODIFIER(Mod4),
		MODIFIER(Mod5),
	};
	int i;
	const struct nx_json *js;

	for(js = json->child; js; js = js->next)
		for(i = 0; i < LENGTH(modifiers); ++i)
			if(!strcasecmp(js->text_value, modifiers[i].name))
				key->mod |= modifiers[i].mod;
}

static void keyparser(const struct nx_json *json, Key *key) {
	key->keysym = XStringToKeysym(json->text_value);
	if(key->keysym == NoSymbol)
		fprintf(stderr, "No such key: %s\n", json->text_value);
}

static void functionparser(const struct nx_json *json, Key *key) {
#define ACTION(fun) { #fun, fun }
	const struct
	{
		const char *name;
		void (*func)(const Arg *);
	}
	actions[] = {
		ACTION(allnonfloat),
		ACTION(focuslast),
		ACTION(focusmon),
		ACTION(focusstack),
		ACTION(killclient),
		ACTION(mirrorlayout),
		ACTION(pushdown),
		ACTION(pushup),
		ACTION(quit),
		ACTION(rotatelayoutaxis),
		ACTION(rotatemonitor),
		ACTION(setlayout),
		ACTION(setmfact),
		ACTION(shiftmastersplit),
		ACTION(spawn),
		ACTION(spawnterm),
		ACTION(tagmon),
		ACTION(togglebar),
		ACTION(toggledock),
		ACTION(togglefloating),
		ACTION(togglefoldtags),
		ACTION(toggleswallow),
		ACTION(updatecolors),
		ACTION(view),
		ACTION(zoom),
	};
	int i;

	if(!strcmp(json->text_value, pendingtagkeys)) {
		key->pending = calloc(strlen(pendingtagkeys) + 1, sizeof(char));
		strcpy(key->pending, pendingtagkeys);
	}
	else for(i = 0; i < LENGTH(actions); ++i)
		if (!strcmp(actions[i].name, json->text_value)) {
			key->func = actions[i].func;
			break;
		}
}

static const Layout *getlayout(const char *name) {
	const struct
	{
		const char* name;
		enum layout lt;
	}
	layoutnames[] = {
		{ "tile"    , TILE    },
		{ "spiral"  , SPIRAL  },
		{ "dwindle" , DWINDLE },
		{ "float"   , FLOAT   },
		{ "monocle" , MONOCLE },
	};
    int i;

    for(i = 0; i < LENGTH(layoutnames); ++i)
        if(!strcmp(layoutnames[i].name, name))
            return &layouts[layoutnames[i].lt];
    fprintf(stderr, "layout not found: %s\n", name);
    return NULL;
}

static void argparser(const struct nx_json *json, Key *key) {
	switch(json->type) {
	case NX_JSON_STRING:
		key->arg.shcmd = calloc(strlen(json->text_value) + 1, sizeof(char));
        strcpy(key->arg.shcmd, json->text_value);
		break;
	case NX_JSON_DOUBLE:
		key->arg.f = (float)json->dbl_value;
		break;
	case NX_JSON_INTEGER:
	case NX_JSON_BOOL:
		key->arg.i = (int)json->int_value;
		break;
	case NX_JSON_NULL:
	case NX_JSON_OBJECT:
	case NX_JSON_ARRAY:
	default:
        break;
	}
}

static void actionparser(const struct nx_json *json, Key *key) {
	const struct
	{
		const char *key;
		void (*parse)(const struct nx_json *js, Key *key);
	} parsers[] =
	{
		{ "function", functionparser },
		{ "arg"     , argparser      },
	};
    int i;
	const struct nx_json *js;
    char *shcmd;

	for (js = json->child; js; js = js->next)
        for (i = 0; i < LENGTH(parsers); ++i)
            if (!strcmp(js->key, parsers[i].key))
                parsers[i].parse(js, key);
    if(key->arg.shcmd && key->func == &setlayout) {
        shcmd = key->arg.shcmd;
        key->arg.v = (void*)getlayout(key->arg.shcmd);
        free(shcmd);
    }
}

static void readKeyAttribute(const struct nx_json *js, Key *key) {
	const struct
	{
		const char *key;
		void (*parse)(const struct nx_json *js, Key *key);
	} parsers[] =
	{
		{ "modifiers", modifiersparser },
		{ "key"      , keyparser       },
		{ "action"   , actionparser    },
	};
	int i;

	for (i = 0; i < LENGTH(parsers); ++i)
		if (!strcmp(js->key, parsers[i].key)) {
			parsers[i].parse(js, key);
			break;
		}
}

static Key *callockey() {
	Key *key = calloc(1, sizeof(Key));

	return key;
}

static Key *getNextKey (const struct nx_json *json) {
	Key *key = callockey();
	const struct nx_json *js;

	for(js = json->child; js; js = js->next)
		readKeyAttribute(js, key);
	return key;
}

static void maketagkeys(Key *key) {
	const Key tagkeys[] =
	{
		{ key->mod,                       key->keysym, view,       {.ui = 1 << key->arg.ui} , NULL },
		{ key->mod|ControlMask,           key->keysym, toggleview, {.ui = 1 << key->arg.ui} , NULL },
		{ key->mod|ShiftMask,             key->keysym, tag,        {.ui = 1 << key->arg.ui} , NULL },
		{ key->mod|ControlMask|ShiftMask, key->keysym, toggletag,  {.ui = 1 << key->arg.ui} , NULL },
	};

	Key *n = key;
    Key *m = key->next;

	for(int i = 0; i < LENGTH(tagkeys); ++i) {
		memcpy(n, &tagkeys[i], sizeof(struct Key));
        if(i < LENGTH(tagkeys) - 1) {
            n->next = callockey();
            n = n->next;
        }
	}
    n->next = m;
}

static void populatetagkeys() {
	Key *key = keys;

	while(key) {
		if(key->pending && !strcmp(key->pending, pendingtagkeys)) {
			maketagkeys(key);
		}
		key = key->next;
	}
}

static void readkeys (const struct nx_json *jsonKeys) {
	if (jsonKeys) {
		const struct nx_json *js = jsonKeys;
		Key* key;

		for(js = jsonKeys->child; js; js = js->next) {
			key = getNextKey(js);
			key->next = keys;
			keys = key;
		}
		populatetagkeys();
	}
}
