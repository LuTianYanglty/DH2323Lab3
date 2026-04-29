//DH2323 skeleton code, Lab3 (SDL2 version)
#include <iostream>
#include <glm/glm.hpp>
#include "SDL2Auxiliary.h"
#include "TestModel.h"
#include <algorithm>
#include <limits>

using namespace std;
using glm::vec2;
using glm::vec3;
using glm::mat3;
using glm::ivec2;

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL2Aux *sdlAux;
int t;
vector<Triangle> triangles;

float focalLength = SCREEN_WIDTH;
vec3  cameraPos(0.0f, 0.0f, -3.001f);
mat3  cameraRot(1.0f);
float yaw = 0.0f;

vec3  lightPos(-0.1f, -0.2f, -0.4f);
vec3  lightPower             = 9.1f * vec3(1, 1, 1);
vec3  indirectLightPowerPerArea = 0.5f * vec3(1, 1, 1);

float depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];
vec3  currentNormal;
vec3  currentReflectance;

struct Pixel
{
	int   x;
	int   y;
	float zinv;
	vec3  pos;
};

struct Vertex
{
	vec3 position;
};

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update(void);
void Draw(void);
void VertexShader(const Vertex& v, Pixel& p);
void Interpolate(ivec2 a, ivec2 b, vector<ivec2>& result);
void Interpolate(Pixel a, Pixel b, vector<Pixel>& result);
void DrawLineSDL(ivec2 a, ivec2 b, vec3 color);
void DrawPolygonEdges(const vector<Vertex>& vertices);
void ComputePolygonRows(const vector<Pixel>& vertexPixels,
                        vector<Pixel>& leftPixels,
                        vector<Pixel>& rightPixels);
void DrawRows(const vector<Pixel>& leftPixels,
              const vector<Pixel>& rightPixels);
void PixelShader(const Pixel& p);
void DrawPolygon(const vector<Vertex>& vertices);

int main(int argc, char* argv[])
{
	LoadTestModel(triangles);
	sdlAux = new SDL2Aux(SCREEN_WIDTH, SCREEN_HEIGHT);
	t = SDL_GetTicks();

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
	int t2 = SDL_GetTicks();
	float dt = float(t2 - t);
	t = t2;
	cout << "Render time: " << dt << " ms." << endl;

	float moveSpeed = 0.002f * dt;
	float rotSpeed  = 0.001f * dt;

	const Uint8* keystate = SDL_GetKeyboardState(NULL);
	if (keystate[SDL_SCANCODE_UP])
		cameraPos += moveSpeed * vec3(cameraRot[0][2], cameraRot[1][2], cameraRot[2][2]);
	if (keystate[SDL_SCANCODE_DOWN])
		cameraPos -= moveSpeed * vec3(cameraRot[0][2], cameraRot[1][2], cameraRot[2][2]);
	if (keystate[SDL_SCANCODE_LEFT]) {
		yaw -= rotSpeed;
		cameraRot = mat3( cos(yaw), 0, sin(yaw),
		                  0,        1, 0,
		                 -sin(yaw), 0, cos(yaw));
	}
	if (keystate[SDL_SCANCODE_RIGHT]) {
		yaw += rotSpeed;
		cameraRot = mat3( cos(yaw), 0, sin(yaw),
		                  0,        1, 0,
		                 -sin(yaw), 0, cos(yaw));
	}
}

void Interpolate(ivec2 a, ivec2 b, vector<ivec2>& result)
{
	int N = result.size();
	vec2 step = vec2(b - a) / float(max(N - 1, 1));
	vec2 current(a);
	for (int i = 0; i < N; ++i)
	{
		result[i] = current;
		current += step;
	}
}

void Interpolate(Pixel a, Pixel b, vector<Pixel>& result)
{
	int N = result.size();
	float inv = 1.0f / float(max(N - 1, 1));
	float stepX    = (b.x    - a.x)    * inv;
	float stepY    = (b.y    - a.y)    * inv;
	float stepZinv = (b.zinv - a.zinv) * inv;
	vec3  stepPos  = (b.pos  - a.pos)  * inv;
	float curX = a.x, curY = a.y, curZinv = a.zinv;
	vec3  curPos = a.pos;
	for (int i = 0; i < N; ++i)
	{
		result[i] = { int(curX), int(curY), curZinv, curPos };
		curX    += stepX;
		curY    += stepY;
		curZinv += stepZinv;
		curPos  += stepPos;
	}
}

void DrawLineSDL(ivec2 a, ivec2 b, vec3 color)
{
	ivec2 delta = glm::abs(a - b);
	int pixels = glm::max(delta.x, delta.y) + 1;
	vector<ivec2> line(pixels);
	Interpolate(a, b, line);
	for (int i = 0; i < pixels; ++i)
		sdlAux->putPixel(line[i].x, line[i].y, color);
}

