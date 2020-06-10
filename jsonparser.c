#include <strings.h>

typedef void (*ArgFunction)(const Arg*);

static ArgFunction texttofunction(const char *name) {
#define FUNC(fun) { #fun, fun }
	const struct
	{
		const char *name;
		ArgFunction func;
	}
	actions[] = {
		FUNC(allnonfloat),
		FUNC(decreasebright),
		FUNC(focuslast),
		FUNC(focusmon),
		FUNC(focusstack),
		FUNC(increasebright),
		FUNC(killclient),
		FUNC(mirrorlayout),
		FUNC(movemouse),
		FUNC(opacitychange),
		FUNC(pushdown),
		FUNC(pushup),
		FUNC(quit),
		FUNC(resizemouse),
		FUNC(rotatelayoutaxis),
		FUNC(rotatemonitor),
		FUNC(sendselkey),
		FUNC(setlayout),
		FUNC(setmfact),
		FUNC(shiftmastersplit),
		FUNC(spawn),
		FUNC(spawnterm),
		FUNC(tabview),
		FUNC(tag),
		FUNC(tagmon),
		FUNC(togglebar),
		FUNC(toggledock),
		FUNC(togglefloating),
		FUNC(togglefoldtags),
		FUNC(toggleswallow),
		FUNC(toggletag),
		FUNC(toggleview),
		FUNC(updatecolors),
		FUNC(view),
		FUNC(viewscroll),
		FUNC(zoom),
	};
	int i;
	ArgFunction func = NULL;

	for(i = 0; i < LENGTH(actions); ++i)
		if (!strcmp(actions[i].name, name)) {
			func = actions[i].func;
			break;
		}
	return func;
}

static unsigned int jsontomodifier(const struct nx_json *json) {
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
	unsigned int mod = 0;

	for(js = json->child; js; js = js->next)
		for(i = 0; i < LENGTH(modifiers); ++i)
			if(!strcasecmp(js->text_value, modifiers[i].name))
				mod |= modifiers[i].mod;
	return mod;
}

static const Layout *getlayout(const char *name) {
	const struct
	{
		const char* name;
		enum layout lt;
	}
	layoutnames[] = {
		{ "tile"     , TILE    },
		{ "spiral"   , SPIRAL  },
		{ "dwindle"  , DWINDLE },
		{ "float"    , FLOAT   },
		{ "monocle"  , MONOCLE },
		{ "varimono" , VARIMONO },
	};
	int i;

	for(i = 0; i < LENGTH(layoutnames); ++i)
		if(!strcmp(layoutnames[i].name, name))
			return &layouts[layoutnames[i].lt];
	fprintf(stderr, "layout not found: %s\n", name);
	return NULL;
}

static void argparser(const struct nx_json *json, Arg *arg) {
	switch(json->type) {
	case NX_JSON_STRING:
		arg->shcmd = calloc(strlen(json->text_value) + 1, sizeof(char));
		strcpy(arg->shcmd, json->text_value);
		break;
	case NX_JSON_DOUBLE:
		arg->f = (float)json->dbl_value;
		break;
	case NX_JSON_INTEGER:
	case NX_JSON_BOOL:
		arg->i = (int)json->int_value;
		break;
	case NX_JSON_NULL:
	case NX_JSON_OBJECT:
	case NX_JSON_ARRAY:
	default:
		break;
	}
}

