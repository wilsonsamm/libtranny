/* This file contains functions that are visible to the outside world. */

#include "monad.h"
#include "../list/list.h"
#include <stdlib.h>
#include <string.h>

monad * monad_new() {
	monad * m = malloc(sizeof(monad));
	
	m->rules = 0;
	m->ralloc = 0;
	m->alive = 1;
	m->namespace = 0;
	m->stack = 0;
	m->child = 0;
	m->command = 0;
	m->id = 1;
	m->outtext = 0;
	m->index = 0;
	m->learned = 0;
	m->trace = 0;
	m->debug = 0;
	m->confidence = 0;
	m->adjunct = 0;
	m->debug = 0;
	m->brake = 0;
	m->intext = 0;
	m->outtext = 0;
	m->parent_id = 0;

	return m;
}

/* This function copies a monad. (it doesn't copy the monad's children though). */
monad * monad_duplicate(monad * m) {
	if(!m) return 0;
	while(!m->alive) {
		m = m->child;
		if(!m) return 0;
	}

	monad * n = monad_new();
	n->rules = m->rules;
	n->ralloc = 0;
	n->alive = m->alive;
	n->namespace = list_new();
	if(m->namespace) list_append_copy(n->namespace, m->namespace);
	
	n->stack = list_new();
	if(m->stack) list_append_copy(n->stack, m->stack);
	n->command = list_new();
	if(m->command) list_append_copy(n->command, m->command);

	n->id = m->id;

	if(m->outtext) n->outtext = strdup(m->outtext);
	n->index = m->index;
	n->trace = m->trace;
	n->debug = m->debug;
	n->confidence = m->confidence;
	n->adjunct = 0;
	n->brake = m->brake;
	n->learned = m->learned;
	n->child = 0; 

	return n;
}

monad * monad_duplicate_all(monad * m) {
	if(!m) return 0;
	
	monad * n = monad_duplicate(m);
	n->child = monad_duplicate_all(m->child);
	return n;
}

void monad_set_trace(monad * m, int trace) {
	while(m) {
		m->trace = trace;
		m = m->child;
	}
}

void monad_free(monad * m) {
	if(!m) return;
	
	while(m) {
		if(m->ralloc) list_free(m->rules);
		if(m->outtext)	free(m->outtext);
		if(m->namespace)	list_free(m->namespace);
		if(m->stack)	list_free(m->stack);

		monad * tmp = m->child;
		free(m);
		m = tmp;
	}
}

list * __monad_rule_loader(char * lang) {
	/* Prepare the filename */
	char * fn = malloc(strlen(lang) + strlen(FN_PATH) + 1);
	if(lang[0] != '.' && lang[0] != '/') {
		strcpy(fn, FN_PATH);
		strcat(fn, lang);
	} else {
		strcpy(fn, lang);
	}

	/* Open the file and tokenise it, and then close it */
	FILE * fp = fopen(fn, "r");
	free(fn);
	list * rules = list_new();

	list_tokenise_file(rules, fp);

	fclose(fp);

	return rules;
}

int monad_rules(monad * m, char * lang) {

	list * rules = __monad_rule_loader(lang);

	/* ralloc is for memory management reasons. monad_free and monad_rules use
	 * it to determine when to free the rules. */
	if(m->ralloc) list_free(m->rules);
	m->rules = rules;
	m->ralloc = 1;
	
	/* Set the RULES register on all the other monads to point to the rules we
	 * just loaded */
	m = m->child;
	while(m) {
		m->ralloc = 0;
		m->rules = rules;
		m = m->child;
	}

	return 0;
}

int monad_popcom(monad * m) {
	if(!m->stack) {
		fprintf(stderr, "Monad %d has no stack!\n", m->id);
	}
	
	if(m->stack->length == 0) {
		if(m->debug) {
			printf("Monad %d had nothing left to do so I've put ", m->id);
			printf("a NULL pointer into the INSTRUCTION register ");
		}
		return 0;
	}

	m->command = list_new();

	list * com = list_get_list(m->stack, 1);
	if(!com) {
		fprintf(stderr, "What is this? Something is wrong with the stack in ");
		fprintf(stderr, "Monad %d. \nHere it is: ", m->id);
		list_prettyprinter(m->stack);
		m->alive = 0;
	}

	list_append_copy(m->command, com);

	list_drop(m->stack, 1);

	return 0;
}

void print_debugging_info(monad * m) {
	printf("\n\nStepping Monad %d. ", m->id);
	printf("This monad is %s.\n", m->alive ? "alive":"dead");
	if(m->stack) {
		printf("Stack:");
		list_prettyprinter(m->stack);
	} else {
		printf("This monad has no stack.");
	}
	print_ns(m, (void*)0);
	
	printf("\n");
	printf("Intext: (%d) \"%s\"\n", m->index, m->intext);
	printf("Outtext: \"%s\"\n", m->outtext);
	if(m->namespace) {
		printf("Namespace: ");
		list_prettyprinter(m->namespace);
	} else {
		printf("No namespace.\n");
	}
	printf("\n");
	
	if(m->adjunct) printf("Adjuncts starting from monad %d\n", m->adjunct->id);
	printf("\nParent: %d\n", m->parent_id);
}

/* This function joins two linked lists of monads together. 
 * (this used to crawl to the end of the linked list "to", but that is 
 * generally the longest one. Now a simple improvement is to find the 
 * end of the shortest one, "addition", and slot the rest of the long 
 * one onto the end of the short one.
 * (My profiler showed that a basic run of the program spent more than 
 * 95% of its time in this function!!! That has now been reduced to less
 * than 0.2%.
 * The monads will be run in a different order, but that's OK.)
 */
