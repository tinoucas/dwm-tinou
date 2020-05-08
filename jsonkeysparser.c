static const char *pendingtagkeys = "tagkeys";

static void keymodifiersparser(const struct nx_json *json, Key *key) {
	key->mod = jsontomodifier(json);
}

static void keyparser(const struct nx_json *json, Key *key) {
	key->keysym = XStringToKeysym(json->text_value);
	if(key->keysym == NoSymbol)
		fprintf(stderr, "No such key: %s\n", json->text_value);
}

static void keyfunctionparser(const struct nx_json *json, Key *key) {
	if(!strcmp(json->text_value, pendingtagkeys)) {
		key->pending = calloc(strlen(pendingtagkeys) + 1, sizeof(char));
		strcpy(key->pending, pendingtagkeys);
	}
	else
		key->func = texttofunction(json->text_value);
}

static void keyargparser(const struct nx_json *json, Key *key) {
	argparser(json, &key->arg);
}

static void keyactionparser(const struct nx_json *json, Key *key) {
	const struct
	{
		const char *key;
		void (*parse)(const struct nx_json *js, Key *key);
	} parsers[] =
	{
		{ "function", keyfunctionparser },
		{ "arg"     , keyargparser      },
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
		{ "modifiers", keymodifiersparser },
		{ "key"      , keyparser          },
		{ "action"   , keyactionparser    },
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

static void readkeys (const struct nx_json *jsonKeys) {
	if (jsonKeys) {
		const struct nx_json *js = jsonKeys;
		Key* key;

		for(js = jsonKeys->child; js; js = js->next) {
			key = getNextKey(js);
			key->next = keys;
			keys = key;
		}
	}
}
