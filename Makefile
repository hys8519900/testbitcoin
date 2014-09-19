LDFLAGS = -DHAVE_WORKING_BOOST_SLEEP -lbitcoin_server -lbitcoin_cli -lbitcoin_wallet -L. -I./leveldb/include/ -L./leveldb -lleveldb -lmemenv -lbitcoin_util -lbitcoin_common -lbitcoin_crypto  -lboost_system -lboost_program_options -lboost_filesystem -lboost_thread -lcrypto  -L./crypto  -pthread  -lanl -ldb_cxx-4.8 -lssl
	
OBJDIR = bin
SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp, $(OBJDIR)/%, $(SRC))

all : $(OBJ)

$(OBJDIR)/% : %.cpp
	g++ $< -o $@ $(LDFLAGS)

runall : $(OBJ)

$(OBJDIR)/% : %.cpp
	g++ $< -o $@ $(LDFLAGS)
	./$@

tmpgdb : m_LogPrint.cpp
	g++ $< -o bin/$@ $(LDFLAGS) -g -O0
