CXXFLAGS = -Wall -pipe
LDFLAGS =
CXX_CLANG := $(shell $(CXX) --version 2>/dev/null | grep clang)
ifeq "$(CXX_CLANG)" ""
	CXXFLAGS += -O4
	ifneq "$(findstring noomp, $(MAKECMDGOALS))" "noomp"
		CXXFLAGS += -fopenmp
		LDFLAGS += -fopenmp
	endif
else
	CXXFLAGS += -O3
ifeq "$(findstring lib, $(MAKECMDGOALS))" "lib"
	CXXFLAGS += -DUSE_NS
endif
endif

HEADERS = \
src/infomap/Edge.h \
src/infomap/flowData.h \
src/infomap/flowData_traits.h \
src/infomap/FlowNetwork.h \
src/infomap/InfomapBase.h \
src/infomap/InfomapContext.h \
src/infomap/InfomapGreedy.h \
src/infomap/InfomapGreedyCommon.h \
src/infomap/InfomapGreedySpecialized.h \
src/infomap/InfomapGreedyTypeSpecialized.h \
src/infomap/MemFlowNetwork.h \
src/infomap/MemNetwork.h \
src/infomap/MultiplexNetwork.h \
src/infomap/Network.h \
src/infomap/NetworkAdapter.h \
src/infomap/Node.h \
src/infomap/NodeFactory.h \
src/infomap/TreeData.h \
src/infomap/treeIterators.h \
src/io/ClusterReader.h \
src/io/Config.h \
src/io/convert.h \
src/io/HierarchicalNetwork.h \
src/io/ProgramInterface.h \
src/io/SafeFile.h \
src/io/TreeDataWriter.h \
src/io/version.h \
src/utils/Date.h \
src/utils/FileURI.h \
src/utils/gap_iterator.h \
src/utils/infomath.h \
src/utils/Logger.h \
src/utils/MersenneTwister.h \
src/utils/Stopwatch.h \
src/utils/types.h \
src/Infomap.h \

SOURCES = \
src/infomap/FlowNetwork.cpp \
src/infomap/InfomapBase.cpp \
src/infomap/InfomapContext.cpp \
src/infomap/MemFlowNetwork.cpp \
src/infomap/MemNetwork.cpp \
src/infomap/MultiplexNetwork.cpp \
src/infomap/Network.cpp \
src/infomap/NetworkAdapter.cpp \
src/infomap/Node.cpp \
src/infomap/TreeData.cpp \
src/io/ClusterReader.cpp \
src/io/HierarchicalNetwork.cpp \
src/io/ProgramInterface.cpp \
src/io/TreeDataWriter.cpp \
src/io/version.cpp \
src/utils/FileURI.cpp \
src/utils/Logger.cpp \

TARGET = Infomap

OBJECTS := $(SOURCES:src/%.cpp=build/%.o)
INFOMAP_OBJECT = build/Infomap.o
INFORMATTER_OBJECT = build/Informatter.o

LIBDIR = build/lib
LIBTARGET = $(LIBDIR)/libInfomap.a
LIBHEADERS := $(HEADERS:src/%.h=$(LIBDIR)/include/%.h)
INFOMAP_LIB_OBJECT = build/Infomaplib.o

SWIG_FILES = $(shell find swig -name "*.i")
SWIG_HEADERS = src/Infomap.h src/infomap/Network.h src/io/HierarchicalNetwork.h
PY_BUILD_DIR = build/py
PY_HEADERS := $(HEADERS:src/%.h=$(PY_BUILD_DIR)/src/%.h)
PY_SOURCES := $(SOURCES:src/%.cpp=$(PY_BUILD_DIR)/src/%.cpp)

.PHONY: all clean noomp lib

## Rule for making the actual target
$(TARGET): $(OBJECTS) $(INFOMAP_OBJECT)
	@echo "Linking object files to target $@..."
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "-- Link finished --"

all: $(TARGET) Infomap-formatter
	@true

python: py-build Makefile
	cd $(PY_BUILD_DIR) && python setup.py build_ext --inplace
	@true

setup.py:
	cd $(PY_BUILD_DIR) && python setup.py build_ext --inplace

py-build: Makefile $(PY_HEADERS) $(PY_SOURCES)
	@mkdir -p $(PY_BUILD_DIR)
	@cp -a $(SWIG_FILES) $(PY_BUILD_DIR)/
	@cp -a swig/setup.py $(PY_BUILD_DIR)/
	swig -c++ -python -outdir $(PY_BUILD_DIR) -o $(PY_BUILD_DIR)/infomap_wrap.cpp $(PY_BUILD_DIR)/Infomap.i


Infomap-formatter: $(OBJECTS) $(INFORMATTER_OBJECT)
	@echo "Making Informatter..."
	$(CXX) $(LDFLAGS) -o $@ $^

lib: $(LIBTARGET) $(LIBHEADERS)
	@echo "Wrote static library and headers to $(LIBDIR)"

$(LIBTARGET): $(INFOMAP_LIB_OBJECT) $(OBJECTS)
	@echo "Creating static library..."
	@mkdir -p $(LIBDIR)
	ar rcs $@ $^

$(LIBDIR)/include/%.h: src/%.h
	@mkdir -p $(dir $@)
	@cp -a $^ $@

$(PY_BUILD_DIR)/src/%.h: src/%.h
	@mkdir -p $(dir $@)
	@cp -a $^ $@

$(PY_BUILD_DIR)/src/%.cpp: src/%.cpp
	@mkdir -p $(dir $@)
	@cp -a $^ $@

$(INFOMAP_LIB_OBJECT): src/Infomap.cpp $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DNO_MAIN -c $< -o $@

## Generic compilation rule for object files from cpp files
build/%.o : src/%.cpp $(HEADERS) Makefile
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

noomp: $(TARGET)
	@true

## Clean Rule
clean:
	$(RM) -r $(TARGET) Informatter build