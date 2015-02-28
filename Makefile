
all: mcp

clean:
	rm -f mcp *.o

mcp: main.o mcp_writer.o
	$(CC) -o mcp main.o mcp_writer.o

