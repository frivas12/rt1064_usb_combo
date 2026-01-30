LATEXBUILDER=latexmk
LATEXBUILDERARGS=-pdf -output-directory=build/ -synctex=1 -interaction=nonstopmode -file-line-error

TOP=src/README.tex
COMPILED=build/README.pdf
TEX_FILES=$(wildcard src/*.tex)

$(COMPILED): $(TEX_FILES)
	$(LATEXBUILDER) $(LATEXBUILDERARGS) $(TOP)

.PHONY : clean
clean:
	rm -f build/*