void monad_join(monad * to, monad * addition) {
	if(!addition) return;
	
	monad * last = addition;
	monad * temp = 0;
	while(last->child) last = last->child;
	temp = to->child;
	to->child = addition;
	last->child = temp;
}

/* This function calls the chosen function on the monads that:
 *  - are still alive. 
 *  - the BRAKE register must not be higher than the given threshold 
 * And returns:
 *  0 if none if the monads are still alive after calling the function 
 *  1 if there are live monads left.
 *
 * If the function returns a non-zero value, then it will be called on the monad again.
 */
int monad_map(monad * m, int(*fn)(monad * m, void * argp), void * arg, int thr) {

	int retval = 0;
	
	monad * beginning = m;
	
	int i = 0;
	
	/* Run! */
	while(m) {
		
		/* Every nth time we step across a monad, we'll run something across the
		 * chain of monads which unlinks the dead ones. This should keep the 
		 * program from allocating gigabytes and gigabytes of memory 
		 * unnecessarily. */
		i++;
		if(i>10000) {
			//fprintf(stderr,"Housekeeping triggered: i= %d; ", i);
			//fprintf(stderr,"m->id = %d;\n", m->id);
			i = 0;
			monad_unlink_dead(beginning, m);
			continue;
		}
		
		if(m->trace == m->id) m->debug = 1;
		if(!m->alive) m->debug = 0;
		if(m->debug) print_debugging_info(m);

		/* Make sure the monad we are about to process really is alive.*/
		if(!m->alive) {
			m = m->child;
			continue;
		}
		
		/* And its BRAKE register must be below the threshold (if there is a threshold at all */
		if(thr != -1 && m->brake > thr) {
			if(m->debug && m->stack) {
				fprintf(stderr, "BRAKE = %d.  THRESHOLD = %d.\n", m->brake, thr);
				fprintf(stderr, "This monad has been skipped since it's BRAKE register is too high.\n");
			}
			
			m = m->child;
			continue;
		}
		
		/* If the adjunct register has been set, then we should proces those monads first. If they live, this monad will die
		 * and vice versa. */
		if(m->adjunct) {
			if(m->debug) printf("Going to do the Adjunct monads now.\n");
			if(monad_map(m->adjunct, fn, arg, thr)) {
				if(m->debug) printf("The Adjunct monads survived, so this monad will now die.\n");
				m->alive = 0;
			} else {
				if(m->debug) printf("The Adjunct monads all died, so this monad will now keep going.\n");
			}
			monad_join(m, m->adjunct); /* Still need to keep it, for memory management reasons */
			m->adjunct = 0;
		}
		
		/* Call the function then! */
		if(!fn(m, arg))	{
			if(m->alive) retval = 1;
			
			m = m->child;
			continue;
		}
	}

	return retval;
}

void monad_unlink_dead(monad * m, monad * end) {
	if(!m) return;
	while(m->child) {
		if(m        == end) return;
		if(m->child == end) return;
		monad * n = m->child;

		if(!n) {
			m = n;
			continue;
		}
		if(n->alive) {
			m = n;
			continue;
		}
	
		if(n->command)	 list_free(n->command);
		if(n->stack)	 list_free(n->stack);
		if(n->namespace) list_free(n->namespace);
		if(n->outtext)   free(n->outtext);

		monad * tmp = n->child;
		free(n);
		m->child = tmp;
		m = m->child;
		if(!m) return;
	}
}

void monad_kill_braked(monad * m) {
	int brake = 9999999;
	
	// Find the lowest value for m->brake
	monad * tmp = m;
	while(tmp) {
		if(!tmp->alive) {
			tmp = tmp->child;
			continue;
		}
		//printf("Comparing %d with %d. ",tmp->brake,brake);
		if(tmp->brake < brake) brake = tmp->brake;
		//printf("The winner is %d.\n", brake);
		tmp = tmp->child;
	}
	
	// kill any monads with a higher brake value.
	// (And any that have a brake value lower than zero. This would only happen
	// due to a bug in a language module
	while(m) {
		if((m->brake > brake) || (m->brake < 0)) {
			//printf("A brake value of %d kills the monad.\n", m->brake);
			m->alive = 0;
		}
		m = m->child;
	}
}
	
void monad_child_tester(monad * m) {
	while(m->child) m = m->child;
}

void monad_kill_unfinished_intext(monad * m) {
	while(m) {
		if(m->intext[m->index] != '\0') m->alive = 0;
		m = m->child;
	}
}

void monad_keep_first(monad * m) {
	/* find first monad that's alive */
	while(m) {
		if(m->alive) break;
		m = m->child;
	}
	if(!m) return;
	/* Kill the rest */
	m = m->child;
	while(m) {
		m->alive = 0;
		m = m->child;
	}
	return;
}

void monad_kill_if_no_df(monad * m) {
	list * df;
	while(m) {
		df = get_namespace(m, "df", 0);
		if(!df) {
			m->alive = 0;
		} else if(df->length < 2) {
			m->alive = 0;
		}
		m = m->child;
	}
}

void monad_set_stack(monad * m, char * stack) {

	
	list * st = list_new();
	list_tokenise_chars(st, stack);

	while(m) {
		while(!m->alive) m = m->child;
		if(m->stack) list_free(m->stack);
		m->stack = list_new();
		list_append_copy(m->stack, st);
		m = m->child;
	}
	return;
}
