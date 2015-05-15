
all: mcp

clean:
	rm -f mcp *.o

mcp: main.o mcp_writer.o mcp_reader.o pthread_barrier.o pthread_barrier_ext.o
	$(CC) -lpthread -o mcp main.o mcp_writer.o mcp_reader.o pthread_barrier.o pthread_barrier_ext.o

