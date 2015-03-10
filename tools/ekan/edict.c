#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../panini.h"
#include "read.h"
#include "reading.h"

#define PATHTOENGLISH "/languages/english/english"

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

/* This function, given a headword and a reading, 
 * will return the panini code to match or generate that headword/reading.
 */
char * segmentation(char * headword, char * moras, kanji * klist, char * suffix) {

	if(!headword) return 0;
	if(!moras) return 0;
	if(!strlen(headword) && !strlen(moras)) return strdup("");
	if(suffix && !strcmp(moras, suffix)) return strdup("");
	
	kanji * tklist = klist;
	
	int i = 0;
	while(strlen(hiragana[i])) {
		
		if(strncmp(hiragana[i], headword, strlen(hiragana[i]))) {
			i++;
			continue;
		}
		
		char * next = segmentation(headword + strlen(hiragana[i]), moras + strlen(latin[i]), klist, suffix);
		if(!next) return 0;
		
		char * def = malloc(strlen(next) + strlen("(segments ) ") + strlen(latin[i]) + 1);
		strcpy(def, latin[i]);
		strcat(def, " ");
		strcat(def, next);
		free(next);
		return def;
	}
	
	i = 0;
	while(strlen(katakana[i])) {
		
		if(strncmp(katakana[i], headword, strlen(katakana[i]))) {
			i++;
			continue;
		}
		
		char * next = segmentation(headword + strlen(katakana[i]), moras + strlen(latin[i]), klist, suffix);
		if(!next) return 0;
		
		char * def = malloc(strlen(next) + strlen("(segments ) ") + strlen(latin[i]) + 1);
		strcpy(def, latin[i]);
		strcat(def, " ");
		strcat(def, next);
		free(next);
		return def;
	}
	
	while(tklist) {
		if(!tklist->glyph) {
			break;
		}
		if(strncmp(tklist->glyph, headword, strlen(tklist->glyph))) {
			tklist = tklist->next;
			continue;
		}
		
		reading * r = tklist->r;
		while(r) {
			if(!r->moras) {
				r = r->next;
				continue;
			}
			if(!strncmp(r->moras, moras, strlen(r->moras))) {
				char * next = segmentation(headword + strlen(tklist->glyph), moras + strlen(r->moras), klist, suffix);
				if(!next) return 0;
				char * def = malloc(+ strlen(tklist->jiscode) + strlen(r->moras) + strlen(next) + 5);
				strcpy(def, tklist->jiscode);
				strcat(def, "-");
				strcat(def, r->moras);
				strcat(def, " ");
				strcat(def, next);
				free(next);
				return def;
			}
			r = r->next;
		}
		if(!r) return 0;
	}
	return 0;
}
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
                 kanji * klist, char * postag, char * incode, char * jpos, char * flags, char * suffix) {
	
	/* If this is not actually a [noun/verb/adj-i/...] then just quit.
	 * We need to look for the tag between the parentheses. First find 
	 * the left one */
	char * leftparen = strstr(ttemp, "(");
	if(!leftparen) return 0;
	
	/* See if the tag is between after the "(" and before the ")". */
	if(!strstr(leftparen, postag)) return 0;
	if(strstr(leftparen, postag) > strstr(leftparen, ")")) return 0;
	
	/* Remove the part-of-speech tag at the front of the translation. */
	ttemp = strstr(leftparen, ")") + strlen(") ");
	
	/* Also, see if there's a (1) or (2) at the front. */
	if(!strncmp(ttemp, "(1) ", strlen("(1) "))) ttemp += strlen("(1) ");
	if(!strncmp(ttemp, "(2) ", strlen("(2) "))) ttemp += strlen("(2) ");
	
	/* Remove the slash at the end of the translation */
	char * translation = strdup(ttemp);
	char * slash = strstr(translation, "/");
	if(!slash) {
		free(translation);
		return 0;
	}
	slash[0] = '.';
	slash[1] = '\0';
	
	if(!strlen(headword)) return 0;
	
	monad * m = monad_duplicate_all(pmonad);
	if(!panini_parse(m, incode, translation, 0, 0, 20)) {
		monad_free(m);
		return 0;
	}
	
	monad_map(m, remove_ns, "rection", -1);
	monad_map(m, remove_ns, "record", -1);
	monad_map(m, remove_ns, "theta", -1);
	monad_map(m, remove_ns, "clues", -1);
	monad_map(m, remove_ns, "record", -1);

	
	/* this string will hold the code to generate the japanese word */
	char * tr = transliterate(reading);
	char * seg = segmentation(headword, tr, klist, suffix);
	
	if(!seg) return 0;
	
	/* Define a [whatever the part of speech is] */
	printf("; %s [%s] (%s) %s\n", headword, reading, jpos, translation);
	printf("(df %s (flags %s) ", jpos, flags);
	
	/* It should have the right meaning */
	monad_map(m, kill_least_confident, (void *)0, -1);
	monad_map(m, print_seme, stdout, -1);

	printf("(segments %s) (call #))\n\n", seg);
	
/*	fprintf(stderr, "%s [%s] (%s) %s                           \n", \
	        headword, reading, jpos, translation); */
	monad_free(m);
	free(seg);
	free(tr);
	free(translation);
	return 1;
}

