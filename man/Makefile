ASCIIDOC_OPTS = \
	-f asciidoc.conf \
	-a man_version="2.0a" \
	-a man_date="`date +%Y-%m-%d`"

bmpanel2.1: asciidoc.conf bmpanel2.txt
	a2x -d manpage -f manpage --asciidoc-opts="$(ASCIIDOC_OPTS)" bmpanel2.txt
