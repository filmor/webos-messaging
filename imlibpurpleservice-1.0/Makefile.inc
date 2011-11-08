LIBS := -llunaservice  -lmojoluna -lmojodb -lmojocore -lpurple -lsanitize
LOCAL_LDFLAGS := $(LDFLAGS) $(LIBS)

IM_SOURCES := \
	BuddyListConsolidator.cpp \
	IMMessage.cpp \
	IMServiceHandler.cpp \
	IMServiceApp.cpp \
	LibpurpleAdapter.cpp \
	IMLoginState.cpp \
	IncomingIMHandler.cpp \
	OutgoingIMHandler.cpp \
	ConnectionStateHandler.cpp \
	DisplayController.cpp \
	SendOneMessageHandler.cpp \
	OutgoingIMCommandHandler.cpp \
	SendOneCommandHandler.cpp \
	OnEnabledHandler.cpp \
	BuddyStatusHandler.cpp
	
SOURCE_DIRS := src

IM_TARGET := $(OBJDIR)/imlibpurpletransport
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
	rm -rf doc/html
	
.PHONY: clean all setup docs
