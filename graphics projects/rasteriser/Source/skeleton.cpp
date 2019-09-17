#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModelH.h"
#include <stdint.h>
#include <algorithm>
#include "Fireball_stub.h"
/* ----------------------------------------------------------------------------*/
/* GLM things and rewriting lib functions                                      */

using namespace std;
using glm::vec3;
using glm::mat3;
using glm::vec4;
using glm::mat4;
using glm::ivec2;
using glm::vec2;

SDL_Event event;

// function to print ivec2s
std::ostream& operator<<(std::ostream& out, ivec2 v) {
  return out << "(" << v.x
             << "," << v.y << ")";
}

// function to print a vertex
std::ostream& operator<<(std::ostream& out, Vertex v) {
  return out << "(" << v.position << ")";
}

bool operator==(const Vertex& lhs, const Vertex& rhs)
{
    return (lhs.position == rhs.position);
}

/* ----------------------------------------------------------------------------*/
/* Constants and Global variables                                                                   */

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 256
//#define SCREEN_WIDTH 1280
//#define SCREEN_HEIGHT 1024
#define FULLSCREEN_MODE true
#define NORMAL_FLAG true
#define FIREBALL false
#define NUM_SLICES 20
// stores the depths of all pixels
float depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

// stores the transparency of all pixels used in compositing hypertexture slices
TransBufferElement transBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

vec3 backColourBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];
/* ----------------------------------------------------------------------------*/
/* FUNCTIONS SIGS                                                              */

bool Update(vec4& cameraPos, mat4& moveCamera, mat4& moveWorld, float& yaw, vec4& T, vec4& lightPos);
void Draw(screen* screen, mat4& moveWorld, vec4& lightPos, vec3& lightcolour, vec4 cameraPos);
void TransformationMatrix(mat4& moveCamera, mat4& moveWorld, float& yaw, vec4& T);
void VertexShader(const Vertex& v, Pixel& p, float f,vec4& lightPos, vec3& lightcolour);
void PixelShader(screen* screen, const Pixel& p, vec4 currentNormal, vec3 currentColour, vec4& lightPos, vec3& lightcolour);
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result );
void DrawLineSDL( screen* screen, Pixel a, Pixel b, vec4 currentNormal, vec3 currentColour, vec4& lightPos, vec3& lightcolour);
void ComputePolygonRows(const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels,vector<Pixel>& rightPixels );
void DrawPolygon( screen* screen, const vector<Vertex>& vertices , float f, vec4& lightPos, vec3& lightcolour, vec4 currentNormal, vec3 currentColour);
void DrawRows( screen* screen, const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels, vec4 currentNormal, vec3 currentColour, vec4& lightPos, vec3& lightcolour);
void ClearBuffer();
void ClearTransBuffer(vec4 cameraPos);
void ClearBackColourBuffer();
vec4 average(vector<vec4> vs);
void DrawHypertexture(screen* screen, const vector<Vertex>& vertices, float f, vec4& lightPos, vec3& lightcolour, vec4& currentNormal);
bool ComparePixel(Pixel a, Pixel b);
void GetSliceVertices(screen* screen, vector<Pixel>& vertices, vector<Slice>& slices, mat4& moveHypertexture);
vec3 Composite(Pixel pixel, mat4& moveHypertexture, vec3& lightcolour, vec4& lightPos, vec4& currentNormal);
void DrawHypertextureSlice(screen* screen, const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels, mat4& moveHypertexture, vec4& lightPos, vec3& lightcolour, vec4& currentNormal);
float euclideanDist(vec4 p1, vec4 p2);
float softSphere(vec4 p);

/* ----------------------------------------------------------------------------*/
/* MAIN                                                                        */

