
all: mcp

clean:
	rm -f mcp *.o
CFLAGS=-g

mcp: main.o mcp_writer.o mcp_reader.o pthread_barrier.o pthread_barrier_ext.o log.o copyfile.o copydir.o queue.o
	$(CC) -lpthread -o mcp main.o mcp_writer.o mcp_reader.o pthread_barrier.o pthread_barrier_ext.o log.o copyfile.o copydir.o queue.o

