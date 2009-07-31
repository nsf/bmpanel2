SET(ASCIIDOC_OPTS
	-f ${CMAKE_CURRENT_SOURCE_DIR}/asciidoc.conf
	-a man_version="2.0a"
)

SET(A2X_OPTS
	-D ${CMAKE_CURRENT_BINARY_DIR} -d manpage -f manpage
)

SET(MAN_SOURCE
	${CMAKE_CURRENT_SOURCE_DIR}/bmpanel2.txt
)

ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bmpanel2.1
	COMMAND a2x ${A2X_OPTS} --asciidoc-opts="${ASCIIDOC_OPTS}" ${MAN_SOURCE}
	DEPENDS asciidoc.conf bmpanel2.txt
)
ADD_CUSTOM_TARGET(manpage ALL DEPENDS bmpanel2.1)
