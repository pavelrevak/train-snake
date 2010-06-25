APP = vlak
SRC = vlak.c
RESPATH = \"res\"

CC = $(CROSS_COMPILE)gcc
LN = $(CROSS_COMPILE)gcc

CCFLAGS = -g -O2 -c -I. -I.. -I../include `sdl-config --cflags` -DRESPATH=$(RESPATH)
LNFLAGS = -lpthread `sdl-config --static-libs` -lSDL_gfx -lSDL_image

MAINOBJ = $(SRC:.c=.o)

CLEANFILES = *.o *.d $(APP)

all: deps $(APP)

-include $(MAINOBJ:.o=.d)

deps: $(MAINOBJ:.o=.d)

%.d: %.c
#	@echo "  DEPS  " $@ "("$<")"
	@$(CC) $(CCFLAGS) -M $< > $@

%.o: %.c
	@echo "  CC    " $@ "("$<")"
	@$(CC) $(CCFLAGS) $<

$(APP): $(MAINOBJ) *.h
	@echo "  LN    " $@ "("$(MAINOBJ) $(LIBS)")"
	@$(LN) $(LNFLAGS) -o $@ $(MAINOBJ) $(LIBS)

run:	$(APP)
	@./$(APP)

.PHONY: clean
clean:
	@echo "  RM    " $(CLEANFILES)
	@rm -f $(CLEANFILES)