int main( int argc, char* argv[] ) {

  // set up haskell
  hs_init(&argc, &argv);

  screen *screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE );

  // camera stuff
  vec4 cameraPos( 0, 0, -3,1 );
  mat4 moveWorld;
  mat4 moveCamera;
  float yaw = 0;
  vec4 T(0.0f,0.0f,-3,1);

  // light stuff
  vec4 lightPos( 0, -0.5, -0.7, 1.0 );
  vec3 lightcolour = 14.f * vec3( 1, 1, 1 );

  while ( Update(cameraPos, moveCamera, moveWorld, yaw, T, lightPos))
    {
      Draw(screen, moveWorld, lightPos, lightcolour, cameraPos);
      SDL_Renderframe(screen);
    }

  SDL_SaveImage( screen, "screenshot.bmp" );

  KillSDL(screen);
  hs_exit();
  return 0;
}

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS DEFS                                                              */

/*Place your drawing here*/
void Draw(screen* screen, mat4& moveWorld, vec4& lightPos, vec3& lightcolour, vec4 cameraPos) {
  /* Clear buffer */
  memset(screen->buffer, 0, screen->height*screen->width*sizeof(uint32_t));
  ClearBuffer();
  ClearTransBuffer(cameraPos);
  ClearBackColourBuffer();
  // loading list of triangles
  std::vector<Triangle> nonHypertextures;
  std::vector<Triangle> hypertextures;
  vec4 bottomLeft;
  vec4 bottomRight;
  vec4 topLeft;
  vec4 bottomLeftBack;
  LoadTestModel(nonHypertextures,hypertextures,bottomLeft,bottomRight,topLeft,bottomLeftBack);

  // Camera attributes
  float focalLength = SCREEN_WIDTH/2;

  // colouring normal geo (the back layer)
  for( uint32_t i=0; i<nonHypertextures.size(); ++i ) {

    vector<Vertex> vertices(3);

    vec4 currentNormal = nonHypertextures[i].normal;
    vec3 currentColour = nonHypertextures[i].color;

    vertices[0].position = moveWorld*nonHypertextures[i].v0;
    vertices[1].position = moveWorld*nonHypertextures[i].v1;
    vertices[2].position = moveWorld*nonHypertextures[i].v2;

    DrawPolygon(screen, vertices, focalLength, lightPos, lightcolour, currentNormal, currentColour);
  }

  // colouring the hypertexture
  vector<Vertex> vertices;
  for( uint32_t i=0; i<hypertextures.size(); ++i ) {

    Vertex v0;
    v0.position = moveWorld*hypertextures[i].v0;
    if (std::find(vertices.begin(), vertices.end(), v0) == vertices.end()) {
      vertices.push_back(v0);
    }
    Vertex v1;
    v1.position = moveWorld*hypertextures[i].v1;
    if (std::find(vertices.begin(), vertices.end(), v1) == vertices.end()) {
      vertices.push_back(v1);
    }
    Vertex v2;
    v2.position = moveWorld*hypertextures[i].v2;
    if (std::find(vertices.begin(), vertices.end(), v2) == vertices.end()) {
      vertices.push_back(v2);
    }

  }
  vec4 currentNormal = hypertextures[0].normal;
  //cout << hypertextures.size() << endl;
  DrawHypertexture(screen, vertices, focalLength, lightPos, lightcolour, currentNormal);


}


