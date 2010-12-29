default:
	./bam -c
	./bam server_release
	./bam client_release
	./bam
build:
	./bam server_release
	./bam client_release
	./bam
