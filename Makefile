all: parta partb

run: all
	@echo executing problem a
	@echo -------------------
	#@exec ./parta
	@echo
	@echo executing problem b
	@echo -------------------
	@exec ./partb

parta: parta.cpp
	gcc parta.cpp -lstdc++ -lm -o parta

partb: partb.cpp
	gcc partb.cpp -lstdc++ -lm -o partb

clean:
	rm parta partb

.PHONY: clean run all
