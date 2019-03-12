#include "Sandbox.h"
#include <typeinfo>

int main(int argc, char ** argv)
{
	sb::Sandbox sandbox;
	return sandbox.Start(1280, 768);
}