/*Place updates of parameters here*/
bool Update(vec4& cameraPos, mat4& moveCamera, mat4& moveWorld, float& yaw, vec4& T, vec4& lightPos) {
  //static int t = SDL_GetTicks();
  /* Compute frame time */
  //int t2 = SDL_GetTicks();
  //float dt = float(t2-t);
  //t = t2;

  SDL_Event e;
  while(SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {return false;}
      else if (e.type == SDL_KEYDOWN) {
	    int key_code = e.key.keysym.sym;
	    switch(key_code) {
	      // CAMERA
	        case SDLK_UP: // Move camera forwards
            T.z = T.z+0.1;
	          break;
	        case SDLK_DOWN: // Move camera backwards
            T.z = T.z-0.1;
		        break;
	        case SDLK_LEFT: // Tilt camera left
            yaw = yaw+0.1;
		        break;
	        case SDLK_RIGHT: // Tilt camera right
            yaw = yaw-0.1;
            break;
        // LIGHT
          case SDLK_w: // Move light forwards
            lightPos.z = lightPos.z+0.1;
		        break;
          case SDLK_s: // Move light backwards
            lightPos.z = lightPos.z-0.1;
		        break;
          case SDLK_d: // Move light right
            lightPos.x = lightPos.x+0.1;
		        break;
          case SDLK_a: // Move light left
            lightPos.x = lightPos.x-0.1;
		        break;
          case SDLK_q: // Move light up
            lightPos.y = lightPos.y+0.1;
		        break;
          case SDLK_e: // Move light down
            lightPos.y = lightPos.y-0.1;
		        break;
          // ESC
	        case SDLK_ESCAPE: // Move camera quit
		        return false;
            break;
	      }
	    }
    }

  TransformationMatrix(moveCamera,moveWorld, yaw,T);
  cameraPos = moveCamera * T;

  return true;
}

void ClearBuffer(){
  // initalises depths to 0 (they are inverse depths i.e. 1/z so zero is infinite)
  for( int y=0; y<SCREEN_HEIGHT; ++y ) {
    for( int x=0; x<SCREEN_WIDTH; ++x ) {
      depthBuffer[y][x] = 0;
    }
  }
}

void ClearTransBuffer(vec4 cameraPos){
  // initalises depths to 0 (they are inverse depths i.e. 1/z so zero is infinite)
  for( int y=0; y<SCREEN_HEIGHT; ++y ) {
    for( int x=0; x<SCREEN_WIDTH; ++x ) {
      transBuffer[y][x].transparency = 0;
      transBuffer[y][x].colour = vec3(0,0,0);
      transBuffer[y][x].density = 0;
      transBuffer[y][x].pos3d = cameraPos;
    }
  }
}

void ClearBackColourBuffer(){
  // initalises depths to 0 (they are inverse depths i.e. 1/z so zero is infinite)
  for( int y=0; y<SCREEN_HEIGHT; ++y ) {
    for( int x=0; x<SCREEN_WIDTH; ++x ) {
      backColourBuffer[y][x] = vec3(0,0,0);
    }
  }
}

// function that finds the average of a vector of vec4s
vec4 average(vector<vec4> vs) {
  vec4 total = vec4(0,0,0,0);
  for (int i=0; i<vs.size(); i++) {
    total = total + vs[i];
  }

  return total / vec4(vs.size(),vs.size(),vs.size(),vs.size());
}

// Function to produce the move matrix, both translation and rotation
void TransformationMatrix(mat4& moveCamera, mat4& moveWorld, float& yaw, vec4& T) {
  // Creating rotation matrix
  // TODO:- maybe update this to full rotation
  vec4 colOne(cos(-yaw), 0, -sin(-yaw),0);
  vec4 colTwo(0,1,0,0);
  vec4 colThree(sin(-yaw), 0, cos(-yaw),0);
  vec4 colFour(0,0,0,1);

  mat4 R(colOne,colTwo,colThree,colFour);

  // Creating translation matrices
  vec4 TcolOne(1,0,0,0);
  vec4 TcolTwo(0,1,0,0);
  vec4 TcolThree(0,0,1,0);

  vec4 TbackcolFour(-T.x,-T.y,-T.z,1);
  vec4 TforwcolFour(T.x,T.y,T.z,1);

  mat4 Tback(TcolOne,TcolTwo,TcolThree,TbackcolFour);
  mat4 Tforw(TcolOne,TcolTwo,TcolThree,TforwcolFour);

  // Updating camera position
  moveCamera =  Tforw * R * Tback;
  moveWorld  =  R * Tback;
}

