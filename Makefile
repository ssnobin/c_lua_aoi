

build/laoi.so: src/lua_aoi.c src/aoi.c src/utils.c
	mkdir -p build
	gcc -fPIC -shared -o build/laoi.so src/lua_aoi.c src/aoi.c src/utils.c