
pe1: pattern_exclude.cc
	gcc -L/usr/include/zypp -lzypp -lstdc++ -lm -o pe1 pattern_exclude.cc

pe2: pattern_exclude2.cc
	gcc -L/usr/include/zypp -lzypp -lstdc++ -lm -o pe2 pattern_exclude.cc

clean:
	rm -f pe1 pe2
	rm -rf tmp

.PHONY: clean
