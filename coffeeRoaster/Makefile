# coffeeRoaster - Web-controlled ClearPath-SC drum motor
#
# Build:   make
# Install: sudo make install

EXEC_NAME := coffee-roaster
INCLUDE_DIR := ../sFoundation/inc/inc-pub
LIBS := -lsFoundation20 -lpthread
CXX := g++
CXXFLAGS := -std=c++11 -I$(INCLUDE_DIR) -O3 -Wall

SRC_FILES := main.cpp motor_controller.cpp socket_server.cpp
OBJ_FILES := $(SRC_FILES:.cpp=.o)

all: $(EXEC_NAME)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(EXEC_NAME): $(OBJ_FILES)
ifneq ($(RPATH),)
	$(CXX) -L$(RPATH) -Wl,-rpath=$(RPATH) -o $(EXEC_NAME) $(OBJ_FILES) $(LIBS)
else
	$(CXX) -o $(EXEC_NAME) $(OBJ_FILES) $(LIBS)
endif

install: $(EXEC_NAME)
	@if ! grep -qP '^User=\S' coffee-roaster.service || ! grep -qP '^User=\S' web/coffee-roaster-web.service; then \
		echo "Error: Set User= in coffee-roaster.service and web/coffee-roaster-web.service before installing."; \
		exit 1; \
	fi
	install -m 755 $(EXEC_NAME) /usr/local/bin/
	install -m 644 coffee-roaster.service /etc/systemd/system/
	install -m 644 web/coffee-roaster-web.service /etc/systemd/system/
	install -d /opt/coffee-roaster/web
	cp -r web/* /opt/coffee-roaster/web/
	@USER=$$(grep -oP '^User=\K\S+' web/coffee-roaster-web.service); \
	chown -R $$USER:$$USER /opt/coffee-roaster/web/
	systemctl daemon-reload

uninstall:
	systemctl stop coffee-roaster coffee-roaster-web || true
	systemctl disable coffee-roaster coffee-roaster-web || true
	rm -f /usr/local/bin/$(EXEC_NAME)
	rm -f /etc/systemd/system/coffee-roaster.service
	rm -f /etc/systemd/system/coffee-roaster-web.service
	rm -rf /opt/coffee-roaster
	systemctl daemon-reload

.PHONY: clean
clean:
	rm -f $(OBJ_FILES)

.PHONY: real_clean
real_clean: clean
	rm -f $(EXEC_NAME)
