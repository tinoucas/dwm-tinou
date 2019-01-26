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
		PARSE_ATTRIBUTE(tags),
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

static void readrules () {
	const char* homedir = getenv("HOME");
	const char* relconfig = ".config/dwm/rules.json";
	char* rulesFile = calloc(strlen(homedir) + strlen(relconfig) + 2, sizeof(char));
	char* content;
	const nx_json *json, *js;

	strcpy(rulesFile, homedir);
	strcat(rulesFile, "/");
	strcat(rulesFile, relconfig);

	content = load_file(rulesFile);

	if (content) {
		json = nx_json_parse_utf8(content);

		for (js = json->child; js; js = js->next)
			readjsonrules(js);
		nx_json_free(json);
		free(content);
	}
}
