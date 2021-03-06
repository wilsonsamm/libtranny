#include "../monad/monad.h"
#include "panini.h"
#include "tranny.h"
#include <stdlib.h>

#include <string.h> 

void post_pc(monad * m) {
	list_free(m->command);
	m->command = 0;
}

int panini_despatch(monad * m, int * switches) {
		
	/* Pop the next command */
	monad_popcom(m);
	
	/* If there were no more commands to execute, then we're done and the monad
	 * survives. */
	if(!m->command) return 0;
		
	/* Don't bother continuing if the monad is dead */
	if(!m->alive) return 0;
	
	/* Debugging info */
	if(m->debug) {
		printf("Monad %d is executing: ", m->id);
		if(m->command) list_prettyprinter(m->command);
		printf("\n");
		printf("Switches: %d\n", *switches);
	}
	
	char * command = list_get_token(m->command, 1);

	if(!strcmp(command, "add-flags-to-df")) {
		add_flags_to_df(m);
		post_pc(m);
		return 1;
	}

	if(!strcmp(command, "add-ns-to-df")) {
		add_ns_to_df(m);
		post_pc(m);
		return 1;
	}

	if(!strcmp(command, "add-record-to-df")) {
		add_record_to_df(m);
		post_pc(m);
		return 1;
	}

	if(!strcmp(command, "add-tag-to-df")) {
		add_tag_to_df(m);
		post_pc(m);
		return 1;
	}
	
	if(!strcmp(command, "adjunct")) {
		panini_call(m, 1, switches);
		post_pc(m);
		return 1;
	}
	
	/* (all-caps)
	 * unimplemented for now. */
	if(!strcmp(command, "all-caps")) {
		m->alive = 0;
		post_pc(m);
		return 1;
	}
	
	/* (brake)
	 * Increments the brake register. If this goes beyond some threshold, 
	 * execution will stop for that monad. */
	if(!strcmp(command, "brake")) {
		m->brake++;
		post_pc(m);
		return 1;
	}
	
	/* (bootstrap)
	 * This function has been especially written so that semantic primes may be
	 * automatically bootstrapped. This means, that suppose you come across some
	 * as yet unknown word, in some input, then it will be bound to the
	 * (head ...) variable, and then picked up by some (open ...) routine which
	 * will generate the dictionary definition.
	 */
	if(!strcmp(command, "bootstrap-seme")) {
		learn_bootstrap(m);
		post_pc(m);
		return 1;
	}

	/* (call)
	 */
	if(!strcmp(command, "call")) {
		panini_call(m, 0, switches);
		post_pc(m);
		return 1;
	}
	/* (check)
	 */
	if(!strcmp(command, "check")) {
		monad_parse_check(m);
		post_pc(m);
		return 1;
	}
	
	/* (confidence 12) 
	 * Each monad has in it an integer called confidence. This instruction adds
	 * an integer to that. */
	if(!strcmp(command, "confidence")) {
		m->confidence += atoi(list_get_token(m->command, 2));;
		post_pc(m);
		return 1;
	}
	
	/* (debug) 
	 * Each monad has a thing called "debug". If this gets turned on, then as it
	 * executes, you get some tracecode to stdout, which you may use to debug 
	 * your language module */
	if(!strcmp(command, "debug")) {
		m->debug = 1;
		post_pc(m);
		return 1;
	}
	
	/* (flags)
	 * Does nothing. */
	if(!strcmp(command, "flags")) {
		post_pc(m);
		return 1;
	}
	
	if(!strcmp(command, "fork")) {
		panini_fork(m, switches);
		post_pc(m);
		return 1;
	}
	if(!strcmp(command, "fuzzy")) {
		panini_fuzzy(m);
		post_pc(m);
		return 1;
	}
	
	if(!strcmp(command, "into")) {
		monad_parse_into(m, (*switches & OUTTEXT), (*switches & INTEXT));
		post_pc(m);
		return 1;
	}
	
	/* (language ...) */
	if(!strcmp(command, "language")) {
		if(checkvars(m, m->command, 0)) m->alive = 0;
		post_pc(m);
		return 1;
	}

	/* (lit )
	 * reads in a string from the intext or appends to the outtext */
	if((*switches & INTEXT)  && !strcmp(command, "lit")) {
		scan_intext(m, list_get_token(m->command, 2));
		post_pc(m);
		return 1;
	}
	if((*switches & OUTTEXT)  && !strcmp(command, "lit")) {
		append_to_outtext(m, list_get_token(m->command, 2));
		post_pc(m);
		return 1;
	}

	
	/* (nop)
	 * Does nothing. */
	if(!strcmp(command, "nop")) {
		post_pc(m);
		return 1;
	}

	/* (open ...)
	 * Opens a lexical class.
	 * Either learns a morpheme, or copies it to the ontology (for names and
	 * such, which are not translated but simply transferred verbatim into the
	 * target text) */
	if(!strcmp(command, "open")) {
		if(*switches & L_OPEN) {
			if(m->debug) {
				printf("learning ...");
			}
			monad_learn_open(m);
		}
		else {
			if(m->debug) {
				printf("dying ...");
			}
			m->alive = 0;
		}
//		if(*switches & P_OPEN) monad_parse_open(m);
		post_pc(m);
		return 1;
	}
	
	/* (phrase ...) */
	if(!strcmp(command, "phrase")) {
		tranny_phrase_ops(m, command);
		post_pc(m);
		return 1;
	}
	
	/* (rection ...) */
	if(!strcmp(command, "rection")) {
		if(checkvars(m, m->command, 0)) m->alive = 0;
		post_pc(m);
		return 1;
	}

	if(!strcmp(command, "recorded-segments")) {
		list_append_token(m->command, "segments");
		list * record = get_namespace(m, "record", 0);
		list * newstack = list_new();
		list * next = list_append_list(newstack);
		list_append_token(next, "segments");
		list_append_copy(next, record);
		list_drop(next, 2);
		list_append_copy(newstack, m->stack);
		list_free(m->stack);
		m->stack = newstack;
		remove_ns(m, "record");
		get_namespace(m, "record", 1);
		post_pc(m);
		return 1;
	}
	/* (return)
	 * Returns from a matching (into ...) instruction. */
	if(!strcmp(command, "return")) {
		monad_parse_return(m);
		post_pc(m);
		return 1;
	}
	
	/* (sandhi ...) */
	if(!strcmp(command, "sandhi")) {
		if(checkvars(m, m->command, 0)) m->alive = 0;
		post_pc(m);
		return 1;
	}
	
	/* (sandhiblock) 
	 * Removes any sandhi information. */
	if(!strcmp(command, "sandhiblock")) {
		remove_ns(m, "sandhi");
		post_pc(m);
		return 1;
	}
	
	/* (seme ...) */
	if(!strcmp(command, "seme")) {
		if(m->debug) {
			printf("calling checkvars(m, m->command, %d)", !(*switches & CR_SEME));
		}
		if(checkvars(m, m->command, !(*switches & CR_SEME))) m->alive = 0;
		post_pc(m);
		return 1;
	}

	/* (space)
	 * reads in a space from the intext or appends to the outtext */
	if((*switches & INTEXT)  && !strcmp(command, "space")) {
		scan_intext(m, " ");
		post_pc(m);
		return 1;
	}
	if((*switches & OUTTEXT)  && !strcmp(command, "space")) {
		append_to_outtext(m, " ");
		post_pc(m);
		return 1;
	}
	
	if(!strcmp(command, "segments")) {
		//panini_segments(m, (*switches & OUTTEXT) || (*switches & L_OPEN));
		panini_segments(m, !!(*switches & OUTTEXT) && !!(*switches & RECORD));
		post_pc(m);
		return 1;
	}
	
	if(!strcmp(command, "tag")) {
		if(*switches & L_TAG) learn_tag(m);
		post_pc(m);
		return 1;
	}
	
	/* (theta ...) */
	if(!strcmp(command, "theta")) {
		if(checkvars(m, m->command, 0)) m->alive = 0;
		post_pc(m);
		return 1;
	}
	
	/* (unbrake)
	 * Undoes the effect of a matching (brake). */
	if(!strcmp(command, "unbrake")) {
		m->brake--;
		post_pc(m);
		return 1;
	}
	
	/* Is the command one of the ones deals with memory? */
	//if(tranny_memory(m, command)) return 1;
	
	printf("No such command as %s.\n", command);

	m->alive = 0;
	return 0;
}