int learnentry(char * headword, char * reading, char * translation, kanji * klist) {
	
	learnentry_func(headword, reading, translation, klist, "n",  "(call noun)(lit .)", "noun", "", 0);
	learnentry_func(headword, reading, translation, klist, "pn", "(call noun)(lit .)", "noun", "", 0);
	learnentry_func(headword, reading, translation, klist, "vs", "(call verb)(lit .)", "verb", "suru", 0);
	learnentry_func(headword, reading, translation, klist, "vs", "(call vp to-infinitive)(lit .)", "verb", "suru", 0);
	learnentry_func(headword, reading, translation, klist, "v1", "(call vp to-infinitive)(lit .)", "verb", "ichidan", "る");
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

int main(int argc, char * argv[], char * envp[]) {
	
	FILE * edict = fopen("./edict.txt", "r");
	if(!edict) {
		fprintf(stderr, "Could not open the file edict.txt.\n");
	}
	
	FILE * kanjidic = fopen("./kanjidic.txt", "r");
	if(!kanjidic) {
		fprintf(stderr, "Could not open the file kanjidic.txt.\n");
	} 
	if(!edict || !kanjidic) exit(99);
	
	/* The first line is only a comment */
	//skip_line(edict);
	skip_line(kanjidic);
	
	/* Read in the kanjidic file. */
	kanji * klist = readkanjidic(kanjidic);
	
	/* We'll need a panini monad later (this is used by learnentry_func)
	 */
	pmonad = monad_new();
	char * buildpath = strdup(getenv("PANINI"));
	if(!buildpath) {
		fprintf(stderr, "The environment variable PANINI has not been set");
		exit(99);
	}
	char * fn = malloc(strlen(buildpath) + strlen(PATHTOENGLISH) + 1);
	strcpy(fn, buildpath);
	strcat(fn, PATHTOENGLISH);
	monad_rules(pmonad, fn);
	free(fn);
	
	int elen = count_lines(edict);
	
	/* Read the edict file in line by line */
	int i = 0;
	for(;;) {
		/* Read in the headword. */
		char * headword = readword(edict);
		if(!headword) break;
		if(!strlen(headword)) break;
		
		/* Every 512th line, print out how far we've come. 
		 * (This test is an optimisation; stderr is a slow thing to print to.) 
		 */
		if(i == (i & ~(0777)))
			fprintf(stderr, "EDICT importer: %d%% \r", (i*100)/elen);
		
		/* Print the line number of the EDICT file (good for debugging) */
		//fprintf(stderr, "EDICT importer: %d%% - %d \r", (i*100)/elen, i);
		
		/* Parse the line and try to generate Panini source code */
		readentry(edict, headword, klist);
		
		i++;
	}
	
	/* Print out all the used kanji. */
	compilekanji(klist);
	free_kanji(klist);
	
	fprintf(stderr, "All done!            \r");
	
	return 0;
}