// Function to project a vertex onto the 2D image plane
void VertexShader(const Vertex& v, Pixel& p, float f, vec4& lightPos, vec3& lightcolour ) {
  p.x = (int)(f * (v.position.x / (v.position.z)) + (SCREEN_WIDTH/2));
  p.y = (int)(f * (v.position.y / (v.position.z)) + (SCREEN_HEIGHT/2));
  p.zinv = 1/v.position.z;
  p.pos3d = v.position;

}

void PixelShader(screen* screen, const Pixel& p, vec4 currentNormal, vec3 currentColour, vec4& lightPos, vec3& lightcolour) {
  int x = p.x;
  int y = p.y;

  if (depthBuffer[y][x] < p.zinv) {
    vec4 rHat = lightPos - p.pos3d;
    float r = sqrt(pow(rHat.x,2) + pow(rHat.y,2) + pow(rHat.z,2)); // find dist
    rHat = glm::normalize(rHat);
    float dot = glm::dot(rHat,currentNormal);
    float scalar = std::fmax(0, dot)* 1/(4 * M_PI * pow(r,2));

    vec3 directLight = scalar * lightcolour;
    vec3 indirectLight = 0.5f*vec3(1,1,1);
    vec3 colour = currentColour * (directLight+indirectLight);

    PutPixelSDL( screen, p.x, p.y, colour );
    depthBuffer[y][x] = p.zinv;
    backColourBuffer[y][x] = colour;
  }

}

// finds points that lie on a line
void Interpolate( Pixel a, Pixel b, vector<Pixel>& result ) {
  int N = result.size();

  float x = float(b.x-a.x) / float(max(N-1,1));
  float y = float(b.y-a.y) / float(max(N-1,1));
  float zinv = (b.zinv-a.zinv) / float(max(N-1,1));

  vec4 aProj = a.pos3d * a.zinv;
  vec4 bProj = b.pos3d * b.zinv;

  vec4 pos3d = (bProj-aProj) / vec4(float(max(N-1,1)),float(max(N-1,1)),float(max(N-1,1)),float(max(N-1,1)));

  float currentX = a.x;
  float currentY = a.y;
  float currentZinv = a.zinv;
  vec4 currentPos   = aProj;

  for( int i=0; i<N; ++i ) {
    result[i].x = currentX;
    result[i].y = currentY;
    result[i].zinv = currentZinv;
    result[i].pos3d = currentPos / currentZinv;
    currentX = currentX + x;
    currentY = currentY + y;
    currentZinv = currentZinv + zinv;
    currentPos  = currentPos + pos3d;
  }
}

// draws one line given two points in image space
void DrawLineSDL( screen* screen, Pixel a, Pixel b, vec4 currentNormal, vec3 currentColour, vec4& lightPos, vec3& lightcolour) {
  int deltaX =  glm::abs( a.x - b.x );
  int deltaY =  glm::abs( a.y - b.y );
  int pixels = glm::max( deltaX, deltaY ) + 1;
  vector<Pixel> line( pixels );

  Interpolate( a, b, line );

  for (int i=0; i<line.size(); i++) {
      PixelShader(screen, line[i], currentNormal, currentColour, lightPos, lightcolour);
  }
}

