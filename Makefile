
OBJS = \
any.o array.o database.o function.o grammar.o \
import.o lexer.o main.o mapping.o object.o \
parse.o port.o string.o thread.o \
fcrypt.o fcrypt_b.o set_key.o libdes_crypt.o

LIBS = -lm

CFLAGS += -m32 -std=c++11 -Wno-c++11-extensions
IFLAGS = -Wall -c -O2
CC = g++
YACC = bison
YFLAGS = -d -v

ffe : $(OBJS)
	$(CC) $(CFLAGS) -o ffe *.o $(LIBS)
#	$(CC) $(CFLAGS) -o ffe $(OBJS) $(LIBS)

libdes_crypt.o :
	gcc -m32 $(IFLAGS) libdes_crypt.c

fcrypt.o :
	gcc -m32 $(IFLAGS) libdes-l/fcrypt.c

fcrypt_b.o :
	gcc -m32 $(IFLAGS) libdes-l/fcrypt_b.c

set_key.o :
	gcc -m32 $(IFLAGS) libdes-l/set_key.c


global.h : platform.h types.h result.h

any.h : global.h

any_impl.h : any.h string.h array.h mapping.h object.h function.h port.h thread.h

array.h : global.h any.h index.h

database.h : global.h

function.h : global.h index.h

import.h : any.h global.h index.h

index.h : global.h

mapping.h : global.h any.h index.h

object.h : global.h any.h index.h

parse.h : opcode.h

port.h : global.h index.h

string.h : global.h index.h

thread.h : global.h index.h any.h

main.o : main.cpp global.h database.h import.h port.h array.h thread.h index.h
	$(CC) $(CFLAGS) $(IFLAGS) main.cpp

any.o : any.cpp any.h database.h index.h string.h array.h mapping.h object.h function.h port.h thread.h
	$(CC) $(CFLAGS) $(IFLAGS) any.cpp

array.o : array.cpp database.h any_impl.h
	$(CC) $(CFLAGS) $(IFLAGS) array.cpp

function.o : function.cpp function.h database.h string.h array.h
	$(CC) $(CFLAGS) $(IFLAGS) function.cpp

mapping.o : mapping.cpp mapping.h database.h any_impl.h
	$(CC) $(CFLAGS) $(IFLAGS) mapping.cpp

object.o : object.cpp object.h database.h string.h index.h any_impl.h
	$(CC) $(CFLAGS) $(IFLAGS) object.cpp

port.o : port.cpp port.h database.h string.h
	$(CC) $(CFLAGS) $(IFLAGS) port.cpp

string.o : string.cpp string.h database.h
	$(CC) $(CFLAGS) $(IFLAGS) string.cpp

thread.o : thread.cpp thread.h database.h string.h array.h mapping.h object.h function.h port.h opcode.h any_impl.h
	$(CC) $(CFLAGS) $(IFLAGS) thread.cpp

database.o : database.cpp database.h any.h index.h string.h array.h mapping.h object.h function.h port.h thread.h
	$(CC) $(CFLAGS) $(IFLAGS) database.cpp

import.o : import.cpp import.h database.h string.h array.h object.h any_impl.h
	$(CC) $(CFLAGS) $(IFLAGS) import.cpp

lexer.o : lexer.cpp global.h parse.h grammar.h string.h array.h
	$(CC) $(CFLAGS) $(IFLAGS) lexer.cpp

parse.o : parse.cpp global.h parse.h grammar.h opcode.h runtime.h string.h function.h array.h any_impl.h
	$(CC) $(CFLAGS) $(IFLAGS) parse.cpp

grammar.o : grammar.cpp global.h parse.h any.h string.h
	$(CC) $(CFLAGS) $(IFLAGS) grammar.cpp

grammar.h grammar.cpp : grammar.y global.h parse.h any.h string.h
	$(YACC) $(YFLAGS) grammar.y
#	mv grammar_.c grammar.cpp
#	mv grammar_.h grammar.h
	mv grammar.tab.c grammar.cpp
	mv grammar.tab.h grammar.h

clean :
	rm *.o *~ ffe grammar.h grammar.cpp grammar.output ffe.exe grammar.out
