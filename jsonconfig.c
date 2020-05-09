#include <sys/stat.h>
#include "nxjson.c"

#include "jsonruleparser.c"
#include "jsonparser.c"
#include "jsonkeysparser.c"
#include "jsonbuttonparser.c"

static void readRuleAttribute (const struct nx_json *js, Rule *rule) {
	int i;

#define PARSE_ATTRIBUTE(a) { #a , a ## parser }

	static const struct
	{
		const char *key;
		void (*parse)(const struct nx_json *js, Rule *rule);
	} parsers[] =
	{
		PARSE_ATTRIBUTE(class),
		PARSE_ATTRIBUTE(instance),
		PARSE_ATTRIBUTE(title),
		PARSE_ATTRIBUTE(tag),
		PARSE_ATTRIBUTE(isfloating),
		PARSE_ATTRIBUTE(isterminal),
		PARSE_ATTRIBUTE(noswallow),
		PARSE_ATTRIBUTE(istransparent),
		PARSE_ATTRIBUTE(nofocus),
		PARSE_ATTRIBUTE(noborder),
		PARSE_ATTRIBUTE(rh),
		PARSE_ATTRIBUTE(monitor),
		PARSE_ATTRIBUTE(remap),
		PARSE_ATTRIBUTE(preflayout),
		PARSE_ATTRIBUTE(istransient),
		PARSE_ATTRIBUTE(procname),
		PARSE_ATTRIBUTE(isfullscreen),
		PARSE_ATTRIBUTE(showdock),
	};

	for (i = 0; i < LENGTH(parsers); ++i)
		if (!strcmp(js->key, parsers[i].key)) {
			parsers[i].parse(js, rule);
			break;
		}
	if (i == LENGTH(parsers))
		fprintf(stderr, "No such rule attribute '%s'\n", js->key);
}

static Rule *callocrule () {
	Rule *rule = calloc(1, sizeof(Rule));

	memcpy(rule, &defaultrule, sizeof(Rule));
	return rule;
}

static Rule *getNextRule (const struct nx_json* json, Rule* rule) {
	const struct nx_json *js;

	rule->next = callocrule();
	for (js = json->child; js; js = js->next)
		readRuleAttribute(js, rule->next);
	return rule->next;
}

static void readrules (const struct nx_json *jsonRules) {
	if (jsonRules) {
		rules = callocrule();
		Rule *rule = rules;
		const struct nx_json *js = jsonRules;
	
		for (js = jsonRules->child; js; js = js->next)
			rule = getNextRule(js, rule);
	}
}

static char* load_file(const char* filepath) {
	struct stat st;
	if (stat(filepath, &st)==-1) {
		return 0;
	}
	int fd=open(filepath, O_RDONLY);
	if (fd==-1) {
		fprintf(stderr, "Can't open file '%s'", filepath);
		return 0;
	}
	char* text=malloc(st.st_size+1); // this is not going to be freed
	if (st.st_size!=read(fd, text, st.st_size)) {
		fprintf(stderr, "Can't read file '%s'", filepath);
		close(fd);
		return 0;
	}
	close(fd);
	text[st.st_size]='\0';
	return text;
}

static void readtags (const struct nx_json *jsontags) {
	numtags = jsontags->length;
	int i;
	struct nx_json *js;

	tags = calloc(numtags + 1, sizeof(char*));

	for (i = 0, js = jsontags->child; js; js = js->next, ++i) {
		tags[i] = calloc(strlen(js->text_value) + 1, sizeof(char));
		strcpy(tags[i], js->text_value);
		if(!strcmp(tags[i], "v"))
			vtag = 1 << i;
	}
}

static void readfont (const struct nx_json *js) {
	font = calloc(strlen(js->text_value) + 1, sizeof(char));
	strcpy(font, js->text_value);
}

static void readterminal (const struct nx_json *js) {
	terminal[0] = calloc(strlen(js->text_value) + 1, sizeof(char));
	strcpy(terminal[0], js->text_value);
}

