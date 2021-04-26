all: csiauthenticator updator accessauth csiauth


csiauthenticator: csiauthenticator.o
	g++ -o csiauthenticator csiauthenticator.o -lpthread

updator: updator.o ensemble.o autoencoder.o utils.o
	g++ -o updator updator.o ensemble.o autoencoder.o utils.o

accessauth: accessauth.o ensemble.o autoencoder.o utils.o
	g++ -o accessauth accessauth.o ensemble.o autoencoder.o utils.o

csiauth: csiauth.o ensemble.o autoencoder.o utils.o
	g++ -o csiauth csiauth.o ensemble.o autoencoder.o utils.o



csiauthenticator.o: csiauthenticator.cpp
	g++ -c csiauthenticator.cpp

updator.o: updator.cpp ensemble.h
	g++ -c updator.cpp

accessauth.o: accessauth.cpp ensemble.h
	g++ -c accessauth.cpp

csiauth.o: csiauth.cpp ensemble.h
	g++ -c csiauth.cpp

ensemble.o: ensemble.cpp ensemble.h autoencoder.h
	g++ -c ensemble.cpp

autoencoder.o: autoencoder.cpp autoencoder.h utils.h
	g++ -c autoencoder.cpp

utils.o: utils.cpp utils.h
	g++ -c utils.cpp

clean:
	rm csiauthenticator csiauthenticator.o updator updator.o accessauth accessauth.o csiauth csiauth.o ensemble.o autoencoder.o utils.o