// function that calculates the rows of a polygon so it can be coloured
void ComputePolygonRows(const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels,vector<Pixel>& rightPixels ) {

  // Find min y-value in vertexPixels
  int min = +numeric_limits<int>::max();
  for (int i=0; i<vertexPixels.size(); i++) {
    if (vertexPixels[i].y < min) min = vertexPixels[i].y;
  }


  // Find max y-value in vertexPixels
  int max = -numeric_limits<int>::max();
  for (int i=0; i<vertexPixels.size(); i++) {
    if (vertexPixels[i].y > max) max = vertexPixels[i].y;
  }

  // find number of rows
  int rows = max - min +1;

  // making left and right pixel arrays the correct size
  leftPixels.resize(rows);
  rightPixels.resize(rows);

  // initialise left and right pixels to big values
  for( int i=0; i<rows; ++i ) {
    leftPixels[i].x = +numeric_limits<int>::max();
    rightPixels[i].x = -numeric_limits<int>::max();
  }

  // Loop over all vertices to get points that are on the edge of each polygon
  for( int i=0; i < vertexPixels.size(); ++i ) {

    int j = (i+1)%vertexPixels.size(); // The next vertex
    int deltaX =  glm::abs( vertexPixels[i].x - vertexPixels[j].x );
    int deltaY =  glm::abs( vertexPixels[i].y - vertexPixels[j].y );
    int pixels = glm::max( deltaX, deltaY ) + 1;
    vector<Pixel> edgePoints( pixels );

    Interpolate( vertexPixels[i], vertexPixels[j], edgePoints );

    for(int y = min; y<=max; y++) {
      for (int p=0; p<edgePoints.size(); p++){
        if (edgePoints[p].y == y) {
          if (edgePoints[p].x > rightPixels[y-min].x) {
            rightPixels[y-min].x = edgePoints[p].x;
            rightPixels[y-min].y = y;
            rightPixels[y-min].zinv = edgePoints[p].zinv;
            rightPixels[y-min].pos3d = edgePoints[p].pos3d;
          }
          if (edgePoints[p].x < leftPixels[y-min].x) {
            leftPixels[y-min].x = edgePoints[p].x;
            leftPixels[y-min].y = y;
            leftPixels[y-min].zinv = edgePoints[p].zinv;
            leftPixels[y-min].pos3d = edgePoints[p].pos3d;
          }
        }
      }
    }
  }
}

// function that draws a polygon given its vertices
void DrawPolygon( screen* screen, const vector<Vertex>& vertices, float f, vec4& lightPos, vec3& lightcolour, vec4 currentNormal, vec3 currentColour) {
  int V = vertices.size();
  vector<Pixel> vertexPixels( V );
  for( int i=0; i<V; ++i ) { // fill in vertexPixels
    VertexShader( vertices[i], vertexPixels[i], f, lightPos, lightcolour);
  }

  // draw polygon
  vector<Pixel> leftPixels;
  vector<Pixel> rightPixels;
  ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
  DrawRows( screen, leftPixels, rightPixels, currentNormal, currentColour, lightPos, lightcolour  );
}

// function that given left and right sides of row of a polygon colours it in
void DrawRows( screen* screen, const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels, vec4 currentNormal, vec3 currentColour, vec4& lightPos, vec3& lightcolour) {
  for (int i = 0; i < rightPixels.size(); i++){ // colour row by row
    DrawLineSDL(screen, leftPixels[i],rightPixels[i], currentNormal, currentColour, lightPos, lightcolour);
  }
}

// hypertexture:
float softSphere(vec4 p) {
  vec4 center(0.3,0.3,0.3,1);
  float r = 0.3;
  float density;
  float dist = euclideanDist(p, center);
  if (dist < r) density = (1- dist/r);
  else density = 0;

  return density;

}

// hyper helper
float euclideanDist(vec4 p1, vec4 p2) {
  return sqrt(pow((p1.x-p2.x),2)+pow((p1.y-p2.y),2)+pow((p1.z-p2.z),2)+pow((p1.w-p2.w),2));
}


