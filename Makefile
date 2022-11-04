soci-test:
	mkdir bin
	g++ -o bin/soci-test src/soci-test.cpp -lsoci_core -lsoci_sqlite3