void DrawPolygonEdges(const vector<Vertex>& vertices)
{
	int V = vertices.size();
	vector<Pixel> proj(V);
	for (int i = 0; i < V; ++i)
		VertexShader(vertices[i], proj[i]);

	for (int i = 0; i < V; ++i)
	{
		int j = (i + 1) % V;
		DrawLineSDL(ivec2(proj[i].x, proj[i].y),
		            ivec2(proj[j].x, proj[j].y),
		            vec3(1, 1, 1));
	}
}

void ComputePolygonRows(const vector<Pixel>& vertexPixels,
                        vector<Pixel>& leftPixels,
                        vector<Pixel>& rightPixels)
{
	int minY = SCREEN_HEIGHT - 1;
	int maxY = 0;
	for (const auto& v : vertexPixels) {
		minY = min(minY, max(v.y, 0));
		maxY = max(maxY, min(v.y, SCREEN_HEIGHT - 1));
	}
	if (minY > maxY) { leftPixels.clear(); rightPixels.clear(); return; }

	int ROWS = maxY - minY + 1;
	leftPixels.resize(ROWS);
	rightPixels.resize(ROWS);

	for (int i = 0; i < ROWS; ++i) {
		leftPixels[i]  = { numeric_limits<int>::max(), minY + i, 0.0f, vec3(0) };
		rightPixels[i] = { numeric_limits<int>::min(), minY + i, 0.0f, vec3(0) };
	}

	int V = vertexPixels.size();
	for (int i = 0; i < V; ++i) {
		int j = (i + 1) % V;
		Pixel a = vertexPixels[i];
		Pixel b = vertexPixels[j];

		int pixels = glm::max(abs(b.x - a.x), abs(b.y - a.y)) + 1;
		vector<Pixel> line(pixels);
		Interpolate(a, b, line);

		for (const auto& p : line) {
			int row = p.y - minY;
			if (row < 0 || row >= ROWS) continue;
			if (p.x < leftPixels[row].x)  leftPixels[row]  = p;
			if (p.x > rightPixels[row].x) rightPixels[row] = p;
		}
	}
}

void PixelShader(const Pixel& p)
{
	if (p.x < 0 || p.x >= SCREEN_WIDTH ||
	    p.y < 0 || p.y >= SCREEN_HEIGHT) return;
	if (p.zinv > depthBuffer[p.y][p.x])
	{
		depthBuffer[p.y][p.x] = p.zinv;

		vec3  r    = lightPos - p.pos;
		float r2   = glm::dot(r, r);
		vec3  rhat = glm::normalize(r);
		float cosA = glm::max(glm::dot(rhat, currentNormal), 0.0f);
		vec3  D    = lightPower * cosA / (4.0f * 3.14159265f * r2);
		vec3  illumination = currentReflectance * (D + indirectLightPowerPerArea);

		sdlAux->putPixel(p.x, p.y, illumination);
	}
}

void DrawRows(const vector<Pixel>& leftPixels,
              const vector<Pixel>& rightPixels)
{
	for (int row = 0; row < (int)leftPixels.size(); ++row) {
		int cnt = rightPixels[row].x - leftPixels[row].x + 1;
		if (cnt <= 0) continue;
		vector<Pixel> line(cnt);
		Interpolate(leftPixels[row], rightPixels[row], line);
		for (const auto& p : line)
			PixelShader(p);
	}
}

void VertexShader(const Vertex& v, Pixel& p)
{
	vec3 pos = cameraRot * (v.position - cameraPos);
	p.zinv = 1.0f / pos.z;
	p.x = int(focalLength * pos.x * p.zinv) + SCREEN_WIDTH  / 2;
	p.y = int(focalLength * pos.y * p.zinv) + SCREEN_HEIGHT / 2;
	p.pos = v.position;
}

void DrawPolygon(const vector<Vertex>& vertices)
{
	int V = vertices.size();
	vector<Pixel> vertexPixels(V);
	for (int i = 0; i < V; ++i)
		VertexShader(vertices[i], vertexPixels[i]);

	vector<Pixel> leftPixels, rightPixels;
	ComputePolygonRows(vertexPixels, leftPixels, rightPixels);
	DrawRows(leftPixels, rightPixels);
}

void Draw()
{
	sdlAux->clearPixels();

	for (int y = 0; y < SCREEN_HEIGHT; ++y)
		for (int x = 0; x < SCREEN_WIDTH; ++x)
			depthBuffer[y][x] = 0;

	for (int i = 0; i < (int)triangles.size(); ++i)
	{
		currentNormal      = triangles[i].normal;
		currentReflectance = triangles[i].color;

		vector<Vertex> vertices(3);
		vertices[0] = { triangles[i].v0 };
		vertices[1] = { triangles[i].v1 };
		vertices[2] = { triangles[i].v2 };
		DrawPolygon(vertices);
	}

	sdlAux->render();
}
