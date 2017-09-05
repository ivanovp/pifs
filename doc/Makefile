#Copyright (C) by Peter Ivanov, 2000-2017
#Makefile
#Usage:
# make
#  Generate dvi and ps files
# make pdf
#  Generate dvi, ps (PostScript) and pdf (Adobe Portable Document Format)
# make html
#  Generate HTML
# make clean
#  Remove all generated files

SOURCE = pifs
VER = 0.1
AUTHOR = Peter Ivanov
EMAIL = ivanovp@gmail.com
# Table of contents or longtables (yes, no)
TOC = yes
# Directory of HTML doc
HTML_DIR = html
## Postscript file options ##
# Papersize (letter, legal, ledger, a3, a4, a5)
PAPER = a4
ADVANCED_PDF = no

.PHONY: all
all:	pdf

.PHONY: ps
ps:	$(SOURCE).ps

$(SOURCE).ps:	$(SOURCE).dvi

$(SOURCE).dvi:	*.tex

.PHONY: pdf
pdf:	$(SOURCE).pdf

ifeq ($(ADVANCED_PDF), yes)
$(SOURCE).pdf:	$(SOURCE)_pdf.pdf
	cp $(SOURCE)_pdf.pdf $(SOURCE).pdf

$(SOURCE)_pdf.pdf:	$(SOURCE)_pdf.ps

$(SOURCE)_pdf.ps:	$(SOURCE)_pdf.dvi

$(SOURCE)_pdf.dvi:	$(SOURCE)_pdf.tex

$(SOURCE)_pdf.tex:	$(SOURCE).tex
	cp $< $@
# Removes the comment sign
	perl -p -i -e "s/%.PDF.//;s/^(.*)%.PS./% PDF $1/" $@
#	perl -p -i -e "s/%.PDF.//" $@
else
#$(SOURCE).pdf:	$(SOURCE).ps
$(SOURCE).pdf:	$(SOURCE).tex
endif

# Compressed 4 -> 1
.PHONY: 4in1
4in1:	$(SOURCE)_4in1.ps

$(SOURCE)_4in1.ps:	$(SOURCE).ps
	mpage -4 -dp -A -M-20 -m10lt28r45b $(SOURCE).ps > $(SOURCE)_4in1.ps

$(SOURCE)_4in1.pdf:	$(SOURCE)_4in1.ps
	ps2pdf $< $@

# Compressed 2 -> 1
.PHONY: 2in1
2in1:	$(SOURCE)_2in1.ps

$(SOURCE)_2in1.ps:	$(SOURCE).ps
	mpage -2 -dp -A -M-20 -m10lt28r45b $(SOURCE).ps > $(SOURCE)_2in1.ps

$(SOURCE)_2in1.pdf:	$(SOURCE)_2in1.ps
	ps2pdf $< $@

.PHONY: dist
dist:	dist/$(SOURCE)$(VER).zip dist/$(SOURCE)-$(VER).ps.gz

dist/$(SOURCE)$(VER).zip: $(SOURCE).pdf 
	-mkdir dist
	zip $@ $?

dist/$(SOURCE)-$(VER).ps.gz: $(SOURCE).ps 
	-mkdir dist
	gzip -c $< > $@

%.dvi:	%.tex
	@echo " * Creating DVI file..."
	latex $<
ifeq ($(TOC), yes)
	latex $<
endif

%.ps:	%.dvi
	@echo " * Creating PS (PostScript) file..."
	dvips -Z -D 600 -f -t$(PAPER) -o $@ $<

#%.pdf:	%.ps
#	@echo " * Creating PDF (Portable Document Format)..."
#	ps2pdf $< $@

%.pdf:	%.tex
	@echo " * Creating PDF (Portable Document Format)..."
	pdflatex $<

.PHONY: html
html:	$(SOURCE).tex
	@echo " * Creating HTML files..."
	mkdir -v $(HTML_DIR)
	latex2html -iso_language 'HU' \
	-address '$(AUTHOR)<br> \
	E-Mail: <a href="mailto:$(EMAIL)"> \
	$(EMAIL)</a>' \
	-split +1 -dir $(HTML_DIR) -html_version 3.2 -local_icons $<

.PHONY: clean
clean:	cleanall
#	-rm -rvf $(SOURCE).aux $(SOURCE).log $(SOURCE).toc

.PHONY: cleanall
cleanall:
	@echo " * Cleaning..."
	@rm -rvf $(SOURCE).dvi $(SOURCE).ps $(SOURCE)_2in1.ps $(SOURCE)_4in1.ps \
	$(SOURCE).lof $(SOURCE).lot \
	$(SOURCE)_2in1.pdf $(SOURCE)_4in1.pdf \
	$(SOURCE)_pdf.dvi $(SOURCE)_pdf.ps \
	$(SOURCE)_pdf.pdf $(SOURCE)_pdf.aux \
	$(SOURCE)_pdf.log $(SOURCE)_pdf.toc $(SOURCE)_pdf.tex\
	$(SOURCE)_pdf.lof $(SOURCE)_pdf.lot \
	$(SOURCE).pdf *.aux \
	$(SOURCE).log $(SOURCE).toc $(HTML_DIR) *.pyc *~ dist

.PHONY: doxygen
doxygen:
	@echo " * Running doxygen..."
	doxygen doxygen.cfg
	@echo " * Running TeX..."
	cd doxygen/latex && $(MAKE)
	cp doxygen/latex/refman.pdf .

