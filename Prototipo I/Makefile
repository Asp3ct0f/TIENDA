# COMPILADOR
CC = gcc

# BANDERAS
CFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter

# LIBRERIAS
LIBS = -lpthread -lncurses

# ARCHIVOS
CLIENTE = Cliente.c cifrado.c
SERVIDOR = Servidor.c cifrado.c

# COMPILAR TODO
all: Cliente Servidor

# CLIENTE
Cliente:
	$(CC) $(CLIENTE) $(CFLAGS) -o Cliente $(LIBS)

# SERVIDOR
Servidor:
	$(CC) $(SERVIDOR) $(CFLAGS) -o Servidor $(LIBS)

# LIMPIAR
clean:
	rm -f Cliente Servidor
