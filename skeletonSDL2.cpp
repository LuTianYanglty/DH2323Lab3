//DH2323 skeleton code, Lab3 (SDL2 version)
#include <iostream>
#include <glm/glm.hpp>
#include "SDL2auxiliary.h"
#include "TestModel.h"
#include <algorithm> //for max()

using namespace std;
using glm::vec3;
using glm::mat3;

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL2Aux *sdlAux;
int t;
vector<Triangle> triangles;

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update(void);
void Draw(void);

int main(int argc, char* argv[])
{
	LoadTestModel(triangles);  // Load model
	sdlAux = new SDL2Aux(SCREEN_WIDTH, SCREEN_HEIGHT);
	t = SDL_GetTicks();	// Set start value for timer.

	while (!sdlAux->quitEvent())
	{
		Update();
		Draw();
	}
	sdlAux->saveBMP("screenshot.bmp");
	return 0;
}

void Update(void)
{
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2 - t);
	t = t2;
	cout << "Render time: " << dt << " ms." << endl;

	const Uint8* keystate = SDL_GetKeyboardState(NULL);
	if (keystate[SDL_SCANCODE_UP]) {
		// Move camera forward
	}
	if (keystate[SDL_SCANCODE_DOWN]) {
		// Move camera backward
	}
	if (keystate[SDL_SCANCODE_LEFT]) {
		// Move camera to the left
	}
	if (keystate[SDL_SCANCODE_RIGHT]) {
		// Move camera to the right
	}
	if (keystate[SDL_SCANCODE_W]) {

	}
	if (keystate[SDL_SCANCODE_S]) {

	}
	if (keystate[SDL_SCANCODE_A]) {

	}
	if (keystate[SDL_SCANCODE_D]) {

	}
	if (keystate[SDL_SCANCODE_Q]) {

	}
	if (keystate[SDL_SCANCODE_E]) {

	}
}

void Draw()
{
	sdlAux->clearPixels();

	for (int i = 0; i<triangles.size(); ++i)
	{
		vector<vec3> vertices(3);

		vertices[0] = triangles[i].v0;
		vertices[1] = triangles[i].v1;
		vertices[2] = triangles[i].v2;

		// Add drawing
	}

	sdlAux->render();
}