# CMD_RM CMD_MKDIR CMD_CP LIBCACHEDIR PLATFORM CONFIG FLAGS
AUTOINF_MAJ=1
AUTOINF_MIN=0
LIBS=core-1.0 autofrotz-1.0

PLATFORMFLAGS::=-D$(PLATFORM)
LIBFLAGS::=$(foreach o,$(LIBS),-I$(LIBCACHEDIR)/$(o)/include) $(foreach o,$(LIBS),-L$(LIBCACHEDIR)/$(o)/lib-$(CONFIG)) $(foreach o,$(shell parselibs libnames $(LIBS)),-l$(o))
DEPENDENCIESFLAGS::=-DDEPENDENCIES="$(shell parselibs dependenciesdefn $(LIBS))"

autoinf: autoinf.exe


o:
	$(CMD_MKDIR) $@


AUTOINF_HDRS=libraries/autoinf.hpp libraries/autoinf.ipp libraries/autoinf.using

o/autoinf.o: libraries/autoinf.cpp $(AUTOINF_HDRS) | o
	gcc $(PLATFORMFLAGS) $(LIBFLAGS) $(DEPENDENCIESFLAGS) -DLIB_MAJ=$(AUTOINF_MAJ) -DLIB_MIN=$(AUTOINF_MIN) $(FLAGS) -x c++ -c $< -o $@

AUTOINF_OBJS=o/autoinf.o


MAIN_HDRS=header.hpp $(AUTOINF_HDRS)

o/%.o: %.cpp $(MAIN_HDRS) | o
	gcc $(PLATFORMFLAGS) $(LIBFLAGS) $(FLAGS) -x c++ -c $< -o $@

MAIN_OBJS=$(patsubst %.cpp,o/%.o,$(wildcard *.cpp))


LIBCACHEOUTDIR=$(LIBCACHEDIR)/autoinf-$(AUTOINF_MAJ).$(AUTOINF_MIN)
autoinf.exe: $(MAIN_OBJS) $(AUTOINF_OBJS)
	gcc $(PLATFORMFLAGS) $(FLAGS) $^ -o $@ $(LIBFLAGS) -lstdc++
	$(CMD_RM) $(LIBCACHEOUTDIR)
	$(CMD_MKDIR) $(LIBCACHEOUTDIR)/lib-$(CONFIG)
	ar -rcsv $(LIBCACHEOUTDIR)/lib-$(CONFIG)/libautoinf.a $(AUTOINF_OBJS)
	$(CMD_MKDIR) $(LIBCACHEOUTDIR)/include
	$(CMD_CP) --target $(LIBCACHEOUTDIR)/include $(AUTOINF_HDRS)
