static void buttonfunctionparser(const struct nx_json *js, Button *button) {
    button->func = texttofunction(js->text_value);
}

static void buttonargparser(const struct nx_json *js, Button *button) {
    argparser(js, &button->arg);
}

static void buttonactionparser(const struct nx_json *json, Button *button) {
	const struct
	{
		const char *key;
		void (*parse)(const struct nx_json *js, Button *button);
	} parsers[] =
	{
		{ "function", buttonfunctionparser },
		{ "arg"     , buttonargparser      },
	};
    int i;
	const struct nx_json *js;
    char *shcmd;

	for (js = json->child; js; js = js->next)
        for (i = 0; i < LENGTH(parsers); ++i)
            if (!strcmp(js->key, parsers[i].key))
                parsers[i].parse(js, button);
    if(button->arg.shcmd) {
        if (button->arg.shcmd && argislayout(button->func)) {
            shcmd = button->arg.shcmd;
            button->arg.v = (void*)getlayout(button->arg.shcmd);
            free(shcmd);
        }
        else if (button->func == &sendselkey) {
            shcmd = button->arg.shcmd;
            button->arg.keysym = XStringToKeysym(shcmd);
            free(shcmd);
        }
    }
}

static void buttonnumparser(const struct nx_json *js, Button *button) {
    button->button = (unsigned int)js->int_value + Button1 - 1;
}

static void buttonclickparser(const struct nx_json *js, Button *button) {
#define CLK(type) { #type, Clk ## type }
    const struct
    {
        const char* name;
        unsigned int click;
    }
    clicktypes[] =
    {
        CLK(TagBar),
        CLK(LtSymbol),
        CLK(StatusText),
        CLK(WinTitle),
        CLK(ClientWin),
        CLK(RootWin),
    };
    int i;

    for(i = 0; i < LENGTH(clicktypes); ++i)
        if (!strcasecmp(js->text_value, clicktypes[i].name)) {
            button->click = clicktypes[i].click;
            break;
        }
}

static void buttonmodifierparser(const struct nx_json *js, Button *button) {
    button->mask = jsontomodifier(js);
}

static void readButtonAttribute(const struct nx_json *js, Button *button) {
	const struct
	{
		const char *key;
		void (*parse)(const struct nx_json *js, Button *button);
	} parsers[] =
	{
		{ "modifiers" , buttonmodifierparser },
		{ "click"     , buttonclickparser    },
		{ "button"    , buttonnumparser      },
		{ "action"    , buttonactionparser   },
	};
	int i;

	for (i = 0; i < LENGTH(parsers); ++i)
		if (!strcmp(js->key, parsers[i].key)) {
			parsers[i].parse(js, button);
			break;
		}
}

static Button *callocbutton() {
	Button *button = calloc(1, sizeof(Button));

	return button;
}

static Button *getNextButton(const struct nx_json *json) {
	Button *button = callocbutton();
	const struct nx_json *js;

	for (js = json->child; js; js = js->next)
		readButtonAttribute(js, button);
	return button;
}

static void readbuttons(const struct nx_json *jsonButtons) {
	if(jsonButtons) {
		const struct nx_json *js = jsonButtons;
		Button *button;

		for(js = jsonButtons->child; js; js = js->next) {
			button = getNextButton(js);
			button->next = buttons;
			buttons = button;
		}
	}
}
