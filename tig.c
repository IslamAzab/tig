 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define VERSION	"tig-0.3"
#if __GNUC__ >= 3
#define __NORETURN __attribute__((__noreturn__))
#else
#define __NORETURN
#endif

static void __NORETURN die(const char *err, ...);
static int read_properties(FILE *pipe, const char *separators, int (*read)(char *, int, char *, int));
static size_t utf8_length(const char *string, size_t max_width, int *coloffset, int *trimmed);
#define SIZEOF_REVGRAPH	19	/* Size of revision ancestry graphics. */
#define AUTHOR_COLS	20

#define TIG_LS_REMOTE \
	"git ls-remote . 2>/dev/null"

#define TIG_DIFF_CMD \
	"git show --patch-with-stat --find-copies-harder -B -C %s"

#define TIG_LOG_CMD	\
	"git log --cc --stat -n100 %s"

#define TIG_MAIN_CMD \
	"git log --topo-order --stat --pretty=raw %s"

/* XXX: Needs to be defined to the empty string. */
#define TIG_HELP_CMD	""
#define TIG_PAGER_CMD	""

static struct ref **get_refs(char *id);

struct int_map {
	const char *name;
	int namelen;
	int value;
static int
set_from_int_map(struct int_map *map, size_t map_size,
		 int *value, const char *name, int namelen)
{

	int i;

	for (i = 0; i < map_size; i++)
		if (namelen == map[i].namelen &&
		    !strncasecmp(name, map[i].name, namelen)) {
			*value = map[i].value;
			return OK;
		}

	return ERR;
}

string_ncopy(char *dst, const char *src, int dstlen)
static char *
chomp_string(char *name)
{
	int namelen;

	while (isspace(*name))
		name++;

	namelen = strlen(name) - 1;
	while (namelen > 0 && isspace(name[namelen]))
		name[namelen--] = 0;

	return name;
}

static bool
string_nformat(char *buf, size_t bufsize, int *bufpos, const char *fmt, ...)
{
	va_list args;
	int pos = bufpos ? *bufpos : 0;

	va_start(args, fmt);
	pos += vsnprintf(buf + pos, bufsize - pos, fmt, args);
	va_end(args);

	if (bufpos)
		*bufpos = pos;

	return pos >= bufsize ? FALSE : TRUE;
}

#define string_format(buf, fmt, args...) \
	string_nformat(buf, sizeof(buf), NULL, fmt, args)

#define string_format_from(buf, from, fmt, args...) \
	string_nformat(buf, sizeof(buf), from, fmt, args)

static int
string_enum_compare(const char *str1, const char *str2, int len)
{
	size_t i;

#define string_enum_sep(x) ((x) == '-' || (x) == '_' || (x) == '.')

	/* Diff-Header == DIFF_HEADER */
	for (i = 0; i < len; i++) {
		if (toupper(str1[i]) == toupper(str2[i]))
			continue;

		if (string_enum_sep(str1[i]) &&
		    string_enum_sep(str2[i]))
			continue;

		return str1[i] - str2[i];
	}

	return 0;
}
#define BUFPUT(x) do { if (bufsize < SIZEOF_CMD) buf[bufsize++] = (x); } while (0)
/*
 * User requests
 */

#define REQ_INFO \
	/* XXX: Keep the view request first and in sync with views[]. */ \
	REQ_GROUP("View switching") \
	REQ_(VIEW_MAIN,		"Show main view"), \
	REQ_(VIEW_DIFF,		"Show diff view"), \
	REQ_(VIEW_LOG,		"Show log view"), \
	REQ_(VIEW_HELP,		"Show help page"), \
	REQ_(VIEW_PAGER,	"Show pager view"), \
	\
	REQ_GROUP("View manipulation") \
	REQ_(ENTER,		"Enter current line and scroll"), \
	REQ_(NEXT,		"Move to next"), \
	REQ_(PREVIOUS,		"Move to previous"), \
	REQ_(VIEW_NEXT,		"Move focus to next view"), \
	REQ_(VIEW_CLOSE,	"Close the current view"), \
	REQ_(QUIT,		"Close all views and quit"), \
	\
	REQ_GROUP("Cursor navigation") \
	REQ_(MOVE_UP,		"Move cursor one line up"), \
	REQ_(MOVE_DOWN,		"Move cursor one line down"), \
	REQ_(MOVE_PAGE_DOWN,	"Move cursor one page down"), \
	REQ_(MOVE_PAGE_UP,	"Move cursor one page up"), \
	REQ_(MOVE_FIRST_LINE,	"Move cursor to first line"), \
	REQ_(MOVE_LAST_LINE,	"Move cursor to last line"), \
	\
	REQ_GROUP("Scrolling") \
	REQ_(SCROLL_LINE_UP,	"Scroll one line up"), \
	REQ_(SCROLL_LINE_DOWN,	"Scroll one line down"), \
	REQ_(SCROLL_PAGE_UP,	"Scroll one page up"), \
	REQ_(SCROLL_PAGE_DOWN,	"Scroll one page down"), \
	\
	REQ_GROUP("Misc") \
	REQ_(PROMPT,		"Bring up the prompt"), \
	REQ_(SCREEN_UPDATE,	"Update the screen"), \
	REQ_(SCREEN_REDRAW,	"Redraw the screen"), \
	REQ_(SCREEN_RESIZE,	"Resize the screen"), \
	REQ_(SHOW_VERSION,	"Show version information"), \
	REQ_(STOP_LOADING,	"Stop all loading views"), \
	REQ_(TOGGLE_LINENO,	"Toggle line numbers"), \
	REQ_(TOGGLE_REV_GRAPH,	"Toggle revision graph visualization")


/* User action requests. */
enum request {
#define REQ_GROUP(help)
#define REQ_(req, help) REQ_##req

