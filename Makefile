all : http_proxy

http_proxy: 
	g++ -o http_proxy http_proxy.cpp -lpcap -lpthread

clean:
	rm -f http_proxy