//function that uses transBuffer to compose pixels from different slices
vec3 Composite(Pixel pixel, mat4& moveHypertexture, vec3& lightcolour, vec4& lightPos, vec4& currentNormal) {
  vec3 totalColour = transBuffer[pixel.y][pixel.x].colour;
  float totalTrans = transBuffer[pixel.y][pixel.x].transparency;

  if (totalTrans < 1){
    //normal calculation part1
    //get current ray direction
    vec4 rayDirection = pixel.pos3d - transBuffer[pixel.y][pixel.x].pos3d;
    vec3 rayDirection3 = vec3(rayDirection.x, rayDirection.y, rayDirection.z);

    //generate throwaway vector used to create a vector perpendicular to rayDirection
    vec4 colOne(cos(30), 0, -sin(30),0);
    vec4 colTwo(0,1,0,0);
    vec4 colThree(sin(30), 0, cos(30),0);
    vec4 colFour(0,0,0,1);
    mat4 throwAwayMatrix = mat4(colOne, colTwo, colThree, colFour);
    vec4 throwAwayDirection = throwAwayMatrix*rayDirection;
    vec3 throwAwayDirection3 = vec3(throwAwayDirection.x, throwAwayDirection.y, throwAwayDirection.z);

    //find two perp directions using cross product
    vec3 perpDirection1result = glm::cross(rayDirection3, throwAwayDirection3);
    vec4 perpDirection1 = vec4(perpDirection1result.x, perpDirection1result.y, perpDirection1result.z, 1);
    vec3 perpDirection2result = glm::cross(rayDirection3, perpDirection1result);
    vec4 perpDirection2 = vec4(perpDirection2result.x, perpDirection2result.y, perpDirection2result.z, 1);

    vec4 hyperPoint = moveHypertexture*pixel.pos3d;
    // look up in hyper
    double density;
    if (FIREBALL){density = fireball(hyperPoint.x,hyperPoint.y,hyperPoint.z);}
    else {
      hyperPoint.w = 1;
      density = softSphere(hyperPoint);
    }

    // change colour based on density
    float red = std::fmin(1.0, 0.9+(float)density);
    float green = 0.96;
    if (density < 0.1) green = std::fmin(0.96, (float)density);
    float blue = 0;
    vec3 currentColour = vec3(red,green,blue);

    if (NORMAL_FLAG){
      vec4 perpPoint1 = moveHypertexture*(pixel.pos3d+perpDirection1);
      vec4 perpPoint2 = moveHypertexture*(pixel.pos3d+perpDirection2);

      double perpDensity1;
      double perpDensity2;
      if (FIREBALL){
        perpDensity1 = fireball(perpPoint1.x,perpPoint1.y,perpPoint1.z);
        perpDensity2 = fireball(perpPoint2.x,perpPoint2.y,perpPoint2.z);
      }
      else {
        perpPoint1.w = 1;
        perpPoint2.w = 1;
        perpDensity1 = softSphere(perpPoint1);
        perpDensity2 = softSphere(perpPoint2);
      }
      //evaluate change in fireball in three directions
      float densityChange1 = (float)perpDensity1 - (float)density;
      float densityChange2 = (float)perpDensity2 - (float)density;
      float densityChange3;
      if (transBuffer[pixel.y][pixel.x].transparency == 0){densityChange3 = 0;}
      else {densityChange3 = (float)density - transBuffer[pixel.y][pixel.x].density;}

      //move to xyz space
      vec4 densityChange = densityChange1*perpDirection1 + densityChange2*perpDirection2 + densityChange3*rayDirection;

      //normalise
      vec4 newNormal = glm::normalize(densityChange);
      newNormal.w = 1;

      currentNormal = newNormal;
    }

    vec4 rHat = lightPos - pixel.pos3d;
    float r = sqrt(pow(rHat.x,2) + pow(rHat.y,2) + pow(rHat.z,2)); // find dist
    rHat = glm::normalize(rHat);
    float dot = glm::dot(rHat,currentNormal);
    float scalar = std::fmax(0, dot)* 1/(4 * M_PI * pow(r,2));

    vec3 directLight = scalar * lightcolour;
    vec3 indirectLight = 0.5f*vec3(1,1,1);
    currentColour = currentColour * (directLight+indirectLight);
    currentColour = currentColour*vec3(1.5,1.5,1.5);

    float transparency = (float)density * (1-totalTrans);
    totalColour = totalColour + transparency * currentColour;
    totalTrans = totalTrans + transparency;

    transBuffer[pixel.y][pixel.x].transparency = totalTrans;
    transBuffer[pixel.y][pixel.x].colour = totalColour;
    transBuffer[pixel.y][pixel.x].density = (float)density;
    transBuffer[pixel.y][pixel.x].pos3d = pixel.pos3d;
  }

  //adding background colour
  totalColour = totalColour + (1-totalTrans) * backColourBuffer[pixel.y][pixel.x];

  return totalColour;

}

