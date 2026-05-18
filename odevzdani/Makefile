# Root Makefile for Petri Net project
# Authors: Lukáš Dudek (xdudekl00), Jan Raška (xraskaj00), Matěj Zaoral (xzaoram00)

.PHONY: all clean run test pack doxygen

all:
	$(MAKE) -C src all

clean:
	$(MAKE) -C src clean
	rm -rf doc/html doc/latex

run:
	$(MAKE) -C src run

test:
	$(MAKE) -C src test

pack: clean
	zip -r xraskaj00-xdudekl00-xzaoram00.zip src examples doc README.txt Makefile Doxyfile .gitignore

doxygen:
	doxygen Doxyfile 2>/dev/null || echo "Doxyfile not found - skipping"