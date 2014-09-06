#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <panini.h>
#include "read.h"
#include "reading.h"
monad * pmonad;
/* 
 * This program reads in all the entries in the edict.locale file. This is 
 * automatically adapted (see Makefile) from the edict file prepared by Jim
 * Breen and others at the Monash University. I extend a gesture of gratitude
 * to those who made this possible.
 * 
 * (The file itself is copied by build.sh from /usr/share/edict/, where Debian
 * puts the file.)
 * 
 * This program is part of the Panini Project, which aims to unify various 
 * computer-readable dictionaries and provide something of a computational
 * linguistics Swiss Army knife. 
 * 
 * This program reads in edict and spits out Panini source code (the functional 
 * equivalent of (mostly) the same data, but usable by libpanini.
 */

/* This function, given these parameters, will determine the meaning of
 * the English translation, and then creates a Panini function to
 * match the headword to this meaning.
 * 
 * char * headword - we will try to interpret this English word to get 
 *                   the meaning.
 * char * reading  - The phonemic shape of the Japanese word.
 * char * ttemp    - A length of text (looking at the EDICT file, this
 *                   may be any of the strings delimited by /.
 * kanji * klist   - This is the list of kanji that was generated by
 *                   the readkanjidic file.
 * char * postag   - The part-of-speech tag (eg. (n), (v), (adj-na) ...)
 * char * incode   - This string contains the Panini code the interpret
 *                   the string in ttemp.
 * char * jpos     - This is the name of the Japanese part of speech
 *                   (eg. noun, verb etc.)
 */
int learnentry_func(char * headword, char * reading, char * ttemp, \
                 kanji * klist, char * postag, char * incode, char * jpos) {
	
	if(!klist) {
		printf("No klist!\n");
		return 0;
	}
	
	
	/* Remove the part-of-speech tag at the front of the translation. */
	int poslen = strlen(postag);
	if(!strncmp(ttemp, postag, poslen)) ttemp += poslen + 1;
	
	/* Remove the slash at the end of the translation */
	char * translation = strdup(ttemp);
	char * slash = strstr(translation, "/");
	if(!slash) {
		free(translation);
		return 0;
	}
	slash[0] = ' ';
	slash[1] = '\0';
	
	if(!strlen(headword)) return 0;
	
	monad * m = monad_duplicate_all(pmonad);
	monad_map(m, unlink_the_dead, (void*)0, -1);
	monad_map(m, set_intext, (void *)translation, -1);
	monad_map(m, set_stack, incode, -1);
	if(!monad_map(m, tranny_parse, (void *)0, 20)) {
		monad_free(m);
		return 0;
	}
	
	
	monad_map(m, remove_ns, "rection", -1);
	monad_map(m, remove_ns, "record", -1);
	monad_map(m, remove_ns, "theta", -1);
	monad_map(m, remove_ns, "clues", -1);
	monad_map(m, remove_ns, "record", -1);

	/* Find the kanji in the list */
	while(klist) {
		if(!klist->glyph) return 0;
		if(strcmp(klist->glyph, headword)) {
			klist = klist->next;
			//printf("\n;strcmp(\"%s\", \"%s\")\n", klist->glyph, headword); 
			continue;
		} else {
			klist->used = 1;
			break;
		}
	}
	
	/* No kanji found? Then just stop here. */
	if(!klist) return 0;
	
	/* Define the Japanese word */
	printf("(df %s ", jpos);
	
	/* It should call the right kanji,*/
	char * r = transliterate(reading);
	printf("(segments %s-%s) ", klist->jiscode, r);
	free(r);
	
	/* It should have the right meaning */
	monad_map(m, kill_least_confident, (void *)0, -1);
	monad_map(m, print_seme, stdout, -1);

	printf(")\n");
	
	fprintf(stderr, "%s [%s] (%s) %s                           \n", \
	        headword, reading, jpos, translation);
	monad_free(m);
	return 1;
}

int learnentry_n(char * headword, char * reading, char * translation, kanji * klist) {
	if(learnentry_func(headword, reading, translation, klist, "(n)", "(constituent Headword noun)", "noun")) return 1;
	if(learnentry_func(headword, reading, translation, klist, "(n,n-suf)", "(constituent Headword noun)", "noun")) return 1;
} 

int learnentry(char * headword, char * reading, char * translation, kanji * klist) {
	if(learnentry_n(headword, reading, translation, klist)) return 1;
	return 0;
}

int readentry(FILE * kanjidic, char * headword, kanji * klist) {
	char * line = readline(kanjidic);
	
	if(!strstr(line, "[")) return 1;
	if(!strstr(line, "]")) return 1;
	
	char * reading = strdup(strstr(line, "[") + strlen("["));
	char * nulterm = strstr(reading, "]");
	nulterm[0] = '\0';
	
	char * translation = strstr(line, "/") + strlen("/");
	while(translation) {
		learnentry(headword, reading, translation, klist);
		if(!strstr(translation, "/")) break;
		translation = strstr(translation, "/") + strlen("/");
	}
	
	return 0;
}

int main(int argc, char * argv[]) {
	
	FILE * edict = fopen("./edict.locale", "r");
	if(!edict) {
		fprintf(stderr, "Could not open the file edict.locale.\n");
		fprintf(stderr, "Exiting.\n");
	}
	
	FILE * kanjidic = fopen("./kanjidic.locale", "r");
	if(!kanjidic) {
		fprintf(stderr, "Could not open the file kanjidic.locale.\n");
		fprintf(stderr, "Exiting.\n");
	} 
	/* The first line is only a comment */
	//skip_line(edict);
	skip_line(kanjidic);
	
	fprintf(stderr, "I have opened both edict.locale and kanjidic.locale.\n");
	/* Read in the kanjidic file. */
	kanji * klist = readkanjidic(kanjidic);
	fprintf(stderr, "Parsed the KANJIDIC file.\n");
	
	/* We'll need a panini monad later (this is used by learnentry_func)
	 */
	pmonad = monad_new();
	monad_rules(pmonad, "english");
	
	int elen = count_lines(edict);
	
	/* Read the edict file in line by line */
	int i = 0;
	for(;;) {
		/* Read in the headword. */
		char * headword = readword(edict);
		if(!headword) break;
		if(!strlen(headword)) break;
		
		/* Every 32nd line, print out how far we've come. 
		 * (This test is an optimisation; the terminal is a slow thing to print to.) */
		if(i == (i & ~(017)))
			fprintf(stderr, "EDICT importer: %d%% %s                            \r", (i*100)/elen, headword);
		
		/* Parse the line and try to generate Panini source code */
		readentry(edict, headword, klist);
		
		i++;
	}
	
	/* Print out all the used kanji. */
	compilekanji(klist);
	free_kanji(klist);
	
	return 0;
}
