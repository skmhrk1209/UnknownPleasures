#include "ofMain.h"
#include "unknown_pleasures.hpp"

int main()
{
	ofSetupOpenGL(1024, 768, OF_WINDOW);
	ofRunApp(new UnknownPleasures(200, 300));
}
