/**
 * @file found_a_bug.c
 * @brief function for dumping debug info and quitting simics when finding a bug
 * @author Ben Blum <bblum@andrew.cmu.edu>
 */

#include <simics/api.h>

#define MODULE_NAME "BUG!"
#define MODULE_COLOUR COLOUR_RED

#define INFO_NAME "INFO"
#define INFO_COLOUR COLOUR_BLUE

#include "common.h"
#include "found_a_bug.h"
#include "kernel_specifics.h"
#include "landslide.h"
#include "schedule.h"
#include "tree.h"
#include "x86.h"

/* The default print macros would print big red "[BUG!]"s even if we're just
 * dumping decision info. Redefine them to be flexible around this point. */
#undef lsprintf
#define lsprintf(v, bug_found, ...) do {				\
	if (bug_found) {						\
		_lsprintf(v, MODULE_NAME, MODULE_COLOUR, __VA_ARGS__);	\
	} else {							\
		_lsprintf(v, INFO_NAME, INFO_COLOUR, __VA_ARGS__);	\
	} } while (0)

#undef PRINT_TREE_INFO
#define PRINT_TREE_INFO(v, bug_found, ls) do {				\
	if (bug_found) {						\
		_PRINT_TREE_INFO(v, MODULE_NAME, MODULE_COLOUR, ls);	\
	} else {							\
		_PRINT_TREE_INFO(v, INFO_NAME, INFO_COLOUR, ls);	\
	} } while (0)

/* Only do verbose trace if the user asked for decision info. */
#define VERBOSE_TRACE(bug_found) (!(bug_found))

static int print_tree_from(struct hax *h, int choose_thread, bool bug_found)
{
	int num;

	if (h == NULL) {
		assert(choose_thread == -1);
		return 0;
	}
	
	num = 1 + print_tree_from(h->parent, h->chosen_thread, bug_found);

	if (h->chosen_thread != choose_thread || VERBOSE_TRACE(bug_found)) {
		lsprintf(BUG, bug_found, COLOUR_BOLD COLOUR_YELLOW
			 "%d:\t%lu instructions, old %d new %d, ", num,
			 h->trigger_count, h->chosen_thread, choose_thread);
		print_qs(BUG, h->oldsched);
		printf(BUG, COLOUR_DEFAULT "\n");
		lsprintf(BUG, bug_found, "\t%s\n", h->stack_trace);
	}

	return num;
}

void _found_a_bug(struct ls_state *ls, bool bug_found)
{
	if (bug_found) {
		lsprintf(BUG, bug_found, COLOUR_BOLD COLOUR_RED
			 "****    A bug was found!     ****\n");
		lsprintf(BUG, bug_found, COLOUR_BOLD COLOUR_RED
			 "**** Decision trace follows. ****\n");
	} else {
		lsprintf(BUG, bug_found, COLOUR_BOLD COLOUR_GREEN
			 "(No bug was found.)\n");
	}

	print_tree_from(ls->save.current, ls->save.next_tid, bug_found);

	char *stack = stack_trace(ls->cpu0, ls->eip, ls->sched.cur_agent->tid);
	lsprintf(BUG, bug_found, COLOUR_BOLD COLOUR_RED "Current stack:"
		 COLOUR_DEFAULT " %s\n", stack);
	MM_FREE(stack);

	PRINT_TREE_INFO(BUG, bug_found, ls);

	if (BREAK_ON_BUG) {
		lsprintf(ALWAYS, bug_found, COLOUR_BOLD COLOUR_YELLOW
			 "Now giving you the debug prompt. Good luck!\n");
		SIM_break_simulation(NULL);
	} else {
		SIM_quit(LS_BUG_FOUND);
	}
}