	/* Offset all requests to avoid conflicts with ncurses getch values. */
	REQ_OFFSET = KEY_MAX + 1,
	REQ_INFO,
	REQ_UNKNOWN,

#undef	REQ_GROUP
#undef	REQ_
};

struct request_info {
	enum request request;
	char *name;
	int namelen;
	char *help;
};

static struct request_info req_info[] = {
#define REQ_GROUP(help)	{ 0, NULL, 0, (help) },
#define REQ_(req, help)	{ REQ_##req, (#req), STRING_SIZE(#req), (help) }
	REQ_INFO
#undef	REQ_GROUP
#undef	REQ_
};

static enum request
get_request(const char *name)
{
	int namelen = strlen(name);
	int i;

	for (i = 0; i < ARRAY_SIZE(req_info); i++)
		if (req_info[i].namelen == namelen &&
		    !string_enum_compare(req_info[i].name, name, namelen))
			return req_info[i].request;

	return REQ_UNKNOWN;
}


/*
 * Options
 */

static const char usage[] =
VERSION " (" __DATE__ ")\n"
"\n"
"Usage: tig [options]\n"
"   or: tig [options] [--] [git log options]\n"
"   or: tig [options] log  [git log options]\n"
"   or: tig [options] diff [git diff options]\n"
"   or: tig [options] show [git show options]\n"
"   or: tig [options] <    [git command output]\n"
"\n"
"Options:\n"
"  -l                          Start up in log view\n"
"  -d                          Start up in diff view\n"
"  -n[I], --line-number[=I]    Show line numbers with given interval\n"
"  -b[N], --tab-size[=N]       Set number of spaces for tab expansion\n"
"  --                          Mark end of tig options\n"
"  -v, --version               Show version and exit\n"
"  -h, --help                  Show help message and exit\n";
static bool opt_rev_graph	= TRUE;
static char opt_encoding[20]	= "";
static bool opt_utf8		= TRUE;
enum option_type {
	OPT_NONE,
	OPT_INT,
};

static bool
check_option(char *opt, char short_name, char *name, enum option_type type, ...)
{
	va_list args;
	char *value = "";
	int *number;

	if (opt[0] != '-')
		return FALSE;

	if (opt[1] == '-') {
		int namelen = strlen(name);

		opt += 2;

		if (strncmp(opt, name, namelen))
			return FALSE;

		if (opt[namelen] == '=')
			value = opt + namelen + 1;

	} else {
		if (!short_name || opt[1] != short_name)
			return FALSE;
		value = opt + 2;
	}

	va_start(args, type);
	if (type == OPT_INT) {
		number = va_arg(args, int *);
		if (isdigit(*value))
			*number = atoi(value);
	}
	va_end(args);

	return TRUE;
}

