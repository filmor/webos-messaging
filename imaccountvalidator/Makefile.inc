LIBS := -llunaservice  -lmojoluna -lmojocore -lpurple  
LOCAL_LDFLAGS := $(LDFLAGS) $(LIBS)

IM_SOURCES := \
	IMAccountValidatorHandler.cpp \
	IMAccountValidatorApp.cpp 
	
	
SOURCE_DIRS := src

IM_TARGET := $(OBJDIR)/imaccountvalidator
IM_OBJECTS := $(IM_SOURCES:%.cpp=$(OBJDIR)/%.o)

all: setup $(IM_TARGET)
		
$(IM_TARGET): $(IM_OBJECTS) 
	$(CXX) -o $(IM_TARGET) $(IM_OBJECTS) $(LOCAL_LDFLAGS) 
	
$(OBJDIR)/%.o: %.cpp
	$(CXX) -MMD $(INCLUDES) $(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) -c $< -o $@

setup: $(OBJDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

vpath %.cpp $(SOURCE_DIRS)

-include $(IM_SOURCES:%.cpp=$(OBJDIR)/%.d)
	
clean:
	rm -rf $(OBJDIR)
		
.PHONY: clean all setup docs
