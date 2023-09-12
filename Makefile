CXX = g++
CXXFLAGS = -g -Wall -fPIC
ROOTFLAGS = `root-config --cflags --glibs`

BINARIES = ReadNT  
TARGETS = ReadNT.o 

#--- if using 'make all' ---#
all : $(TARGETS)

#--- if making individual targets ---#

ReadNT.o : ReadNT.cxx
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(ROOTFLAGS) -o $*.o $(LDLIBS) -c $*.cxx #compile
	$(CXX) $(INCLUDE) $(CXXFLAGS) $(ROOTFLAGS) $(LDLIBS) -o $* $*.o        #link

clean:
	rm -f $(BINARIES) $(TARGETS)

