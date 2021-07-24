CCX86=gcc -Wall 
CPPX86=g++ -Wall
CCARMHF=arm-linux-gnueabihf-gcc -Wall
CPPARMHF=arm-linux-gnueabihf-g++ -Wall
CCARMEL=arm-linux-gnueabi-gcc -DAVOID_THREAD_NAMES -Wall
CPPARMEL=arm-linux-gnueabi-g++ -Wall

INSTALL_INCLUDE_DIR=/home/imx6/Desktop/ElettrolaserDEPS/include/gblib/
INSTALL_LIBRARY_DIR_X86=/home/imx6/Desktop/ElettrolaserDEPS/x86/lib/
INSTALL_LIBRARY_DIR_ARMHF=/home/imx6/Desktop/ElettrolaserDEPS/armhf/lib/
INSTALL_LIBRARY_DIR_ARMEL=/home/imx6/Desktop/ElettrolaserDEPS/armhf/lib/

clean:
	rm -rf obj/*
	rm -rf library/*

##################################################################
# X86
##################################################################
gb_utility_x86: src/gb_utility.c h/gb_utility.h
	$(CCX86) -c -g src/gb_utility.c -o obj/gb_utility.o
	$(CCX86) -c -g -fPIC src/gb_utility.c -o obj/gb_utility_so.o

gb_serialapi_x86: src/gb_serialapi.c
	$(CCX86) -c -g src/gb_serialapi.c -o obj/gb_serialapi.o
	$(CCX86) -c -g -fPIC src/gb_serialapi.c -o obj/gb_serialapi_so.o

gb_serialmanager_x86: src/gb_serialmanager.c
	$(CCX86) -c -g src/gb_serialmanager.c -o obj/gb_serialmanager.o
	$(CCX86) -c -g -fPIC src/gb_serialmanager.c -o obj/gb_serialmanager_so.o

gb_tcpmanager_x86: src/gb_tcpmanager.c
	$(CCX86) -c -g src/gb_tcpmanager.c -o obj/gb_tcpmanager.o
	$(CCX86) -c -g -fPIC src/gb_tcpmanager.c -o obj/gb_tcpmanager_so.o

gb_udpmanager_x86: src/gb_udpmanager.c
	$(CCX86) -c -g src/gb_udpmanager.c -o obj/gb_udpmanager.o
	$(CCX86) -c -g -fPIC src/gb_udpmanager.c -o obj/gb_udpmanager_so.o

gb_crypto_x86: src/gb_crypto.c
	$(CCX86) -c -g src/gb_crypto.c -o obj/gb_crypto.o
	$(CCX86) -c -g -fPIC src/gb_crypto.c -o obj/gb_crypto_so.o

gblib_x86: gb_utility_x86 gb_serialapi_x86 gb_serialmanager_x86 gb_tcpmanager_x86 gb_udpmanager_x86 gb_crypto_x86
	touch library/fakefile
	rm library/*
	ar r library/libgb.a obj/gb_utility.o obj/gb_serialapi.o obj/gb_serialmanager.o obj/gb_tcpmanager.o obj/gb_udpmanager.o obj/gb_crypto.o
	$(CCX86) -shared -o library/libgb.so obj/gb_utility_so.o obj/gb_serialapi_so.o obj/gb_serialmanager_so.o obj/gb_tcpmanager_so.o obj/gb_udpmanager_so.o obj/gb_crypto_so.o

test_x86:
	$(CCX86) test/serial2tcp.c library/libgb.a -lpthread -o test/serial2tcp.out 
	$(CCX86) test/tcp_client.c library/libgb.a -lpthread -o test/tcp_client.out
	$(CCX86) test/udp_listener.c library/libgb.a -lpthread -o test/udp_listener.out
	$(CCX86) test/core.c library/libgb.a -lpthread -o test/core.out
	$(CCX86) test/test_gbcrypto.c library/libgb.a -lpthread -o test/test_gbcrypto.out 
	$(CCX86) test/websockets.c library/libgb.a -lpthread -o test/websockets.out 

gb_install_x86:
	cp library/libgb.a $(INSTALL_LIBRARY_DIR_X86)
	cp library/libgb.so $(INSTALL_LIBRARY_DIR_X86)
	cp h/* $(INSTALL_INCLUDE_DIR)
##################################################################

##################################################################
# ARMHF
##################################################################
gb_utility_armhf: src/gb_utility.c h/gb_utility.h
	$(CCARMHF) -c -g src/gb_utility.c -o obj/gb_utility.o
	$(CCARMHF) -c -g -fPIC src/gb_utility.c -o obj/gb_utility_so.o

gb_serialapi_armhf: src/gb_serialapi.c
	$(CCARMHF) -c -g src/gb_serialapi.c -o obj/gb_serialapi.o
	$(CCARMHF) -c -g -fPIC src/gb_serialapi.c -o obj/gb_serialapi_so.o

gb_serialmanager_armhf: src/gb_serialmanager.c
	$(CCARMHF) -c -g src/gb_serialmanager.c -o obj/gb_serialmanager.o
	$(CCARMHF) -c -g -fPIC src/gb_serialmanager.c -o obj/gb_serialmanager_so.o

gb_tcpmanager_armhf: src/gb_tcpmanager.c
	$(CCARMHF) -c -g src/gb_tcpmanager.c -o obj/gb_tcpmanager.o
	$(CCARMHF) -c -g -fPIC src/gb_tcpmanager.c -o obj/gb_tcpmanager_so.o

gb_udpmanager_armhf: src/gb_udpmanager.c
	$(CCARMHF) -c -g src/gb_udpmanager.c -o obj/gb_udpmanager.o
	$(CCARMHF) -c -g -fPIC src/gb_udpmanager.c -o obj/gb_udpmanager_so.o

gb_crypto_armhf: src/gb_crypto.c
	$(CCARMHF) -c -g src/gb_crypto.c -o obj/gb_crypto.o
	$(CCARMHF) -c -g -fPIC src/gb_crypto.c -o obj/gb_crypto_so.o 

gblib_armhf: gb_utility_armhf gb_serialapi_armhf gb_serialmanager_armhf gb_tcpmanager_armhf gb_udpmanager_armhf gb_crypto_armhf
	touch library/fakefile
	rm library/*
	ar r library/libgb.a obj/gb_utility.o obj/gb_serialapi.o obj/gb_serialmanager.o obj/gb_tcpmanager.o obj/gb_udpmanager.o obj/gb_crypto.o
	$(CCARMHF) -shared -o library/libgb.so obj/gb_utility_so.o obj/gb_serialapi_so.o obj/gb_serialmanager_so.o obj/gb_tcpmanager_so.o obj/gb_udpmanager_so.o obj/gb_crypto_so.o

test_armhf:
	$(CCARMHF) test/serial2tcp.c library/libgb.a -lpthread -o test/serial2tcp.out 
	$(CCARMHF) test/tcp_client.c library/libgb.a -lpthread -o test/tcp_client.out
	$(CCARMHF) test/udp_listener.c library/libgb.a -lpthread -o test/udp_listener.out
	$(CCARMHF) test/core.c library/libgb.a -lpthread -o test/core.out
	$(CCARMHF) test/test_gbcrypto.c library/libgb.a -lpthread -o test/test_gbcrypto.out
	$(CCARMHF) test/websockets.c library/libgb.a -lpthread -o test/websockets.out

gb_install_armhf:
	cp library/libgb.a $(INSTALL_LIBRARY_DIR_ARMHF)
	cp library/libgb.so $(INSTALL_LIBRARY_DIR_ARMHF)
	cp h/* $(INSTALL_INCLUDE_DIR)
##################################################################

##################################################################
# ARMEL
##################################################################
gb_utility_armel: src/gb_utility.c h/gb_utility.h
	$(CCARMEL) -c -g src/gb_utility.c -o obj/gb_utility.o
	$(CCARMEL) -c -g -fPIC src/gb_utility.c -o obj/gb_utility_so.o

gb_serialapi_armel: src/gb_serialapi.c
	$(CCARMEL) -c -g src/gb_serialapi.c -o obj/gb_serialapi.o
	$(CCARMEL) -c -g -fPIC src/gb_serialapi.c -o obj/gb_serialapi_so.o

gb_serialmanager_armel: src/gb_serialmanager.c
	$(CCARMEL) -c -g src/gb_serialmanager.c -o obj/gb_serialmanager.o
	$(CCARMEL) -c -g -fPIC src/gb_serialmanager.c -o obj/gb_serialmanager_so.o

gb_tcpmanager_armel: src/gb_tcpmanager.c
	$(CCARMEL) -c -g src/gb_tcpmanager.c -o obj/gb_tcpmanager.o
	$(CCARMEL) -c -g -fPIC src/gb_tcpmanager.c -o obj/gb_tcpmanager_so.o

gb_udpmanager_armel: src/gb_udpmanager.c
	$(CCARMEL) -c -g src/gb_udpmanager.c -o obj/gb_udpmanager.o
	$(CCARMEL) -c -g -fPIC src/gb_udpmanager.c -o obj/gb_udpmanager_so.o

gb_crypto_armel: src/gb_crypto.c
	$(CCARMEL) -c -g src/gb_crypto.c -o obj/gb_crypto.o
	$(CCARMEL) -c -c -fPIC src/gb_crypto.c -o obj/gb_crypto_so.o

gblib_armel: gb_utility_armel gb_serialapi_armel gb_serialmanager_armel gb_tcpmanager_armel gb_udpmanager_armel gb_crypto_armel
	touch library/fakefile
	rm library/*
	ar r library/libgb.a obj/gb_utility.o obj/gb_serialapi.o obj/gb_serialmanager.o obj/gb_tcpmanager.o obj/gb_udpmanager.o obj/gb_crypto.o
	$(CCARMEL) -shared -o library/libgb.so obj/gb_utility_so.o obj/gb_serialapi_so.o obj/gb_serialmanager_so.o obj/gb_tcpmanager_so.o obj/gb_udpmanager_so.o obj/gb_crypto_so.o

test_armel:
	$(CCARMEL) test/serial2tcp.c library/libgb.a -lpthread -o test/serial2tcp.out 
	$(CCARMEL) test/tcp_client.c library/libgb.a -lpthread -o test/tcp_client.out
	$(CCARMEL) test/udp_listener.c library/libgb.a -lpthread -o test/udp_listener.out
	$(CCARMEL) test/core.c library/libgb.a -lpthread -o test/core.out
	$(CCARMEL) test/test_gbcrypto.c library/libgb.a -lpthread -o test/test_gbcrypto.out
	$(CCARMEL) test/websockets.c library/libgb.a -lpthread -o test/websockets.out

gb_install_armel:
	cp library/libgb.a $(INSTALL_LIBRARY_DIR_ARMEL)
	cp library/libgb.so $(INSTALL_LIBRARY_DIR_ARMEL)
	cp h/* $(INSTALL_INCLUDE_DIR)
##################################################################

##################################################################
# X64
##################################################################
gb_utility_x64: src/gb_utility.c h/gb_utility.h
	$(CCX86) -c -g src/gb_utility.c -o obj/gb_utility.o
	$(CCX86) -c -g -fPIC src/gb_utility.c -o obj/gb_utility_so.o

gb_serialapi_x64: src/gb_serialapi.c
	$(CCX86) -c -g src/gb_serialapi.c -o obj/gb_serialapi.o
	$(CCX86) -c -g -fPIC src/gb_serialapi.c -o obj/gb_serialapi_so.o

gb_serialmanager_x64: src/gb_serialmanager.c
	$(CCX86) -c -g src/gb_serialmanager.c -o obj/gb_serialmanager.o
	$(CCX86) -c -g -fPIC src/gb_serialmanager.c -o obj/gb_serialmanager_so.o

gb_tcpmanager_x64: src/gb_tcpmanager.c
	$(CCX86) -c -g -Dx64 src/gb_tcpmanager.c -o obj/gb_tcpmanager.o
	$(CCX86) -c -g -Dx64 -fPIC src/gb_tcpmanager.c -o obj/gb_tcpmanager_so.o

gb_udpmanager_x64: src/gb_udpmanager.c
	$(CCX86) -c -g src/gb_udpmanager.c -o obj/gb_udpmanager.o
	$(CCX86) -c -g -fPIC src/gb_udpmanager.c -o obj/gb_udpmanager_so.o

gb_crypto_x64: src/gb_crypto.c
	$(CCX86) -c -g src/gb_crypto.c -o obj/gb_crypto.o
	$(CCX86) -c -g -fPIC src/gb_crypto.c -o obj/gb_crypto_so.o

gblib_x64: gb_utility_x64 gb_serialapi_x64 gb_serialmanager_x64 gb_tcpmanager_x64 gb_udpmanager_x64 gb_crypto_x64
	@echo "\e[1;34m**************************"
	@echo "GENERATING STATIC LIBRARY" 
	@echo "**************************"
	touch library/fakefile
	rm library/*
	ar r library/libgb.a obj/gb_utility.o obj/gb_serialapi.o obj/gb_serialmanager.o obj/gb_tcpmanager.o obj/gb_udpmanager.o obj/gb_crypto.o
	@echo "\e[1;36m***************************"
	@echo "GENERATING DYNAMIC LIBRARY"
	@echo "***************************"
	$(CCX86) -shared -o library/libgb.so obj/gb_utility_so.o obj/gb_serialapi_so.o obj/gb_serialmanager_so.o obj/gb_tcpmanager_so.o obj/gb_udpmanager_so.o obj/gb_crypto_so.o
	@echo "***************************\e[0m"
test_x64:
	$(CCX86) test/serial2tcp.c library/libgb.a -lpthread -o test/serial2tcp.out 
	$(CCX86) test/tcp_client.c library/libgb.a -lpthread -o test/tcp_client.out
	$(CCX86) test/udp_listener.c library/libgb.a -lpthread -o test/udp_listener.out
	$(CCX86) test/core.c library/libgb.a -lpthread -o test/core.out
	$(CCX86) test/test_gbcrypto.c library/libgb.a -lpthread -o test/test_gbcrypto.out 
	$(CCX86) test/websockets.c library/libgb.a -lpthread -o test/websockets.out 

gb_install_x64:
	cp library/libgb.a $(INSTALL_LIBRARY_DIR_X86)
	cp library/libgb.so $(INSTALL_LIBRARY_DIR_X86)
	cp h/* $(INSTALL_INCLUDE_DIR)
##################################################################

all_x86: gblib_x86 test_x86 gb_install_x86
install_x86: gb_install_x86

all_armhf: gblib_armhf test_armhf gb_install_armhf
install_armhf: gb_install_armhf

all_armel: gblib_armel test_armel gb_install_armel
install_armel: gb_install_armel

all_x64: gblib_x64 test_x64 gb_install_x64
install_x64: gb_install_x64
