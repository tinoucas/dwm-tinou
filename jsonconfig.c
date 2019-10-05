#include "nxjson.c"

#include "jsonruleparser.c"

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
	rule = rule->next;
	for (js = json->child; js; js = js->next)
		readRuleAttribute(js, rule);
	return rule;
}

static void readjsonrules (const struct nx_json *jsonRules) {
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
	userscript  = calloc(strlen(js->text_value) + 1, sizeof(char));
	strcpy(userscript, js->text_value);
}

static void readconfig () {
	const char* homedir = getenv("HOME");
	const char* relconfig = ".config/dwm/config.json";
	char* rulesFile = calloc(strlen(homedir) + strlen(relconfig) + 2, sizeof(char));
	char* content;
	const nx_json *json, *js;

	strcpy(rulesFile, homedir);
	strcat(rulesFile, "/");
	strcat(rulesFile, relconfig);

	content = load_file(rulesFile);

	if (content) {
		json = nx_json_parse_utf8(content);

		for (js = json->child; js; js = js->next) {
			if (!strcmp(js->key, "rules"))
				readjsonrules(js);
			else if (!strcmp(js->key, "tags"))
				readtags(js);
			else if (!strcmp(js->key, "font"))
				readfont(js);
			else if (!strcmp(js->key, "terminal"))
				readterminal(js);
			else if (!strcmp(js->key, "startupscript"))
				readstartupscript(js);
		}
		nx_json_free(json);
		free(content);
	}
	if (!font) {
		font = calloc(strlen(fallbackfont) + 1, sizeof(char));
		strcpy(font, fallbackfont);
	}
}
