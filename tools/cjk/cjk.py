import os
import codecs
import csv

def addtodb(fields, db):
	""" Given a dictionary, db, and a list, ('a', 'b', 'c'), looks up in db
	another dictionary by the name 'a', and adds the key value pair b,c in that
	dictionary. A useful example:
	Call the function with: fields = ('U+638E', 'kCantonese', 'gei2'), db = {}
	When the function returns, db = {u'U+638E': {'kCantonese': [u'gei2']}}
	"""
	if fields[0] not in db:
		db[fields[0]] = {}
	codepoint = db[fields[0]]

	if 'glyph' not in codepoint:
		codepoint['glyph'] = unichr(int(fields[0][2:], 16))
		
	if fields[1] == 'kMandarin':
		codepoint['kMandarin'] = fields[2][:-1].split(' ')
	
	if fields[1] == 'kCantonese':
		codepoint['kCantonese'] = fields[2][:-1].split(' ')
	
	if fields[1] == 'kTraditionalVariant':
		codepoint['kTraditionalVariant'] = fields[2][:-1].split(' ')
		codepoint['trad_glyph'] = unichr(int(fields[2].split(' ')[0][2:], 16))
	
	if fields[1] == 'kSimplifiedVariant':
		codepoint['kSimplifiedVariant'] = fields[2][:-1].split(' ')
		codepoint['simp_glyph'] = unichr(int(fields[2].split(' ')[0][2:], 16))

def readUnihanFile(fn, db):
	filename = os.environ['PANINI'] + 'resources/unihan/' + fn
	with codecs.open(filename, 'r', 'UTF-8') as f:
		for line in f:
			fields = line.split('	')
			if len(fields) == 3:
				addtodb(fields, db)

def addpinyins(db):
	pinyintable = {}

	for codepoint in db:
		cp = db[codepoint]
		if 'pinyin' not in cp:
			cp['pinyin'] = []
		if 'kMandarin' in cp:
			for syllable in cp['kMandarin']:
				if syllable in pinyintable:
					cp['pinyin'].append(pinyintable[syllable])
				else:
					with codecs.open('badsyllables.csv', 'a', 'UTF-8') as bad:
						bad.write(syllable+',\n')

def createmandarin(db):
	pinyintable = {}
	mandarin = codecs.open('mandarin.panini', 'w', 'UTF-8')
	
	with codecs.open('mandarin.csv', 'r', 'UTF-8') as csvfile:
		for row in csvfile:
			pinyintable[row.split(',')[0]] = row.split(',')[1][:-1]
			
	hsk = {}
	
	with codecs.open('hsk.csv', 'r', 'UTF-8') as csvfile:
		for row in csvfile:
			hsk[row.split(',')[0]] = row.split(',')[1][:-1]

	for codepoint in db:
		cp = db[codepoint]
		if 'kMandarin' in cp:

			# See if the glyph is in the HSK syllabus
			if 'simp_glyph' in cp:
				sg = cp['simp_glyph']
			else:
				sg = cp['glyph']
			if sg in hsk:
				cp['hsk'] = hsk[sg]
			else:
				continue
			
			for syllable in cp['kMandarin']:
				segmentname = codepoint[2:]+'-'+pinyintable[syllable]
				if syllable not in pinyintable:
					with codecs.open('badsyllables.csv', 'a', 'UTF-8') as bad:
						bad.write(syllable+',\n')
				else:
					mandarin.write('(segment ')
					mandarin.write('(underlying '+ segmentname +') ')
					mandarin.write('(language (language mandarin) (orthography pinyin)) ')
					mandarin.write('(lit '+ syllable + ') (space))\n')
					
					mandarin.write('(segment ')
					mandarin.write('(underlying '+ segmentname +') ')
					mandarin.write('(language (language mandarin)')
					mandarin.write(' (orthography pinyin2)) ')
					mandarin.write('(lit ' + pinyintable[syllable] + ') (space))\n')

					if 'kSimplifiedVariant' not in cp and 'kTraditionalVariant' not in cp:
						mandarin.write('(segment ')
						mandarin.write('(underlying '+ segmentname +') ')
						mandarin.write('(language (language mandarin)')
						mandarin.write(' (orthography simplified)) ')
						mandarin.write('(lit ' + cp['glyph'] + '))\n')
						mandarin.write('(segment ')
						mandarin.write('(underlying '+ segmentname +') ')
						mandarin.write('(language (language mandarin)')
						mandarin.write(' (orthography traditional)) ')
						mandarin.write('(lit ' + cp['glyph'] + '))\n')

					if 'kSimplifiedVariant' in cp and 'kTraditionalVariant' not in cp:
						mandarin.write('(segment ')
						mandarin.write('(underlying '+ segmentname +') ')
						mandarin.write('(language (language mandarin)')
						mandarin.write(' (orthography simplified)) ')
						mandarin.write('(lit ' + cp['simp_glyph'] + '))\n')
						mandarin.write('(segment ')
						mandarin.write('(underlying '+ segmentname +') ')
						mandarin.write('(language (language mandarin)')
						mandarin.write(' (orthography traditional)) ')
						mandarin.write('(lit ' + cp['glyph'] + '))\n')

					if 'kSimplifiedVariant' not in cp and 'kTraditionalVariant' in cp:
						mandarin.write('(segment ')
						mandarin.write('(underlying '+ segmentname +') ')
						mandarin.write('(language (language mandarin)')
						mandarin.write(' (orthography simplified)) ')
						mandarin.write('(lit ' + cp['glyph'] + '))\n')
						mandarin.write('(segment ')
						mandarin.write('(underlying '+ segmentname +') ')
						mandarin.write('(language (language mandarin)')
						mandarin.write(' (orthography traditional)) ')
						mandarin.write('(lit ' + cp['trad_glyph'] + '))\n')

				mandarin.write('(df guess-cjk (segments ' + segmentname+ '))\n')
					
	mandarin.close()

def createcanto(db):
	canto = codecs.open('canto.panini', 'w', 'UTF-8')
	for codepoint in db:
		cp = db[codepoint]
		if 'kCantonese' in cp:
			for jyutping in cp['kCantonese']:
				segmentname = codepoint[2:]+'-'+jyutping
				
				canto.write('(segment ')
				canto.write('(underlying '+ segmentname +') ')
				canto.write('(language (language cantonese) (orthography jyutping)) ')
				canto.write('(lit '+ jyutping + '))\n')
				
				canto.write('(segment ')
				canto.write('(underlying '+ segmentname +') ')
				canto.write('(language (language cantonese) (orthography traditional)) ')
				canto.write('(lit ' + cp['glyph'] + '))\n')
	canto.close()

db = {}
readUnihanFile('Unihan_Readings.txt', db)
readUnihanFile('Unihan_Variants.txt', db)

createmandarin(db)
createcanto(db)

#print db['U+9C7C']
#print db['U+9B5A']
