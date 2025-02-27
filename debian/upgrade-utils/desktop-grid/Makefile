out:
	gcc -o meta-get -I./ meta-get.c metatree.c metabuilder.c crc32.c `pkg-config --cflags --libs gio-2.0`
	gcc -o meta-set -I./ meta-set.c metatree.c metabuilder.c crc32.c `pkg-config --cflags --libs gio-2.0`

clean:
	rm meta-get
	rm meta-set
