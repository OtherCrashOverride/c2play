all:
	g++ -g -std=c++11 main.cpp -o c2play -lavformat -lavcodec -lavutil -L/usr/lib/aml_libs -lamcodec -lamadec -lamavutils -lpthread -ldl -lasound
