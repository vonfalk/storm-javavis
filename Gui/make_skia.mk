# Defines found in BUILD.gn
# SK_R32_SHIFT=16 seems to be assumed on Linux and Fuschsia
# SK_ASSUME_GL_ES is to assume GLES and not GL (SK_ASSUME_GL=1)
# SK_GAMMA_APPLY_TO_A8 seems to be set for "skia_private" config.
# GR_OP_ALLOCATE_USE_NEW seems to be enable if we have gpu support
# SKIA_IMPLEMENTATION=1 seems to be needed when building the library
# SK_UNICODE_AVAILABLE=1 for unicode support in text shaping modules
# SK_SHAPER_HARFBUZZ_AVAILABLE=1 for harfbuzz support
DEFINES := SK_R32_SHIFT=16 SK_ASSUME_GL_ES=1 SK_GAMMA_APPLY_TO_A8 GR_OP_ALLOCATE_USE_NEW SKIA_IMPLEMENTATION=1 SK_SUPPORT_GPU=1 SK_GL=1 SK_UNICODE_AVAILABLE=1 SK_SHAPER_HARFBUZZ_AVAILABLE=1

# There seems to be a number of options for enabling SSE and AVX. I don't think that is needed since
# we will be using it through OpenGL.

# Note: The below issue is fixed now, and therefore we can use -O3 freely again. This issue broke Skia even on -O2.
# Note: Using -O3 with GCC seems to break the initialization of the ComponentArray passed to Swizzle
# in SkSLIRGenerator.cpp, in the function getNormalizeSkPositionCode. Don't know if this is UB in
# the Skia code or a micompilation by GCC. Should probably be reported somehow...

# The -Wno-psabi flag is to silence a warning about ABI compatibility since we don't explicitly enable AVX at the time.

# The cheap flags are used to build the standalone SKSL compiler to reduce compile times a bit.
CHEAP_CXXFLAGS := -std=c++17 -fPIC -iquote. -Wno-psabi -I/usr/include/freetype2 $(addprefix -D,$(DEFINES))
# These are the flags for the final library. We want to use -O3 here for speed.
CXXFLAGS := -O3 $(CHEAP_CXXFLAGS)
# Note: We skipped these: codec
SKIA_LIBS := skshaper skparagraph
SOURCE_DIRS := effects effects/imagefilters gpu gpu/text gpu/ccpr gpu/effects gpu/effects/generated gpu/geometry gpu/glsl gpu/gl gpu/gl/builders gpu/mock gpu/gl/egl gpu/gl/glx gpu/ops gpu/tessellate gpu/gradients gpu/gradients/generated images opts sfnt utils c core fonts image lazy pathops shaders shaders/gradients sksl sksl/ir
PORTS := SkDebug_stdio.cpp SkDiscardableMemory_none.cpp SkFontConfigInterface*.cpp SkFontMgr_fontconfig*.cpp SkFontMgr_FontConfigInterface*.cpp SkFontHost_*.cpp SkGlobalInitialization_default.cpp SkMemory_malloc.cpp SkOSFile_posix.cpp SkOSFile_stdio.cpp SkOSLibrary_posix.cpp SkImageGenerator_none.cpp
CODEC := SkMasks.cpp
SOURCES := $(wildcard $(addsuffix /*.cpp,$(addprefix src/,$(SOURCE_DIRS)))) $(wildcard $(addprefix src/ports/,$(PORTS))) $(wildcard $(addprefix src/codec/,$(CODEC)))
SOURCES := $(filter-out src/sksl/SkSLMain.cpp,$(SOURCES)) # Remove the main file...
SOURCES := $(filter-out src/gpu/gl/GrGLMakeNativeInterface_none.cpp,$(SOURCES))
LIB_SOURCES := $(wildcard $(addsuffix /src/*.cpp,$(addprefix modules/,$(SKIA_LIBS))))
LIB_SOURCES := $(filter-out %_coretext.cpp,$(LIB_SOURCES))
OBJECTS := $(patsubst src/%.cpp,out/%.o,$(SOURCES))
LIB_OBJECTS := $(patsubst modules/%.cpp,out/modules/%.o,$(LIB_SOURCES))

SKSL_SRC := src/core/SkMalloc.cpp src/core/SkMath.cpp src/core/SkSemaphore.cpp src/core/SkThreadID.cpp src/gpu/GrBlockAllocator.cpp src/gpu/GrMemoryPool.cpp src/ports/SkMemory_malloc.cpp $(wildcard src/sksl/*.cpp src/sksl/ir/*.cpp)
SKSL_OBJ := $(patsubst src/%.cpp,out/slc/%.o,$(SKSL_SRC))
SKSL_PRECOMP := $(addsuffix .sksl,$(addprefix src/sksl/sksl_,fp frag geom gpu interp pipeline vert))
SKSL_DEHYDRATED := $(patsubst src/sksl/%.sksl,src/sksl/generated/%.dehydrated.sksl,$(SKSL_PRECOMP))

ifeq ($(OUTPUT),)
OUTPUT := skia.a
endif

all: $(OUTPUT)

.PHONY: clean
clean:
	rm -r out/

$(OUTPUT): $(OBJECTS) $(LIB_OBJECTS) out/skcms.o
	@echo "Linking $(OUTPUT)..."
	@ar rcs $(OUTPUT) $(OBJECTS) out/skcms.o

$(OBJECTS): out/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	g++ -c $(CXXFLAGS) -o $@ $<

$(LIB_OBJECTS): out/modules/%.o: modules/%.cpp
	@mkdir -p $(dir $@)
	g++ -c $(CXXFLAGS) -I/usr/include/harfbuzz/ -o $@ $<

out/sksl/SkSLCompiler.o: src/sksl/SkSLCompiler.cpp $(SKSL_DEHYDRATED)

out/skcms.o: third_party/skcms/skcms.cc
	gcc -c -I include/third_party/skcms -O3 -fPIC $< -o $@

out/skslc: $(SKSL_OBJ)
	g++ -pthread -o $@ $(SKSL_OBJ)

$(SKSL_OBJ): out/slc/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	g++ -c $(CHEAP_CXXFLAGS) -DSKSL_STANDALONE -o $@ $<

src/sksl/sksl_fp.sksl: src/sksl/sksl_fp_raw.sksl
	cat include/private/GrSharedEnums.h $^ | sed 's|^#|// #|g' > $@

$(SKSL_DEHYDRATED): src/sksl/generated/%.dehydrated.sksl: src/sksl/%.sksl out/skslc
	cd src/sksl/ && ../../out/skslc $(notdir $<) generated/$(notdir $@)

print:
	@echo $(SOURCES)
	@echo $(SKSL_DEHYDRATED)
