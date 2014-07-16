# CMD_RM CMD_MKDIR CMD_CP LIBCACHEDIR CONFIG FLAGS
AUTOINF_MAJ=1
AUTOINF_MIN=0
LIBS=core-1.0 bitset-1.0 autofrotz-1.0

LIBFLAGS::=$(foreach o,$(LIBS),-I$(LIBCACHEDIR)/$(o)/include) $(foreach o,$(LIBS),-L$(LIBCACHEDIR)/$(o)/lib-$(CONFIG)) $(foreach o,$(shell parselibs libnames $(LIBS)),-l$(o))
DEPENDENCIESFLAGS::=-DDEPENDENCIES="$(shell parselibs dependenciesdefn $(LIBS))"

autoinf: autoinf.exe


o:
	$(CMD_MKDIR) $@


AUTOINF_HDRS=libraries/autoinf.hpp libraries/autoinf.ipp libraries/autoinf.using

o/autoinf.o: libraries/autoinf.cpp $(AUTOINF_HDRS) | o
	gcc $(FLAGS) $(LIBFLAGS) $(DEPENDENCIESFLAGS) -DLIB_MAJ=$(AUTOINF_MAJ) -DLIB_MIN=$(AUTOINF_MIN) -x c++ -c $< -o $@

AUTOINF_OBJS=o/autoinf.o


MAIN_HDRS=header.hpp $(AUTOINF_HDRS)

o/%.o: %.cpp $(MAIN_HDRS) | o
	gcc $(FLAGS) $(LIBFLAGS) -x c++ -c $< -o $@

MAIN_OBJS=$(patsubst %.cpp,o/%.o,$(wildcard *.cpp))


LIBCACHEOUTDIR=$(LIBCACHEDIR)/autoinf-$(AUTOINF_MAJ).$(AUTOINF_MIN)
autoinf.exe: $(MAIN_OBJS) $(AUTOINF_OBJS)
	gcc $(FLAGS) $^ -o $@ $(LIBFLAGS) -lstdc++
	$(CMD_RM) $(LIBCACHEOUTDIR)
	$(CMD_MKDIR) $(LIBCACHEOUTDIR)/lib-$(CONFIG)
	ar -rcsv $(LIBCACHEOUTDIR)/lib-$(CONFIG)/libautoinf.a $(AUTOINF_OBJS)
	$(CMD_MKDIR) $(LIBCACHEOUTDIR)/include
	$(CMD_CP) --target $(LIBCACHEOUTDIR)/include $(AUTOINF_HDRS)
