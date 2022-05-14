client-phase: client-phase1.cpp client-phase2.cpp client-phase3.cpp client-phase4.cpp client-phase5.cpp
	g++ -pthread -std=c++17 -o client-phase1 client-phase1.cpp -lssl -lcrypto
	g++ -pthread -std=c++17 -o client-phase2 client-phase2.cpp -lssl -lcrypto
	g++ -pthread -std=c++17 -o client-phase3 client-phase3.cpp -lssl -lcrypto
	g++ -pthread -std=c++17 -o client-phase4 client-phase4.cpp -lssl -lcrypto
	g++ -pthread -std=c++17 -o client-phase5 client-phase5.cpp -lssl -lcrypto