static void readstartupscript (const struct nx_json *js) {
	userscript = calloc(strlen(js->text_value) + 1, sizeof(char));
	strcpy(userscript, js->text_value);
}

static void readdockposition (const struct nx_json *js) {
	const struct {
		const char* name;
		ScreenSide side;
	} sides[] = {
		{ "bottom", Bottom },
		{ "top",    Top    },
		{ "left",   Left   },
		{ "right",  Right  },
	};

	for (int i = 0; i < LENGTH(sides); ++i)
		if (!strcmp(js->text_value, sides[i].name))
			dockposition = sides[i].side;
}

static void readdockmonitor (const struct nx_json *js) {
    dockmonitor = js->int_value;
}

static void readpicomfreezeworkaround (const struct nx_json *js) {
	picomfreezeworkaround = (js->int_value != 0);
}

static void maketagkeys(Key *key, int tagnum) {
	const Key tagkeymappings[] =
	{
		{ key->mod,                       key->keysym, view,       {.ui = 1 << tagnum } , NULL },
		{ key->mod|ControlMask,           key->keysym, toggleview, {.ui = 1 << tagnum } , NULL },
		{ key->mod|ShiftMask,             key->keysym, tag,        {.ui = 1 << tagnum } , NULL },
		{ key->mod|ControlMask|ShiftMask, key->keysym, toggletag,  {.ui = 1 << tagnum } , NULL },
	};

	Key *n = key;
	Key *m = key->next;

	tagkeys[tagnum] = (char*)calloc(2, sizeof(char));
	tagkeys[tagnum][0] = (char)key->keysym;
	tagkeysmod = key->mod;
	for(int i = 0; i < LENGTH(tagkeymappings); ++i) {
		memcpy(n, &tagkeymappings[i], sizeof(struct Key));
		if(i < LENGTH(tagkeymappings) - 1) {
			n->next = callockey();
			n = n->next;
		}
	}
	n->next = m;
}

static void populatetagkeys() {
	Key *key = keys;
	int i;
	char *tagname;

	while(key) {
		if(key->pending && !strcmp(key->pending, pendingtagkeys)) {
			free(key->pending);
			key->pending = NULL;
			tagname = key->arg.shcmd;
			for(i = 0; i < numtags; ++i) {
				if (!strcmp(tagname, tags[i])){
					maketagkeys(key, i);
					free(tagname);
					break;
				}
			}
		}
		key = key->next;
	}
}

#define ATTRIBUTE(a) { #a, &read##a }

static void readconfig () {
	const char* homedir = getenv("HOME");
	const char* relconfig = ".config/dwm/config.json";
	char* rulesFile = calloc(strlen(homedir) + strlen(relconfig) + 2, sizeof(char));
	char* content;
	const nx_json *json, *js;
    int att, i;
	const struct
	{
		const char* tagname;
		void (*read)(const struct nx_json *js);
	}
	readattributes[] = {
        ATTRIBUTE(rules),
		ATTRIBUTE(tags),
		ATTRIBUTE(font),
		ATTRIBUTE(terminal),
		ATTRIBUTE(startupscript),
		ATTRIBUTE(dockposition),
		ATTRIBUTE(dockmonitor),
		ATTRIBUTE(picomfreezeworkaround),
		ATTRIBUTE(keys),
		ATTRIBUTE(buttons),
	};

	for(i = 0; i < LENGTH(tagkeys); ++i)
		tagkeys[i] = NULL;
	strcpy(rulesFile, homedir);
	strcat(rulesFile, "/");
	strcat(rulesFile, relconfig);

	content = load_file(rulesFile);

	if (content) {
		json = nx_json_parse_utf8(content);

		for (js = json->child; js; js = js->next) {
            for (att = 0; att < LENGTH(readattributes); ++att)
                if (!strcmp(js->key, readattributes[att].tagname))
                    (*readattributes[att].read)(js);
		}
		nx_json_free(json);
		free(content);
	}
	if (!font) {
		font = calloc(strlen(fallbackfont) + 1, sizeof(char));
		strcpy(font, fallbackfont);
	}
	populatetagkeys();
}
