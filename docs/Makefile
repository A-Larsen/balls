FILENAME=$(shell find . -type f -name "*.adoc")

build: $(FILENAME)
	asciidoctor -b html5 $(FILENAME)

