CXXFLAGS = -g -std=c++11 -Wall -Wextra -Werror
LIBS = -lmega

.PHONY: clean

megaproxy: megaproxy.cpp
	g++ $(CXXFLAGS) $(LIBS) -o $@ $^

clean:
	rm -f megaproxy
