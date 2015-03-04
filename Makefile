
all: mcp

clean:
	rm -f mcp *.o

mcp: main.o mcp_writer.o mcp_reader.o
	$(CC) -o mcp main.o mcp_writer.o mcp_reader.o