int panini_addrecord(monad * m, void * nothing) {
	get_namespace(m, "record" , 1);
	return 0;
}

int panini_parse(monad *m, char * commands, char * intext, int editdistance, int record, int threshold) {
	
	int retval;
	int switches = CR_SEME | INTEXT;
	
	/* If we want to record the segments, then do so */
	//if(record) switches |= RECORD;
	if(record) monad_map(m, panini_addrecord, 0, -1);
	
	/* If we have to set some edit distance, then do so */
	if(editdistance) monad_map(m, (int(*)(monad * m, void * argp))set_edit, &editdistance, threshold);
	
	/* First, set the stack to contain the right instructions */
	monad_set_stack(m, commands);
//	monad_map(m, (int(*)(monad * m, void * argp))set_stack, commands, threshold);
	
	/* Next, set the INTEXT up */
	monad_map(m, (int(*)(monad * m, void * argp))set_intext, intext, threshold);
	
	/* Then, parse the instructions and the INTEXT */
	retval = monad_map(m, (int(*)(monad * m, void * argp))panini_despatch, &switches, threshold);
	
	/* Then, kill any monad that didn't finish the program. */
	monad_map(m, kill_not_done, (void *)0, -1);
	
	/* and free any monads that are not still alive
	 * (this forgets their state so that the memory becomes free. */
	monad_unlink_dead(m, 0);
		
	return retval;
}

