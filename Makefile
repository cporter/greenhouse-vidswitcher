
CXX = clang++
CXXFLAGS = -Wempty-body \
  -fdiagnostics-show-option \
  -Wall \
  -Wpointer-arith \
  -Wshadow \
  -Wwrite-strings \
  -Wno-overloaded-virtual \
  -Wno-strict-overflow \
  -Wno-error=unused-variable \
  -Werror \
  -g -O3 -std=gnu++11

%: %.C
	${CXX} `PKG_CONFIG_PATH=$(PKG_CONFIG_PATH):/opt/oblong/Greenhouse/lib/pkgconfig /opt/oblong/deps-64-8/bin/pkg-config --libs --cflags --static libGreenhouse` ${CXXFLAGS} $^ -o $@

all: vids

.PHONY: clean

clean:
	rm -f vids
