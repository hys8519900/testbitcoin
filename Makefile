LDFLAGS = -DHAVE_WORKING_BOOST_SLEEP -lbitcoin_server -lbitcoin_cli -lbitcoin_wallet -L. -I./leveldb/include/ -L./leveldb -lleveldb -lmemenv -lbitcoin_util -lbitcoin_common -lbitcoin_crypto  -lboost_system -lboost_program_options -lboost_filesystem -lboost_thread -lcrypto  -L./crypto  -pthread  -lanl -ldb_cxx-4.8 -lssl
	
SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp, %, $(SRC))

all : $(OBJ)

$(OBJ) : % : %.cpp
	g++ $< -o bin/$@ $(LDFLAGS)

m_loadblockchain_gdb : m_loadblockchain.cpp
	g++ $< -o bin/$@ $(LDFLAGS) -g -O0
