CC=g++
HEADERS_COMMON=../q4sCommon/ConfigFile.h ../q4sCommon/Q4SCommonProtocol.h ../q4sCommon/Q4SMessageManager.h ../q4sCommon/Q4SMessageInfo.h ../q4sCommon/Q4SCriticalSection.h ../q4sCommon/Q4SMessageTools.h ../q4sCommon/Q4SSocket.h ../q4sCommon/Q4SMathUtils.h ../q4sCommon/Q4SStructs.h ../q4sCommon/Q4SMessage.h ../q4sCommon/Q4SSDP.h ../q4sCommon/Q4SSDPParams.h
HEADERS_CLIENT=Q4SClientConfigFile.h Q4SClientProtocol.h  Q4SClientSocket.h  Q4SClientStateManager.h Q4SClientState.h
FILE_COMMON= ../q4sCommon/ConfigFile.cpp ../q4sCommon/Q4SCommonProtocol.cpp ../q4sCommon/Q4SMessageManager.cpp ../q4sCommon/Q4SCriticalSection.cpp ../q4sCommon/Q4SSocket.cpp ../q4sCommon/Q4SMathUtils.cpp ../q4sCommon/Q4SMessage.cpp ../q4sCommon/Q4SSDP.cpp ../q4sCommon/Q4SStructs.cpp ../q4sCommon/Q4SSDPParams.cpp ../q4sCommon/Q4SMessageTools.cpp
FILE_CLIENT= Q4SClientConfigFile.cpp Q4SClient.cpp  Q4SClientProtocol.cpp Q4SClientSocket.cpp Q4SClientStateManager.cpp
DEBUG_FLAGS= -DDEBUG
OBJ=Q4SClient
FLAGS= -pthread
all:
	$(CC) -O3 $(HEADERS_COMMON) $(HEADERS_CLIENT) $(FILE_COMMON) $(FILE_CLIENT) $(DEBUG_FLAGS) $(FLAGS) -o $(OBJ)
clean:
	rm -rf $(OBJ) *~* 