		if (check_option(opt, 'n', "line-number", OPT_INT, &opt_num_interval)) {
		if (check_option(opt, 'b', "tab-size", OPT_INT, &opt_tab_size)) {
			opt_tab_size = MIN(opt_tab_size, TABSIZE);
		if (check_option(opt, 'v', "version", OPT_NONE)) {
		if (check_option(opt, 'h', "help", OPT_NONE)) {
			printf(usage);
			return FALSE;
		}

		die("unknown option '%s'\n\n%s", opt, usage);
	if (*opt_encoding && strcasecmp(opt_encoding, "UTF-8"))
		opt_utf8 = FALSE;

LINE(DIFF_HEADER,  "diff --git ",	COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_INDEX,	"index ",	  COLOR_BLUE,	COLOR_DEFAULT,	0), \
LINE(DIFF_OLDMODE,	"old file mode ", COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_NEWMODE,	"new file mode ", COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_COPY_FROM,	"copy from",	  COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_COPY_TO,	"copy to",	  COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_RENAME_FROM,	"rename from",	  COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_RENAME_TO,	"rename to",	  COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_SIMILARITY,   "similarity ",	  COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_DISSIMILARITY,"dissimilarity ", COLOR_YELLOW,	COLOR_DEFAULT,	0), \
LINE(DIFF_TREE,		"diff-tree ",	  COLOR_BLUE,	COLOR_DEFAULT,	0), \
LINE(PP_REFS,	   "Refs: ",		COLOR_RED,	COLOR_DEFAULT,	0), \
LINE(MAIN_REF,     "",			COLOR_CYAN,	COLOR_DEFAULT,	A_BOLD), \
	const char *name;	/* Option name. */
	int namelen;		/* Size of option name. */
	const char *line;	/* The start of line to match. */
	{ #type, STRING_SIZE(#type), (line), STRING_SIZE(line), (fg), (bg), (attr) }
	return LINE_DEFAULT;
}

static inline int
get_line_attr(enum line_type type)
{
	assert(type < ARRAY_SIZE(line_info));
	return COLOR_PAIR(type) | line_info[type].attr;
}

static struct line_info *
get_line_info(char *name, int namelen)
{
	enum line_type type;

	for (type = 0; type < ARRAY_SIZE(line_info); type++)
		if (namelen == line_info[type].namelen &&
		    !string_enum_compare(line_info[type].name, name, namelen))
			return &line_info[type];

	return NULL;
}

static void
init_colors(void)
{
	int default_bg = COLOR_BLACK;
	int default_fg = COLOR_WHITE;
	enum line_type type;

	start_color();

	if (use_default_colors() != ERR) {
		default_bg = -1;
		default_fg = -1;
	}

	for (type = 0; type < ARRAY_SIZE(line_info); type++) {
		struct line_info *info = &line_info[type];
		int bg = info->bg == COLOR_DEFAULT ? default_bg : info->bg;
		int fg = info->fg == COLOR_DEFAULT ? default_fg : info->fg;

		init_pair(type, fg, bg);
	}
}

struct line {
	enum line_type type;
	void *data;		/* User data */
};


/*
 * Keys
 */

struct keybinding {
	int alias;
	enum request request;
	struct keybinding *next;
};

static struct keybinding default_keybindings[] = {
	/* View switching */
	{ 'm',		REQ_VIEW_MAIN },
	{ 'd',		REQ_VIEW_DIFF },
	{ 'l',		REQ_VIEW_LOG },
	{ 'p',		REQ_VIEW_PAGER },
	{ 'h',		REQ_VIEW_HELP },
	{ '?',		REQ_VIEW_HELP },

	/* View manipulation */
	{ 'q',		REQ_VIEW_CLOSE },
	{ KEY_TAB,	REQ_VIEW_NEXT },
	{ KEY_RETURN,	REQ_ENTER },
	{ KEY_UP,	REQ_PREVIOUS },
	{ KEY_DOWN,	REQ_NEXT },

	/* Cursor navigation */
	{ 'k',		REQ_MOVE_UP },
	{ 'j',		REQ_MOVE_DOWN },
	{ KEY_HOME,	REQ_MOVE_FIRST_LINE },
	{ KEY_END,	REQ_MOVE_LAST_LINE },
	{ KEY_NPAGE,	REQ_MOVE_PAGE_DOWN },
	{ ' ',		REQ_MOVE_PAGE_DOWN },
	{ KEY_PPAGE,	REQ_MOVE_PAGE_UP },
	{ 'b',		REQ_MOVE_PAGE_UP },
	{ '-',		REQ_MOVE_PAGE_UP },

	/* Scrolling */
	{ KEY_IC,	REQ_SCROLL_LINE_UP },
	{ KEY_DC,	REQ_SCROLL_LINE_DOWN },
	{ 'w',		REQ_SCROLL_PAGE_UP },
	{ 's',		REQ_SCROLL_PAGE_DOWN },

	/* Misc */
	{ 'Q',		REQ_QUIT },
	{ 'z',		REQ_STOP_LOADING },
	{ 'v',		REQ_SHOW_VERSION },
	{ 'r',		REQ_SCREEN_REDRAW },
	{ 'n',		REQ_TOGGLE_LINENO },
	{ 'g',		REQ_TOGGLE_REV_GRAPH},
	{ ':',		REQ_PROMPT },

	/* wgetch() with nodelay() enabled returns ERR when there's no input. */
	{ ERR,		REQ_SCREEN_UPDATE },

	/* Use the ncurses SIGWINCH handler. */
	{ KEY_RESIZE,	REQ_SCREEN_RESIZE },
};

#define KEYMAP_INFO \
	KEYMAP_(GENERIC), \
	KEYMAP_(MAIN), \
	KEYMAP_(DIFF), \
	KEYMAP_(LOG), \
	KEYMAP_(PAGER), \
	KEYMAP_(HELP) \

enum keymap {
#define KEYMAP_(name) KEYMAP_##name
	KEYMAP_INFO
#undef	KEYMAP_
};

static struct int_map keymap_table[] = {
#define KEYMAP_(name) { #name, STRING_SIZE(#name), KEYMAP_##name }
	KEYMAP_INFO
#undef	KEYMAP_
};

#define set_keymap(map, name) \
	set_from_int_map(keymap_table, ARRAY_SIZE(keymap_table), map, name, strlen(name))

static struct keybinding *keybindings[ARRAY_SIZE(keymap_table)];

static void
add_keybinding(enum keymap keymap, enum request request, int key)
{
	struct keybinding *keybinding;

	keybinding = calloc(1, sizeof(*keybinding));
	if (!keybinding)
		die("Failed to allocate keybinding");

	keybinding->alias = key;
	keybinding->request = request;
	keybinding->next = keybindings[keymap];
	keybindings[keymap] = keybinding;
}

/* Looks for a key binding first in the given map, then in the generic map, and
 * lastly in the default keybindings. */
static enum request
get_keybinding(enum keymap keymap, int key)
{
	struct keybinding *kbd;
	int i;

	for (kbd = keybindings[keymap]; kbd; kbd = kbd->next)
		if (kbd->alias == key)
			return kbd->request;

	for (kbd = keybindings[KEYMAP_GENERIC]; kbd; kbd = kbd->next)
		if (kbd->alias == key)
			return kbd->request;

	for (i = 0; i < ARRAY_SIZE(default_keybindings); i++)
		if (default_keybindings[i].alias == key)
			return default_keybindings[i].request;

	return (enum request) key;
}


struct key {
	char *name;
	int value;
};

static struct key key_table[] = {
	{ "Enter",	KEY_RETURN },
	{ "Space",	' ' },
	{ "Backspace",	KEY_BACKSPACE },
	{ "Tab",	KEY_TAB },
	{ "Escape",	KEY_ESC },
	{ "Left",	KEY_LEFT },
	{ "Right",	KEY_RIGHT },
	{ "Up",		KEY_UP },
	{ "Down",	KEY_DOWN },
	{ "Insert",	KEY_IC },
	{ "Delete",	KEY_DC },
	{ "Hash",	'#' },
	{ "Home",	KEY_HOME },
	{ "End",	KEY_END },
	{ "PageUp",	KEY_PPAGE },
	{ "PageDown",	KEY_NPAGE },
	{ "F1",		KEY_F(1) },
	{ "F2",		KEY_F(2) },
	{ "F3",		KEY_F(3) },
	{ "F4",		KEY_F(4) },
	{ "F5",		KEY_F(5) },
	{ "F6",		KEY_F(6) },
	{ "F7",		KEY_F(7) },
	{ "F8",		KEY_F(8) },
	{ "F9",		KEY_F(9) },
	{ "F10",	KEY_F(10) },
	{ "F11",	KEY_F(11) },
	{ "F12",	KEY_F(12) },
};

static int
get_key_value(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(key_table); i++)
		if (!strcasecmp(key_table[i].name, name))
			return key_table[i].value;

	if (strlen(name) == 1 && isprint(*name))
		return (int) *name;

	return ERR;
}

static char *
get_key(enum request request)
{
	static char buf[BUFSIZ];
	static char key_char[] = "'X'";
	int pos = 0;
	char *sep = "    ";
	int i;

	buf[pos] = 0;

	for (i = 0; i < ARRAY_SIZE(default_keybindings); i++) {
		struct keybinding *keybinding = &default_keybindings[i];
		char *seq = NULL;
		int key;

		if (keybinding->request != request)
			continue;

		for (key = 0; key < ARRAY_SIZE(key_table); key++)
			if (key_table[key].value == keybinding->alias)
				seq = key_table[key].name;

		if (seq == NULL &&
		    keybinding->alias < 127 &&
		    isprint(keybinding->alias)) {
			key_char[1] = (char) keybinding->alias;
			seq = key_char;
		}

		if (!seq)
			seq = "'?'";

		if (!string_format_from(buf, &pos, "%s%s", sep, seq))
			return "Too many keybindings!";
		sep = ", ";
	}

	return buf;
}


/*
 * User config file handling.
 */

static struct int_map color_map[] = {
#define COLOR_MAP(name) { #name, STRING_SIZE(#name), COLOR_##name }
	COLOR_MAP(DEFAULT),
	COLOR_MAP(BLACK),
	COLOR_MAP(BLUE),
	COLOR_MAP(CYAN),
	COLOR_MAP(GREEN),
	COLOR_MAP(MAGENTA),
	COLOR_MAP(RED),
	COLOR_MAP(WHITE),
	COLOR_MAP(YELLOW),
};

#define set_color(color, name) \
	set_from_int_map(color_map, ARRAY_SIZE(color_map), color, name, strlen(name))

static struct int_map attr_map[] = {
#define ATTR_MAP(name) { #name, STRING_SIZE(#name), A_##name }
	ATTR_MAP(NORMAL),
	ATTR_MAP(BLINK),
	ATTR_MAP(BOLD),
	ATTR_MAP(DIM),
	ATTR_MAP(REVERSE),
	ATTR_MAP(STANDOUT),
	ATTR_MAP(UNDERLINE),
};

#define set_attribute(attr, name) \
	set_from_int_map(attr_map, ARRAY_SIZE(attr_map), attr, name, strlen(name))

static int   config_lineno;
static bool  config_errors;
static char *config_msg;

/* Wants: object fgcolor bgcolor [attr] */
static int
option_color_command(int argc, char *argv[])
{
	struct line_info *info;

	if (argc != 3 && argc != 4) {
		config_msg = "Wrong number of arguments given to color command";
		return ERR;
	}

	info = get_line_info(argv[0], strlen(argv[0]));
	if (!info) {
		config_msg = "Unknown color name";
		return ERR;
	}

	if (set_color(&info->fg, argv[1]) == ERR ||
	    set_color(&info->bg, argv[2]) == ERR) {
		config_msg = "Unknown color";
		return ERR;
	}

	if (argc == 4 && set_attribute(&info->attr, argv[3]) == ERR) {
		config_msg = "Unknown attribute";
		return ERR;
	}

	return OK;
}

/* Wants: name = value */
static int
option_set_command(int argc, char *argv[])
{
	if (argc != 3) {
		config_msg = "Wrong number of arguments given to set command";
		return ERR;
	}

	if (strcmp(argv[1], "=")) {
		config_msg = "No value assigned";
		return ERR;
	}

	if (!strcmp(argv[0], "show-rev-graph")) {
		opt_rev_graph = (!strcmp(argv[2], "1") ||
				 !strcmp(argv[2], "true") ||
				 !strcmp(argv[2], "yes"));
		return OK;
	}

	if (!strcmp(argv[0], "line-number-interval")) {
		opt_num_interval = atoi(argv[2]);
		return OK;
	}

	if (!strcmp(argv[0], "tab-size")) {
		opt_tab_size = atoi(argv[2]);
		return OK;
	}

	if (!strcmp(argv[0], "commit-encoding")) {
		char *arg = argv[2];
		int delimiter = *arg;
		int i;

		switch (delimiter) {
		case '"':
		case '\'':
			for (arg++, i = 0; arg[i]; i++)
				if (arg[i] == delimiter) {
					arg[i] = 0;
					break;
				}
		default:
			string_copy(opt_encoding, arg);
			return OK;
		}
	}

	config_msg = "Unknown variable name";
	return ERR;
}

/* Wants: mode request key */
static int
option_bind_command(int argc, char *argv[])
{
	enum request request;
	int keymap;
	int key;

	if (argc != 3) {
		config_msg = "Wrong number of arguments given to bind command";
		return ERR;
	}

	if (set_keymap(&keymap, argv[0]) == ERR) {
		config_msg = "Unknown key map";
		return ERR;
	}

	key = get_key_value(argv[1]);
	if (key == ERR) {
		config_msg = "Unknown key";
		return ERR;
	}

	request = get_request(argv[2]);
	if (request == REQ_UNKNOWN) {
		config_msg = "Unknown request name";
		return ERR;
	}

	add_keybinding(keymap, request, key);

	return OK;
static int
set_option(char *opt, char *value)
	char *argv[16];
	int valuelen;
	int argc = 0;

	/* Tokenize */
	while (argc < ARRAY_SIZE(argv) && (valuelen = strcspn(value, " \t"))) {
		argv[argc++] = value;

		value += valuelen;
		if (!*value)
			break;

		*value++ = 0;
		while (isspace(*value))
			value++;
	}

	if (!strcmp(opt, "color"))
		return option_color_command(argc, argv);

	if (!strcmp(opt, "set"))
		return option_set_command(argc, argv);

	if (!strcmp(opt, "bind"))
		return option_bind_command(argc, argv);

	config_msg = "Unknown option command";
	return ERR;
static int
read_option(char *opt, int optlen, char *value, int valuelen)
	int status = OK;
	config_lineno++;
	config_msg = "Internal error";
	/* Check for comment markers, since read_properties() will
	 * only ensure opt and value are split at first " \t". */
	optlen = strcspn(opt, "#");
	if (optlen == 0)
		return OK;
	if (opt[optlen] != 0) {
		config_msg = "No option value";
		status = ERR;
	}  else {
		/* Look for comment endings in the value. */
		int len = strcspn(value, "#");

		if (len < valuelen) {
			valuelen = len;
			value[valuelen] = 0;
		}

		status = set_option(opt, value);
	}

	if (status == ERR) {
		fprintf(stderr, "Error on line %d, near '%.*s': %s\n",
			config_lineno, optlen, opt, config_msg);
		config_errors = TRUE;

	/* Always keep going if errors are encountered. */
	return OK;
static int
load_options(void)
{
	char *home = getenv("HOME");
	char buf[1024];
	FILE *file;
	config_lineno = 0;
	config_errors = FALSE;
	if (!home || !string_format(buf, "%s/.tigrc", home))
		return ERR;
	/* It's ok that the file doesn't exist. */
	file = fopen(buf, "r");
	if (!file)
		return OK;
	if (read_properties(file, " \t", read_option) == ERR ||
	    config_errors == TRUE)
		fprintf(stderr, "Errors while loading %s.\n", buf);
	return OK;
}
/*
 * The viewer
 */
struct view;
struct view_ops;
/* The display array of active views and the index of the current view. */
static struct view *display[2];
static unsigned int current_view;
#define foreach_view(view, i) \
	for (i = 0; i < ARRAY_SIZE(display) && (view = display[i]); i++)

#define displayed_views()	(display[1] != NULL ? 2 : 1)

/* Current head and commit ID */
static char ref_commit[SIZEOF_REF]	= "HEAD";
static char ref_head[SIZEOF_REF]	= "HEAD";
	const char *cmd_fmt;	/* Default command line format */
	const char *cmd_env;	/* Command line set via environment */
	const char *id;		/* Points to either of ref_{head,commit} */

	struct view_ops *ops;	/* View operations */

	enum keymap keymap;	/* What keymap does this view have */
	/* If non-NULL, points to the view that opened this view. If this view
	 * is closed tig will switch back to the parent view. */
	struct view *parent;

	struct line *line;	/* Line index */
	unsigned long line_size;/* Total number of allocated lines */
struct view_ops {
	/* What type of content being displayed. Used in the title bar. */
	const char *type;
	/* Draw one line; @lineno must be < view->height. */
	bool (*draw)(struct view *view, struct line *line, unsigned int lineno);
	/* Read one line; updates view->line. */
	bool (*read)(struct view *view, char *data);
	/* Depending on view, change display based on current line. */
	bool (*enter)(struct view *view, struct line *line);
};

#define VIEW_STR(name, cmd, env, ref, ops, map) \
	{ name, cmd, #env, ref, ops, map}
#define VIEW_(id, name, ops, ref) \
	VIEW_STR(name, TIG_##id##_CMD,  TIG_##id##_CMD, ref, ops, KEYMAP_##id)
	VIEW_(MAIN,  "main",  &main_ops,  ref_head),
	VIEW_(DIFF,  "diff",  &pager_ops, ref_commit),
	VIEW_(LOG,   "log",   &pager_ops, ref_head),
	VIEW_(HELP,  "help",  &pager_ops, "static"),
	VIEW_(PAGER, "pager", &pager_ops, "static"),
static bool
draw_view_line(struct view *view, unsigned int lineno)
{
	if (view->offset + lineno >= view->lines)
		return FALSE;
	return view->ops->draw(view, &view->line[view->offset + lineno], lineno);
}
		if (!draw_view_line(view, lineno))

static void
update_view_title(struct view *view)
{
	if (view == display[current_view])
		wbkgdset(view->title, get_line_attr(LINE_TITLE_FOCUS));
	else
		wbkgdset(view->title, get_line_attr(LINE_TITLE_BLUR));

	werase(view->title);
	wmove(view->title, 0, 0);

	if (*view->ref)
		wprintw(view->title, "[%s] %s", view->name, view->ref);
	else
		wprintw(view->title, "[%s]", view->name);

	if (view->lines || view->pipe) {
		unsigned int view_lines = view->offset + view->height;
		unsigned int lines = view->lines
				   ? MIN(view_lines, view->lines) * 100 / view->lines
				   : 0;

		wprintw(view->title, " - %s %d of %d (%d%%)",
			view->ops->type,
			view->lineno + 1,
			view->lines,
			lines);
	}

	if (view->pipe) {
		time_t secs = time(NULL) - view->start_time;

		/* Three git seconds are a long time ... */
		if (secs > 2)
			wprintw(view->title, " %lds", secs);
	}

	wmove(view->title, 0, view->width - 1);
	wrefresh(view->title);
}

			view->win = newwin(view->height, 0, offset, 0);
			wresize(view->win, view->height, view->width);
redraw_display(void)
	struct view *view;
	int i;
	foreach_view (view, i) {
		redraw_view(view);
		update_view_title(view);
	}
}
static void
update_display_cursor(void)
{
	struct view *view = display[current_view];
	/* Move the cursor to the right-most column of the cursor line.
	 *
	 * XXX: This could turn out to be a bit expensive, but it ensures that
	 * the cursor does not jump around. */
		wmove(view->win, view->lineno - view->offset, view->width - 1);
		wrefresh(view->win);
do_scroll_view(struct view *view, int lines, bool redraw)
			if (!draw_view_line(view, line))
		draw_view_line(view, 0);
		draw_view_line(view, view->lineno - view->offset);
	if (!redraw)
		return;

	do_scroll_view(view, lines, TRUE);
move_view(struct view *view, enum request request, bool redraw)
		draw_view_line(view,  prev_lineno);
		do_scroll_view(view, steps, redraw);
	draw_view_line(view, view->lineno - view->offset);

	if (!redraw)
		return;
static void
end_update(struct view *view)
{
	if (!view->pipe)
		return;
	set_nonblocking_input(FALSE);
	if (view->pipe == stdin)
		fclose(view->pipe);
	else
		pclose(view->pipe);
	view->pipe = NULL;
}

	const char *id = view->id;

	if (view->pipe)
		end_update(view);
		const char *format = view->cmd_env ? view->cmd_env : view->cmd_fmt;
		if (!string_format(view->cmd, format, id, id, id, id, id))
			if (view->line[i].data)
				free(view->line[i].data);
static struct line *
realloc_lines(struct view *view, size_t line_size)
	struct line *tmp = realloc(view->line, sizeof(*view->line) * line_size);

	if (!tmp)
		return NULL;

	view->line = tmp;
	view->line_size = line_size;
	return view->line;
	if (!realloc_lines(view, view->lines + lines))
		report("");

/*
 * View opening
 */

static void open_help_view(struct view *view)
{
	char buf[BUFSIZ];
	int lines = ARRAY_SIZE(req_info) + 2;
	int i;

	if (view->lines > 0)
		return;

	for (i = 0; i < ARRAY_SIZE(req_info); i++)
		if (!req_info[i].request)
			lines++;

	view->line = calloc(lines, sizeof(*view->line));
	if (!view->line) {
		report("Allocation failure");
		return;
	}

	view->ops->read(view, "Quick reference for tig keybindings:");

	for (i = 0; i < ARRAY_SIZE(req_info); i++) {
		char *key;

		if (!req_info[i].request) {
			view->ops->read(view, "");
			view->ops->read(view, req_info[i].help);
			continue;
		}

		key = get_key(req_info[i].request);
		if (!string_format(buf, "%-25s %s", key, req_info[i].help))
			continue;

		view->ops->read(view, buf);
	}
}

	int nviews = displayed_views();
	struct view *base_view = display[0];
	if (view == VIEW(REQ_VIEW_HELP)) {
		open_help_view(view);

	} else if ((reload || strcmp(view->vid, view->id)) &&
		   !begin_update(view)) {
		display[1] = view;
			current_view = 1;
	/* Resize the view when switching between split- and full-screen,
	 * or when switching between two different full-screen views. */
	if (nviews != displayed_views() ||
	    (nviews == 1 && base_view != display[0]))
		resize_display();
		do_scroll_view(prev, lines, TRUE);
		if (split && !backgrounded) {
			/* "Blur" the previous view. */
		}
		view->parent = prev;
	if (view->pipe && view->lines == 0) {
		report("");
		report("");
		move_view(view, request, TRUE);
	case REQ_NEXT:
	case REQ_PREVIOUS:
		request = request == REQ_NEXT ? REQ_MOVE_DOWN : REQ_MOVE_UP;

		if (view == VIEW(REQ_VIEW_DIFF) &&
		    view->parent == VIEW(REQ_VIEW_MAIN)) {
			bool redraw = display[1] == view;

			view = view->parent;
			move_view(view, request, redraw);
			if (redraw)
				update_view_title(view);
		} else {
			move_view(view, request, TRUE);
			break;
		}
		return view->ops->enter(view, &view->line[view->lineno]);
		int nviews = displayed_views();
	case REQ_TOGGLE_LINENO:
		redraw_display();
		break;

	case REQ_TOGGLE_REV_GRAPH:
		opt_rev_graph = !opt_rev_graph;
		redraw_display();
		for (i = 0; i < ARRAY_SIZE(views); i++) {
			view = &views[i];
				report("Stopped loading the %s view", view->name),
		report("%s (built %s)", VERSION, __DATE__);
		redraw_display();
	case REQ_VIEW_CLOSE:
		/* XXX: Mark closed views by letting view->parent point to the
		 * view itself. Parents to closed view should never be
		 * followed. */
		if (view->parent &&
		    view->parent->parent != view->parent) {
			memset(display, 0, sizeof(display));
			current_view = 0;
			display[current_view] = view->parent;
			view->parent = view;
			resize_display();
			redraw_display();
			break;
		}
		/* Fall-through */
 * Pager backend
pager_draw(struct view *view, struct line *line, unsigned int lineno)
	char *text = line->data;
	enum line_type type = line->type;
	int textlen = strlen(text);
			string_copy(view->ref, text + 7);
		while (text && col_offset + col < view->width) {
			char *pos = text;
			if (*text == '\t') {
				text++;
				pos = spaces;
				text = strchr(text, '\t');
				cols = line ? text - pos : strlen(pos);
			waddnstr(view->win, pos, MIN(cols, cols_max));
		for (; pos < textlen && col < view->width; pos++, col++)
			if (text[pos] == '\t')
		waddnstr(view->win, text, pos);
static void
add_pager_refs(struct view *view, struct line *line)
{
	char buf[1024];
	char *data = line->data;
	struct ref **refs;
	int bufpos = 0, refpos = 0;
	const char *sep = "Refs: ";

	assert(line->type == LINE_COMMIT);

	refs = get_refs(data + STRING_SIZE("commit "));
	if (!refs)
		return;

	do {
		struct ref *ref = refs[refpos];
		char *fmt = ref->tag ? "%s[%s]" : "%s%s";

		if (!string_format_from(buf, &bufpos, fmt, sep, ref->name))
			return;
		sep = ", ";
	} while (refs[refpos++]->next);

	if (!realloc_lines(view, view->line_size + 1))
		return;

	line = &view->line[view->lines];
	line->data = strdup(buf);
	if (!line->data)
		return;

	line->type = LINE_PP_REFS;
	view->lines++;
}

pager_read(struct view *view, char *data)
	struct line *line = &view->line[view->lines];
	line->data = strdup(data);
	if (!line->data)
	line->type = get_line_type(line->data);

	if (line->type == LINE_COMMIT &&
	    (view == VIEW(REQ_VIEW_DIFF) ||
	     view == VIEW(REQ_VIEW_LOG)))
		add_pager_refs(view, line);

pager_enter(struct view *view, struct line *line)
	int split = 0;
	if (line->type == LINE_COMMIT &&
	   (view == VIEW(REQ_VIEW_LOG) ||
	    view == VIEW(REQ_VIEW_PAGER))) {
		open_view(view, REQ_VIEW_DIFF, OPEN_SPLIT);
		split = 1;
	/* Always scroll the view even if it was split. That way
	 * you can use Enter to scroll through the log view and
	 * split open each commit diff. */
	scroll_view(view, REQ_SCROLL_LINE_DOWN);

	/* FIXME: A minor workaround. Scrolling the view will call report("")
	 * but if we are scrolling a non-current view this won't properly
	 * update the view title. */
	if (split)
		update_view_title(view);

/*
 * Main view backend
 */

struct commit {
	char id[41];			/* SHA1 ID. */
	char title[75];			/* First line of the commit message. */
	char author[75];		/* Author of the commit. */
	struct tm time;			/* Date from the author ident. */
	struct ref **refs;		/* Repository references. */
	chtype graph[SIZEOF_REVGRAPH];	/* Ancestry chain graphics. */
	size_t graph_size;		/* The width of the graph array. */
};
main_draw(struct view *view, struct line *line, unsigned int lineno)
	struct commit *commit = line->data;
	size_t authorlen;
	int trimmed = 1;
	if (opt_utf8) {
		authorlen = utf8_length(commit->author, AUTHOR_COLS - 2, &col, &trimmed);
	} else {
		authorlen = strlen(commit->author);
		if (authorlen > AUTHOR_COLS - 2) {
			authorlen = AUTHOR_COLS - 2;
			trimmed = 1;
		}
	}

	if (trimmed) {
		waddnstr(view->win, commit->author, authorlen);
	col += AUTHOR_COLS;
	if (opt_rev_graph && commit->graph_size) {
		size_t i;

		wmove(view->win, lineno, col);
		/* Using waddch() instead of waddnstr() ensures that
		 * they'll be rendered correctly for the cursor line. */
		for (i = 0; i < commit->graph_size; i++)
			waddch(view->win, commit->graph[i]);

		col += commit->graph_size + 1;
	}

	wmove(view->win, lineno, col);
	struct commit *commit = view->lines
			      ? view->line[view->lines - 1].data : NULL;
		view->line[view->lines++].data = commit;
		commit->graph[commit->graph_size++] = ACS_LTEE;
		if (!commit)
			break;

		if (!commit)
main_enter(struct view *view, struct line *line)
	enum open_flags flags = display[0] == view ? OPEN_SPLIT : OPEN_DEFAULT;

	open_view(view, REQ_VIEW_DIFF, flags);
/*
 * Unicode / UTF-8 handling
 *
 * NOTE: Much of the following code for dealing with unicode is derived from
 * ELinks' UTF-8 code developed by Scrool <scroolik@gmail.com>. Origin file is
 * src/intl/charset.c from the utf8 branch commit elinks-0.11.0-g31f2c28.
 */

/* I've (over)annotated a lot of code snippets because I am not entirely
 * confident that the approach taken by this small UTF-8 interface is correct.
 * --jonas */
static inline int
unicode_width(unsigned long c)
{
	if (c >= 0x1100 &&
	   (c <= 0x115f				/* Hangul Jamo */
	    || c == 0x2329
	    || c == 0x232a
	    || (c >= 0x2e80  && c <= 0xa4cf && c != 0x303f)
						/* CJK ... Yi */
	    || (c >= 0xac00  && c <= 0xd7a3)	/* Hangul Syllables */
	    || (c >= 0xf900  && c <= 0xfaff)	/* CJK Compatibility Ideographs */
	    || (c >= 0xfe30  && c <= 0xfe6f)	/* CJK Compatibility Forms */
	    || (c >= 0xff00  && c <= 0xff60)	/* Fullwidth Forms */
	    || (c >= 0xffe0  && c <= 0xffe6)
	    || (c >= 0x20000 && c <= 0x2fffd)
	    || (c >= 0x30000 && c <= 0x3fffd)))
		return 2;

	return 1;
}

/* Number of bytes used for encoding a UTF-8 character indexed by first byte.
 * Illegal bytes are set one. */
static const unsigned char utf8_bytes[256] = {
	1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4, 5,5,5,5,6,6,1,1,
/* Decode UTF-8 multi-byte representation into a unicode character. */
static inline unsigned long
utf8_to_unicode(const char *string, size_t length)
{
	unsigned long unicode;
	switch (length) {
	case 1:
		unicode  =   string[0];
		break;
	case 2:
		unicode  =  (string[0] & 0x1f) << 6;
		unicode +=  (string[1] & 0x3f);
		break;
	case 3:
		unicode  =  (string[0] & 0x0f) << 12;
		unicode += ((string[1] & 0x3f) << 6);
		unicode +=  (string[2] & 0x3f);
		break;
	case 4:
		unicode  =  (string[0] & 0x0f) << 18;
		unicode += ((string[1] & 0x3f) << 12);
		unicode += ((string[2] & 0x3f) << 6);
		unicode +=  (string[3] & 0x3f);
		break;
	case 5:
		unicode  =  (string[0] & 0x0f) << 24;
		unicode += ((string[1] & 0x3f) << 18);
		unicode += ((string[2] & 0x3f) << 12);
		unicode += ((string[3] & 0x3f) << 6);
		unicode +=  (string[4] & 0x3f);
		break;
	case 6:
		unicode  =  (string[0] & 0x01) << 30;
		unicode += ((string[1] & 0x3f) << 24);
		unicode += ((string[2] & 0x3f) << 18);
		unicode += ((string[3] & 0x3f) << 12);
		unicode += ((string[4] & 0x3f) << 6);
		unicode +=  (string[5] & 0x3f);
		break;
	default:
		die("Invalid unicode length");
	}
	/* Invalid characters could return the special 0xfffd value but NUL
	 * should be just as good. */
	return unicode > 0xffff ? 0 : unicode;
}
/* Calculates how much of string can be shown within the given maximum width
 * and sets trimmed parameter to non-zero value if all of string could not be
 * shown.
 *
 * Additionally, adds to coloffset how many many columns to move to align with
 * the expected position. Takes into account how multi-byte and double-width
 * characters will effect the cursor position.
 *
 * Returns the number of bytes to output from string to satisfy max_width. */
static size_t
utf8_length(const char *string, size_t max_width, int *coloffset, int *trimmed)
{
	const char *start = string;
	const char *end = strchr(string, '\0');
	size_t mbwidth = 0;
	size_t width = 0;
	*trimmed = 0;
	while (string < end) {
		int c = *(unsigned char *) string;
		unsigned char bytes = utf8_bytes[c];
		size_t ucwidth;
		unsigned long unicode;
		if (string + bytes > end)
			break;
		/* Change representation to figure out whether
		 * it is a single- or double-width character. */

		unicode = utf8_to_unicode(string, bytes);
		/* FIXME: Graceful handling of invalid unicode character. */
		if (!unicode)
			break;
		ucwidth = unicode_width(unicode);
		width  += ucwidth;
		if (width > max_width) {
			*trimmed = 1;
			break;
		}
		/* The column offset collects the differences between the
		 * number of bytes encoding a character and the number of
		 * columns will be used for rendering said character.
		 *
		 * So if some character A is encoded in 2 bytes, but will be
		 * represented on the screen using only 1 byte this will and up
		 * adding 1 to the multi-byte column offset.
		 *
		 * Assumes that no double-width character can be encoding in
		 * less than two bytes. */
		if (bytes > ucwidth)
			mbwidth += bytes - ucwidth;

		string  += bytes;
	}

	*coloffset += mbwidth;

	return string - start;
static bool cursed = FALSE;
	update_display_cursor();
static int
read_prompt(void)
{
	enum { READING, STOP, CANCEL } status = READING;
	char buf[sizeof(opt_cmd) - STRING_SIZE("git \0")];
	int pos = 0;

	while (status == READING) {
		struct view *view;
		int i, key;

		foreach_view (view, i)
			update_view(view);

		report(":%.*s", pos, buf);
		/* Refresh, accept single keystroke of input */
		key = wgetch(status_win);
		switch (key) {
		case KEY_RETURN:
		case KEY_ENTER:
		case '\n':
			status = pos ? STOP : CANCEL;
			break;

		case KEY_BACKSPACE:
			if (pos > 0)
				pos--;
			else
				status = CANCEL;
			break;

		case KEY_ESC:
			status = CANCEL;
			break;

		case ERR:
			break;

		default:
			if (pos >= sizeof(buf)) {
				report("Input string too long");
				return ERR;
			}

			if (isprint(key))
				buf[pos++] = (char) key;
		}
	}

	if (status == CANCEL) {
		/* Clear the status window */
		report("");
		return ERR;
	}

	buf[pos++] = 0;
	if (!string_format(opt_cmd, "git %s", buf))
		return ERR;
	opt_request = REQ_VIEW_PAGER;

	return OK;
}
/* Id <-> ref store */
static struct ref ***id_refs;
static size_t id_refs_size;

	struct ref ***tmp_id_refs;
	struct ref **ref_list = NULL;
	size_t ref_list_size = 0;
	for (i = 0; i < id_refs_size; i++)
		if (!strcmp(id, id_refs[i][0]->id))
			return id_refs[i];

	tmp_id_refs = realloc(id_refs, (id_refs_size + 1) * sizeof(*id_refs));
	if (!tmp_id_refs)
		return NULL;

	id_refs = tmp_id_refs;

		tmp = realloc(ref_list, (ref_list_size + 1) * sizeof(*ref_list));
			if (ref_list)
				free(ref_list);
		ref_list = tmp;
		if (ref_list_size > 0)
			ref_list[ref_list_size - 1]->next = 1;
		ref_list[ref_list_size] = &refs[i];
		ref_list[ref_list_size]->next = 0;
		ref_list_size++;
	if (ref_list)
		id_refs[id_refs_size++] = ref_list;

	return ref_list;
read_ref(char *id, int idlen, char *name, int namelen)
	struct ref *ref;
	bool tag = FALSE;
	if (!strncmp(name, "refs/tags/", STRING_SIZE("refs/tags/"))) {
		/* Commits referenced by tags has "^{}" appended. */
		if (name[namelen - 1] != '}')
			return OK;

		while (namelen > 0 && name[namelen] != '^')
			namelen--;

		tag = TRUE;
		namelen -= STRING_SIZE("refs/tags/");
		name	+= STRING_SIZE("refs/tags/");

	} else if (!strncmp(name, "refs/heads/", STRING_SIZE("refs/heads/"))) {
		namelen -= STRING_SIZE("refs/heads/");
		name	+= STRING_SIZE("refs/heads/");

	} else if (!strcmp(name, "HEAD")) {
		return OK;
	}

	refs = realloc(refs, sizeof(*refs) * (refs_size + 1));
	if (!refs)
	ref = &refs[refs_size++];
	ref->name = malloc(namelen + 1);
	if (!ref->name)
		return ERR;
	strncpy(ref->name, name, namelen);
	ref->name[namelen] = 0;
	ref->tag = tag;
	string_copy(ref->id, id);
	return OK;
}
static int
load_refs(void)
{
	const char *cmd_env = getenv("TIG_LS_REMOTE");
	const char *cmd = cmd_env && *cmd_env ? cmd_env : TIG_LS_REMOTE;
	return read_properties(popen(cmd, "r"), "\t", read_ref);
}
static int
read_repo_config_option(char *name, int namelen, char *value, int valuelen)
{
	if (!strcmp(name, "i18n.commitencoding"))
		string_copy(opt_encoding, value);
	return OK;
}

static int
load_repo_config(void)
{
	return read_properties(popen("git repo-config --list", "r"),
			       "=", read_repo_config_option);
}

static int
read_properties(FILE *pipe, const char *separators,
		int (*read_property)(char *, int, char *, int))
{
	char buffer[BUFSIZ];
	char *name;
	int state = OK;

	if (!pipe)
		return ERR;

	while (state == OK && (name = fgets(buffer, sizeof(buffer), pipe))) {
		char *value;
		size_t namelen;
		size_t valuelen;

		name = chomp_string(name);
		namelen = strcspn(name, separators);
		if (name[namelen]) {
			name[namelen] = 0;
			value = chomp_string(name + namelen + 1);
			valuelen = strlen(value);
		} else {
			value = "";
			valuelen = 0;
		}
		state = read_property(name, namelen, value, valuelen);
	if (state != ERR && ferror(pipe))
		state = ERR;
	return state;

static void __NORETURN
static void __NORETURN
die(const char *err, ...)
	if (load_options() == ERR)
		die("Failed to load user config.");

	/* Load the repo config file so options can be overwritten from
	 * the command line.  */
	if (load_repo_config() == ERR)
		die("Failed to load repo config.");

	/* Require a git repository unless when running in pager mode. */
	if (refs_size == 0 && opt_request != REQ_VIEW_PAGER)
		die("Not a git repository");


		request = get_keybinding(display[current_view]->keymap, key);
			if (read_prompt() == ERR)
				request = REQ_SCREEN_UPDATE;