int panini_learnvocab(monad * m, char * commands, FILE * out, char * intext, int threshold) {
	
	int retval;
	int switches = INTEXT | L_OPEN | CR_SEME;
	/* First, set the stack to contain the right instructions */
	monad_set_stack(m, commands);
//	monad_map(m, (int(*)(monad * m, void * argp))set_stack, commands, threshold);
	
	/* Next, set the INTEXT up */
	monad_map(m, (int(*)(monad * m, void * argp))set_intext, intext, threshold);
	
	/* Then, learn the language given the intext */
	monad_map(m, (int(*)(monad * m, void * argp))panini_despatch, &switches, threshold);
	
	/* Then, kill any monad that didn't finish the program, */
	retval = monad_map(m, kill_not_done, (void *)0, -1);
	
	/* and any that have not finished processing the intext */
	monad_kill_unfinished_intext(m);

	/* and then any that have a higher brake value than any other, */
	monad_kill_braked(m);

	/* and any that didn't learn anything, */
	monad_kill_if_no_df(m);

	/* and then any that have a lower confidence value than any other, */
	panini_keep_confident(m);
	
	/* and then free any monads that are not still alive
	 * (this forgets their state so that the memory becomes free. */
	monad_unlink_dead(m, 0);

	/* Then print out what we learned. */
	if(out) {
		monad_map(m, (int(*)(monad * m, void * argp))print_df, out, threshold);
		fflush(out);
	}
	
	return retval;
}

int panini_learnpp(monad * m, char * commands, FILE * out, char * intext, int threshold) {
	
	int retval;
	int switches = INTEXT | L_TAG;
	
	/* First, set the stack to contain the right instructions */
	monad_set_stack(m, commands);
//	monad_map(m, (int(*)(monad * m, void * argp))set_stack, commands, threshold);
	
	/* Next, set the INTEXT up */
	monad_map(m, (int(*)(monad * m, void * argp))set_intext, intext, threshold);
	
	/* Then, learn the language given the intext */
	monad_map(m, (int(*)(monad * m, void * argp))panini_despatch, &switches, threshold);
	
	/* Then, kill any monad that didn't finish the program, */
	retval = monad_map(m, kill_not_done, (void *)0, -1);
	
	/* and any that have not finished processing the intext */
	monad_kill_unfinished_intext(m);

	/* and then any that have a higher brake value than any other, */
	monad_kill_braked(m);

	/* and any that didn't learn anything, */
	monad_kill_if_no_df(m);
	
	/* and any that have an identical outtext to any other monad, */
	monad_map(m, (int(*)(monad * m, void * argp))kill_identical_outtexts, (void*)0, -1);
	
	/* and then free any monads that are not still alive
	 * (this forgets their state so that the memory becomes free. */
	monad_unlink_dead(m, 0);

	/* Then print out what we learned. */
	if(out) {
		monad_map(m, (int(*)(monad * m, void * argp))print_df, out, threshold);
		fflush(out);
	}
	
	return retval;
}

int panini_generate(monad *m, char * commands, int record, int threshold) {
	
	int retval;
	int switches = OUTTEXT | CR_SEME;
	
	/* If we want to record the segments, then do so */
	if(record) switches |= RECORD;
	
	/* First, set the stack to contain the right instructions */
	monad_set_stack(m, commands);
//	monad_map(m, (int(*)(monad * m, void * argp))set_stack, commands, threshold);
	
	/* Then, parse the instructions, generating OUTTEXT */
	retval = monad_map(m, (int(*)(monad * m, void * argp))panini_despatch, &switches, threshold);
	
	/* Then, kill any monad that didn't finish the program
     * and free any dead monads so that they don't occupy memory. */
	monad_map(m, kill_not_done, (void *)0, -1);
	monad_unlink_dead(m, 0);
	
	return retval;
}

int panini_keep_confident(monad * m) {
	
	/* Find the highest confidence of all the monads */
	int confidence = 0;
	monad_map(m, (int(*)(monad * m, void * argp))max_confidence, &confidence, -1);
	
	/* The kill all monads that are less confident */
	int retval = monad_map(m,  (int(*)(monad * m, void * argp))kill_less_confident, &confidence, -1);
	
	monad_unlink_dead(m, 0);
	return retval;
}

monad * panini_test(char * srclang, char * srcexec, char * intext, char * dstlang, char * dstexec) {

	monad * m = monad_new();

	/* As far as we know, does this word exist in the srclanguage? */
	monad_rules(m, srclang);
	if(!panini_parse(m, srcexec, intext, 0, 0, 5)) {
		monad_free(m);
		return 0;
	}
	
	/* Do we already know how to put it in the dstlanguage? */
	monad_rules(m, dstlang);
	monad * n = monad_duplicate(m);
	if(panini_generate(m, dstexec, 0, 5)) {
		monad_free(m);
		monad_free(n);
		return 0;
	}
	monad_free(m);
	
	return n;
}