void DrawHypertextureSlice(screen* screen, const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels, mat4& moveHypertexture, vec4& lightPos, vec3& lightcolour, vec4& currentNormal){
  // get the RBGA value for the hypertexures
  // composite using transparency buffer
  // draw
  for (int z = 0; z < leftPixels.size(); z++){
    int deltaX =  glm::abs( rightPixels[z].x - leftPixels[z].x );
    int deltaY =  glm::abs( rightPixels[z].y - leftPixels[z].y );
    int pixels = glm::max( deltaX, deltaY ) + 1;
    vector<Pixel> line( pixels );

    Interpolate( leftPixels[z], rightPixels[z], line );

    for (int i=0; i<line.size(); i++) {
        vec3 colour = Composite(line[i], moveHypertexture, lightcolour, lightPos, currentNormal);
        PutPixelSDL(screen, line[i].x, line[i].y, colour);
    }
  }
}

// function that fins what hypertexture to overlay, and overlays it
void DrawHypertexture(screen* screen, const vector<Vertex>& vertices, float f, vec4& lightPos, vec3& lightcolour, vec4& currentNormal) {
  int V = vertices.size();
  vector<Pixel> vertexPixels( V );
  for( int i=0; i<V; ++i ) { // fill in vertexPixels
    VertexShader( vertices[i], vertexPixels[i], f, lightPos, lightcolour);
  }

/* testy code
  for( int i=0; i<vertexPixels.size(); ++i ) { // fill in vertexPixels
    PutPixelSDL(screen, vertexPixels[i].x, vertexPixels[i].y, vec3(0,0,0));
  }

  cout << "new" << endl;
  sort(vertexPixels.begin(), vertexPixels.end(), ComparePixel);
  for( int i=0; i<vertexPixels.size(); ++i ) { // fill in vertexPixels
    cout << vertexPixels[i].zinv << endl;
  }
  */

  sort(vertexPixels.begin(), vertexPixels.end(), ComparePixel);
  vector<Slice> slices;
  mat4 moveHypertexture;
  GetSliceVertices(screen, vertexPixels, slices, moveHypertexture);

  // go through slice by slice
  for (int q = 0; q < slices.size(); q++ ){
    //Calculate slice row coordinates
    vector<Pixel> leftPixels;
    vector<Pixel> rightPixels;
    vector<Pixel> slicePixels;
    slicePixels.push_back(slices[q].topLeft);
    slicePixels.push_back(slices[q].topRight);
    slicePixels.push_back(slices[q].bottomRight);
    slicePixels.push_back(slices[q].bottomLeft);
    ComputePolygonRows( slicePixels, leftPixels, rightPixels );
    // draw slice
    DrawHypertextureSlice(screen, leftPixels, rightPixels, moveHypertexture, lightPos, lightcolour, currentNormal);
  }
}

bool ComparePixel(Pixel a, Pixel b) {
  return (a.zinv < b.zinv);
}

