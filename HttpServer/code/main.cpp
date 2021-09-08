#include <iostream>
#include "WebServer.h"
using namespace std;

int main(int argc, char** argv)
{
	
	WebServer server(13160, 3, 10000, true, 16, "zzq", "zzq", "WebServer", 3306, 20);	// 10000 10s
	server.start();
}