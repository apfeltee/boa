
SHELL = /bin/zsh -G

INCFLAGS =

unflags = \
	-Wunused \
	-Wunused-but-set-parameter \
	-Wunused-but-set-variable \
	-Wunused-const-variable \
	-Wunused-const-variable \
	-Wno-unused-function \
	-Wunused-label \
	-Wunused-local-typedefs \
	-Wunused-macros \
	-Wunused-parameter \
	-Wunused-result \
	-Wunused-value \
	-Wunused-variable

unflags += -Wvla
## has a tendency to report false positives!
#unflags += -pedantic


allunflags = $(unflags)
#allunflags = $(unflags) 

WFLAGS = -Wall -Wextra -Wshadow -Wpointer-arith -Wuninitialized -Winit-self  $(allunflags)
WFLAGS += -Wmaybe-uninitialized  -Wnull-dereference -Wduplicated-cond
WFLAGS +=  -Wwrite-strings

## very verbose
#WFLAGS += -Wconversion

## will complain about computed gotos
#WFLAGS += -pedantic

## also very verbose
#WFLAGS += -Weffc++

#WFLAGS = -w


OPTFLAGS = -Og
#OPTFLAGS = -O5 -funroll-loops -flto -ffast-math 


## empty by default
EXTRAFLAGS =
## might be needed for -fsanitize* to work properly, but that's just a theory.
#EXTRAFLAGS =  -mno-red-zone -fno-omit-frame-pointer

#EXTRAFLAGS += -fno-stack-protector

#*** WARNING ***
# causes quite a steep use of memory!
#EXTRAFLAGS += -fsanitize=undefined
#EXTRAFLAGS += -fsanitize=memory
#EXTRAFLAGS += -fsanitize=address
#EXTRAFLAGS += -fstack-protector-all -ftrapv
#export COLLECT_NO_DEMANGLE=0

### WARNING: can be quite verbose! prints unused sections, giving a better clue which functions can be removed.
#EXTRAFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,--print-gc-sections

CFLAGS =
#CFLAGS += $(INCFLAGS) -Ofast -march=native -flto -ffast-math -funroll-loops
CFLAGS += $(INCFLAGS) $(OPTFLAGS) -g3 -ggdb3
#CFLAGS += -DNEON_INLINE="__attribute__((hot, optimize(5)))"

CC = gcc $(WFLAGS) $(EXTRAFLAGS)
#CC = clang -gdwarf-4 -ferror-limit=1000 $(WFLAGS) $(EXTRAFLAGS)

CC = gcc $(WFLAGS) $(EXTRAFLAGS)
#CC = tcc $(WFLAGS) $(EXTRAFLAGS)
DEPCC = gcc

LDFLAGS = -ldl -lm -lreadline
target = run

srcfiles_all = $(wildcard *.c)
headerfiles_all = $(wildcard *.h)

objfiles_all = $(srcfiles_all:.c=.o)
depfiles_all = $(objfiles_all:.o=.d)
protofile = prot.inc

# janky mess
havecproto = 1
ifeq (, $(shell which cproto))
havecproto = 0
endif

.PHONY: all
all: $(protofile) $(target)

## dear god what a kludge
ifeq (1, $(havecproto))
$(protofile): $(srcfiles_all) $(headerfiles_all)
	echo > $(protofile)
	cproto $(srcfiles_all) $(headerfiles_all) 2>/dev/null | perl -pe 's/\b_Bool\b/bool/g' | grep -vP __inline > $(protofile)_tmp
	mv $(protofile)_tmp $(protofile)
else
$(protofile): $(srcfiles_all)
endif

$(target): $(objfiles_all)
	$(CC) -o $(target) $(objfiles_all) $(LDFLAGS)

-include $(depfiles_all)

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.c
	$(DEPCC) $(CFLAGS) $< -MM -MT $(@:.d=.o) -MF $@

%.o: %.c
	$(CC) $(CFLAGS) -c $(DBGFLAGS) -o $@ $<


.PHONY: clean
clean:
	rm -f $(objfiles_all) $(target) *.exe *.ilk *.obj *.pdb

.PHONY: cleandep
cleandep:
	rm -f $(depfiles_all)

.PHONY: distclean
distclean: cleandep clean

.PHONY: rebuild
rebuild: clean cleandep $(target)