// takes the front and back corners and generates corners of intermedary slices using interpolation
void GetSliceVertices(screen* screen, vector<Pixel>& vertices, vector<Slice>& slices, mat4& moveHypertexture) {
  // find which pixel is which
  // NOTE:- y axis inverted
  // finding back corners
  Pixel backTopLeft     = vertices[0];
  Pixel backTopRight    = vertices[0];
  Pixel backBottomLeft  = vertices[0];
  Pixel backBottomRight = vertices[0];
  for (int i =1; i <=3; i++) {
    // top left: largest y, smallest x
    if (backTopLeft.pos3d.y <= vertices[i].pos3d.y && backTopLeft.pos3d.x >= vertices[i].pos3d.x ) {
      backTopLeft = vertices[i];
    }
    // top right: biggest y, biggest x
    if (backTopRight.pos3d.y <= vertices[i].pos3d.y && backTopRight.pos3d.x <= vertices[i].pos3d.x) {
      backTopRight = vertices[i];
    }
    // bottom left: smallest y, smallest x
    if (backBottomLeft.pos3d.y >= vertices[i].pos3d.y && backBottomLeft.pos3d.x >= vertices[i].pos3d.x) {
      backBottomLeft = vertices[i];
    }
    // bottom right: smallest y, biggest x
    if (backBottomRight.pos3d.y >= vertices[i].pos3d.y && backBottomRight.pos3d.x <= vertices[i].pos3d.x) {
      backBottomRight = vertices[i];
    }
  }
  // finding front corners
  Pixel frontTopLeft     = vertices[4];
  Pixel frontTopRight    = vertices[4];
  Pixel frontBottomLeft  = vertices[4];
  Pixel frontBottomRight = vertices[4];
  for (int i =5; i <vertices.size(); i++) {
    // top left: largest y, smallest x
    if (frontTopLeft.pos3d.y <= vertices[i].pos3d.y && frontTopLeft.pos3d.x >= vertices[i].pos3d.x ) {
      frontTopLeft = vertices[i];
    }
    // top right: biggest y, biggest x
    if (frontTopRight.pos3d.y <= vertices[i].pos3d.y && frontTopRight.pos3d.x <= vertices[i].pos3d.x) {
      frontTopRight = vertices[i];
    }
    // bottom left: smallest y, smallest x
    if (frontBottomLeft.pos3d.y >= vertices[i].pos3d.y && frontBottomLeft.pos3d.x >= vertices[i].pos3d.x) {
      frontBottomLeft = vertices[i];
    }
    // bottom right: smallest y, biggest x
    if (frontBottomRight.pos3d.y >= vertices[i].pos3d.y && frontBottomRight.pos3d.x <= vertices[i].pos3d.x) {
      frontBottomRight = vertices[i];
    }
  }

  // interpolate each edge
  vector<Pixel>topLeftList(NUM_SLICES);
  Interpolate(frontTopLeft, backTopLeft, topLeftList);

  vector<Pixel>topRightList(NUM_SLICES);
  Interpolate(frontTopRight, backTopRight, topRightList);

  vector<Pixel>bottomLeftList(NUM_SLICES);
  Interpolate(frontBottomLeft, backBottomLeft, bottomLeftList);

  vector<Pixel>bottomRightList(NUM_SLICES);
  Interpolate(frontBottomRight, backBottomRight, bottomRightList);
  // slice by slice add vertices to master list

  for (int p = 0; p < topLeftList.size(); p++){
    Slice newSlice;
    newSlice.topLeft = topLeftList[p];
    newSlice.bottomLeft = bottomLeftList[p];
    newSlice.topRight = topRightList[p];
    newSlice.bottomRight = bottomRightList[p];
    slices.push_back(newSlice);
  }

  // Creating translation matrices
  vec4 TcolOne(1,0,0,0);
  vec4 TcolTwo(0,1,0,0);
  vec4 TcolThree(0,0,1,0);
  vec4 bottom(-frontBottomLeft.pos3d.x,-frontBottomLeft.pos3d.y,-frontBottomLeft.pos3d.z,1);

  mat4 hyperT(TcolOne,TcolTwo,TcolThree, bottom);

  // for rotation of hyper
  vec4 xaxis = frontBottomRight.pos3d - frontBottomLeft.pos3d;
  vec4 yaxis = frontTopLeft.pos3d - frontBottomLeft.pos3d;
  vec4 zaxis = backBottomLeft.pos3d - frontBottomLeft.pos3d;

  xaxis = glm::normalize(xaxis);
  yaxis = glm::normalize(yaxis);
  zaxis = glm::normalize(zaxis);
  vec4 RcolFour(0,0,0,1);

  mat4 hyperRotation(xaxis, yaxis, zaxis, RcolFour);

  moveHypertexture = glm::inverse(hyperRotation) * hyperT;

}
