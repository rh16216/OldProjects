#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModelH.h"
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <algorithm>
#include "Fireball_stub.h"

/* ----------------------------------------------------------------------------*/
/* GLM things                                                              */

// GLM names
using namespace std;
using glm::vec3;
using glm::mat3;
using glm::vec4;
using glm::mat4;

/* ----------------------------------------------------------------------------*/
/* Constants                                                                   */

//#define SCREEN_WIDTH 32
//#define SCREEN_HEIGHT 26
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 48
//#define SCREEN_WIDTH 320
//#define SCREEN_HEIGHT 256
//#define SCREEN_WIDTH 1280
//#define SCREEN_HEIGHT 1024

#define FULLSCREEN_MODE true
#define RAY_MARCH_STEP 10
#define NORMAL_FLAG false

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS SIGS                                                              */

bool Update(vec4& cameraPos, mat4& move, mat4& moveHypertexture, float& yaw, vec4& T, vector<vec4>& lights);
void Draw(screen* screen, vec4& cameraPos, mat4& move, mat4& moveHypertexture, vector<vec4> lights, vec3 lightcolour );
vec3 returnItersectionParams(Triangle tri, vec3 s, vec3 d);
bool ClosestIntersection(
       vec4 start, // start point
       vec4 dir,   // direction
       const vector<Triangle>& triangles,
       Intersection& closestIntersection );
vec3 DirectLight( vec4 normal, vec4 position, vector<vec4> lights, vec3 lightcolour, const vector<Triangle>& triangles, mat4& moveHypertexture, vec4 bottomLeft, vec4 cameraPos);
void TransformationMatrix(mat4& move, mat4& moveHypertexture, float& yaw, vec4& T);
vec3 Composite(vector<vec4>);
void Interpolate( vec4 a, vec4 b, vector<vec4>& result );
void rayMarch(mat4& moveHypertexture, vec4 bottomLeft, vector<vec4>& layers, vec4 frontPoint, vec4 backPoint, vector<vec4> lights, vec3 lightcolour, vec4 cameraPos, const vector<Triangle>& triangles, vec3 indirectLight, vec4 normal);
void lookUpColour(mat4& moveHypertexture, vec4 bottomLeft, vector<vec4>& points, vector<vec4>& layers, vector<vec4> lights, vec3 lightcolour, vec4 cameraPos, const vector<Triangle>& triangles, vec3 indirectLight, vec4 normal);
float rayMarchDensity(mat4& moveHypertexture, vec4 bottomLeft, vec4 frontPoint, vec4 backPoint);
float lookUpDensity(mat4& moveHypertexture, vec4 bottomLeft, vector<vec4>& points);
vec3 lightCalc(vec4 rHat, vec4 normal, vec3 lightcolour, float r );

/* ----------------------------------------------------------------------------*/
/* MAIN                                                                        */
int main( int argc, char* argv[] ) {

  // set up haskell
  hs_init(&argc, &argv);

  // camera stuff
  vec4 cameraPos(0.0f,0.0f,-3,1);
  mat4 moveHypertexture;
  mat4 move;
  float yaw = 0;
  vec4 T(0.0f,0.0f,-3,1);

  // light stuff
  // group of lights to simulate soft shadows
  vec4 lightPos1( 0, -0.5, -0.7, 1.0 );
  vec4 lightPos2( -0.1, -0.5, -0.7, 1.0 );
  vec4 lightPos3( 0.1, -0.5, -0.7, 1.0 );
  vec4 lightPos4( 0, -0.6, -0.7, 1.0 );
  vec4 lightPos5( 0, -0.4, -0.7, 1.0 );
  vec4 lightPos6( 0, -0.5, -0.8, 1.0 );
  vec4 lightPos7( 0, -0.5, -0.6, 1.0 );
  vector<vec4> lights;
  lights.push_back(lightPos1);
  lights.push_back(lightPos2);
  lights.push_back(lightPos3);
  lights.push_back(lightPos4);
  lights.push_back(lightPos5);
  lights.push_back(lightPos6);
  lights.push_back(lightPos7);
  vec3 lightcolour = 2.f * vec3( 1, 1, 1 );

  // Rendering loop
  screen *screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE );
  while ( Update(cameraPos, move, moveHypertexture, yaw, T, lights)) {
      Draw(screen, cameraPos, move, moveHypertexture, lights, lightcolour);
      SDL_Renderframe(screen);
    }
  SDL_SaveImage( screen, "screenshot.bmp" );

  KillSDL(screen);
  hs_exit();
  return 0;
}

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS DEFS                                                              */

// function that draws onto screen
void Draw(screen* screen, vec4& cameraPos, mat4& move, mat4& moveHypertexture, vector<vec4> lights, vec3 lightcolour ) {

  // Clear buffer
  memset(screen->buffer, 0, screen->height*screen->width*sizeof(uint32_t));

  // Camera attributes
  float focalLength = SCREEN_WIDTH/2;
  // loading list of triangles
  std::vector<Triangle> nonHypertextures;
  std::vector<Triangle> hypertextures;
  vec4 bottomLeft;
  vec4 bottomRight;
  vec4 topLeft;
  vec4 topRight;
  LoadTestModel(nonHypertextures,hypertextures,bottomLeft,bottomRight,topLeft,topRight);

  std::vector<Triangle> triangles = hypertextures;
  triangles.insert(triangles.end(), nonHypertextures.begin(), nonHypertextures.end());

  // Colouring!!!
  for (int x = 0; x < SCREEN_HEIGHT; x ++) {
    for (int y = 0; y < SCREEN_WIDTH; y ++) {
      // finding point in image plane taking into account camera pos
      vec4 d(x-SCREEN_HEIGHT/2, y-SCREEN_WIDTH/2,focalLength,1);
      d = move * d;
      d = glm::normalize(d);
      //finding intersection and colouring accordingly
      Intersection closestIntersection;
      vec3 colour = black;
      vec3 indirectLight = 0.5f*vec3(1,1,1);
      if (ClosestIntersection(cameraPos, d, triangles, closestIntersection )) {
        colour = (DirectLight(triangles[closestIntersection.triangleIndex].normal, closestIntersection.position, lights, lightcolour, triangles, moveHypertexture, bottomLeft, cameraPos )
                + indirectLight)
                * triangles[closestIntersection.triangleIndex].colour;
        // dealing with hypertexture
        // NOTE:- only designed to deal with ONE hypertexture
        // if you want more than one phenomina put it in one big bounding box
        if (triangles[closestIntersection.triangleIndex].hypertexture) {
          // find the back of the hypertexture
          std::vector<Triangle> toInspect = hypertextures;
          toInspect.erase(toInspect.begin() + closestIntersection.triangleIndex); // removing front face
          vec4 backPoint;
          vec4 frontPoint;
          Intersection backFaceIntersection;
          if (ClosestIntersection(cameraPos, d, toInspect, backFaceIntersection )) { // index is in toInspect
            frontPoint = closestIntersection.position;
            backPoint  = backFaceIntersection.position;
          }
          else { // case that camera is in bounding box, only one hypertexture intersection
            frontPoint = cameraPos;
            backPoint  = closestIntersection.position;
          }
          // find the next closest thing behind
          Intersection thingBehindIntersection;
          ClosestIntersection(cameraPos, d, nonHypertextures, thingBehindIntersection ); // index is in nonHypertextures

          // put ray marching here
          vector<vec4> layers;
          rayMarch(moveHypertexture, bottomLeft, layers, frontPoint, backPoint, lights, lightcolour, cameraPos, nonHypertextures, indirectLight, triangles[closestIntersection.triangleIndex].normal );
          // add back colour with light
          vec3 backColour = (DirectLight(nonHypertextures[thingBehindIntersection.triangleIndex].normal, thingBehindIntersection.position, lights, lightcolour, nonHypertextures, moveHypertexture, bottomLeft, cameraPos )
                + indirectLight)
                * nonHypertextures[thingBehindIntersection.triangleIndex].colour;
          layers.push_back(vec4(backColour.x, backColour.y, backColour.z, 1));
          colour = Composite(layers);
        }
      }
      PutPixelSDL(screen, x, y, colour);
    }
  }
}

// Function that adapts parameters based on user input (camera/light moving)
bool Update(vec4& cameraPos, mat4& move, mat4& moveHypertexture, float& yaw, vec4& T, vector<vec4>& lights) {

  // Compute frame time
  static int t = SDL_GetTicks();
  int t2 = SDL_GetTicks();
  t = t2;

  // Update variables based on input
  SDL_Event e;
  while(SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) return false;
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
            for (int i = 0; i < lights.size(); i++) {
              lights[i].z = lights[i].z+0.1;
            }
		        break;
          case SDLK_s: // Move light backwards
            for (int i = 0; i < lights.size(); i++) {
              lights[i].z = lights[i].z-0.1;
            }
		        break;
          case SDLK_d: // Move light right
            for (int i = 0; i < lights.size(); i++) {
              lights[i].x = lights[i].x+0.1;
            }
		        break;
          case SDLK_a: // Move light left
            for (int i = 0; i < lights.size(); i++) {
              lights[i].x = lights[i].x-0.1;
            }
		        break;
          case SDLK_q: // Move light up
            for (int i = 0; i < lights.size(); i++) {
              lights[i].y = lights[i].y+0.1;
            }
		        break;
          case SDLK_e: // Move light down
            for (int i = 0; i < lights.size(); i++) {
              lights[i].y = lights[i].y-0.1;
            }
            break;
          //ESC
	        case SDLK_ESCAPE: // Move camera quit
		        return false;
            break;
	      }
      }
    }

  TransformationMatrix(move,moveHypertexture, yaw,T);
  cameraPos = move * T;

  return true;
}

// Function to produce the move matrix, both translation and rotation
void TransformationMatrix(mat4& move, mat4& moveHypertexture, float& yaw, vec4& T) {
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

  std::vector<Triangle> nonHypertextures;
  std::vector<Triangle> hypertextures;
  vec4 bottomLeft;
  vec4 bottomRight;
  vec4 topLeft;
  vec4 bottomLeftBack;
  LoadTestModel(nonHypertextures,hypertextures,bottomLeft,bottomRight,topLeft,bottomLeftBack);

  // for translation of hyper
  vec4 bottom(-bottomLeft.x,-bottomLeft.y,-bottomLeft.z,1);
  mat4 hyperT(TcolOne,TcolTwo,TcolThree, bottom);

  // for rotation of hyper
  vec4 xaxis = bottomRight - bottomLeft;
  vec4 yaxis = topLeft - bottomLeft;
  vec4 zaxis = bottomLeftBack - bottomLeft;

  xaxis = glm::normalize(xaxis);
  yaxis = glm::normalize(yaxis);
  zaxis = glm::normalize(zaxis);
  vec4 RcolFour(0,0,0,1);

  mat4 hyperRotation(xaxis, yaxis, zaxis, RcolFour);


  // Updating camera position
  move =  Tforw * R * Tback; // rotate
  moveHypertexture = glm::inverse(hyperRotation) * hyperT;
}

// function that calculates how much light hits a spot taking into account hypertexture
vec3 DirectLight( vec4 normal, vec4 position, vector<vec4> lights, vec3 lightcolour, const vector<Triangle>& triangles, mat4& moveHypertexture, vec4 bottomLeft, vec4 cameraPos) {
// normal light-----------------------
  vec3 sumColour(0,0,0);
  for (int i = 0; i < lights.size(); i++) {
    // Find vector from triangle to lights
    vec4 rHat = lights[i] - position;
    float r = sqrt(pow(rHat.x,2) + pow(rHat.y,2) + pow(rHat.z,2)); // find dist
    rHat = glm::normalize(rHat);

    vec4 pos = position + rHat * 0.1f;
    vec3 colour(0,0,0);
    // Check for shadows
    Intersection shadowIntersection;
    if (ClosestIntersection(pos, rHat, triangles, shadowIntersection)) {
      // if there is something in the way there is a shadow
      if (shadowIntersection.distance < r && !(triangles[shadowIntersection.triangleIndex].hypertexture)) colour = vec3(0,0,0);
      // act differently depending if the closet intersection is a hypertexture
      else if (triangles[shadowIntersection.triangleIndex].hypertexture) {
        std::vector<Triangle> toInspect = triangles;
        toInspect.erase(toInspect.begin() + shadowIntersection.triangleIndex); // removing front face
        vec4 backPoint;
        vec4 frontPoint;
        Intersection backFaceIntersection;
        if (ClosestIntersection(pos, rHat, toInspect, backFaceIntersection )) { // index is in toInspect
          frontPoint = shadowIntersection.position;
          backPoint  = backFaceIntersection.position;
        }
        else { // case that camera is in bounding box, only one hypertexture intersection
          frontPoint = cameraPos;
          backPoint  = shadowIntersection.position;
        }
        float density = rayMarchDensity(moveHypertexture, bottomLeft, frontPoint, backPoint );
        colour = lightCalc(rHat, normal, lightcolour, r );
        colour = colour * vec3(1-density,1-density,1-density); // include density
      }
      else colour = lightCalc(rHat,normal , lightcolour, r ); // nothing in the way so calc light normally
    }
    else colour = lightCalc(rHat, normal, lightcolour, r ); // no intersection - so calc light normally
    sumColour = sumColour + colour;
  }

// fireball light-----------------------
  // light info stuff
  vec4 firelightPos( 0.3, 0.15, -0.7, 1.0 );
  vec3 firelightColour = 14.f * vec3( 1, 0, 0 );

  // Find vector from triangle to firelight
  vec4 rHatfire = firelightPos - position;
  float rfire = sqrt(pow(rHatfire.x,2) + pow(rHatfire.y,2) + pow(rHatfire.z,2)); // find dist
  rHatfire = glm::normalize(rHatfire);

  vec4 posfire = position + rHatfire * 0.1f;
  vec3 colourfire;
  // Check for shadows
  Intersection shadowIntersectionfire;
  if (ClosestIntersection(posfire, rHatfire, triangles, shadowIntersectionfire)) {
    // if there is something in the way there is a shadow
    if (shadowIntersectionfire.distance < rfire && !(triangles[shadowIntersectionfire.triangleIndex].hypertexture)) colourfire = vec3(0,0,0);
    else colourfire = lightCalc(rHatfire,normal , firelightColour, rfire ); // nothing in the way so calc light normally
  }
  else colourfire = lightCalc(rHatfire, normal, firelightColour, rfire ); // no intersection - so calc light normally

  return sumColour+colourfire;
}

// Function that calculates how much light hits a point from the source based on how far away it is
vec3 lightCalc(vec4 rHat, vec4 normal, vec3 lightcolour, float r ) {
  float dot = glm::dot(rHat,normal);
  float scalar = std::fmax(0, dot)* 1/(4 * M_PI * pow(r,2));
  return scalar * lightcolour;
}

// function that ray marches between the entry and exit points of the hypertexture bounding box totaling the density
float rayMarchDensity(mat4& moveHypertexture, vec4 bottomLeft, vec4 frontPoint, vec4 backPoint) {
    vector<vec4> points;
    Interpolate(frontPoint, backPoint, points);
    return lookUpDensity(moveHypertexture, bottomLeft, points);
}

// Function that maps points in world space to their hypertexture density
float lookUpDensity(mat4& moveHypertexture, vec4 bottomLeft, vector<vec4>& points) {
  float total = 0;
  for (int i = 0; i < points.size(); i++) {
    // convert to hyper space
    vec4 hyperPoint = moveHypertexture*points[i];
    // look up in hyper
    double density = fireball(hyperPoint.x,hyperPoint.y,hyperPoint.z);
    // add to total
    total = total + (float)density;
  }
  return total;
}

// Function that calculates the u,v and t needed to find the intersection point
vec3 returnItersectionParams(Triangle tri, vec3 s, vec3 d) {

  vec4 v0 = tri.v0;
  vec4 v1 = tri.v1;
  vec4 v2 = tri.v2;

  vec3 e1 = vec3(v1.x-v0.x,v1.y-v0.y,v1.z-v0.z); // v1-v0 (axis)
  vec3 e2 = vec3(v2.x-v0.x,v2.y-v0.y,v2.z-v0.z); // v2-v0 (axis)
  vec3 b = vec3(s.x-v0.x,s.y-v0.y,s.z-v0.z);     // constants in equation

  mat3 A( -d, e1, e2 );

  return glm::inverse( A ) * b;
}

// Function that takes point params and returns point
vec3 calculateIntersectionPoint(vec3 params, vec3 s, vec3 d) {
  return (s+(params.x * d));
}

// Function that finds if a ray intersects any triangles, returning true if it does
// Also returns information about the closest intersection via closestIntersection
bool ClosestIntersection(vec4 start, vec4 dir, const vector<Triangle>& triangles, Intersection& closestIntersection ) {
  closestIntersection.distance = std::numeric_limits<float>::max();
  bool intersection = false;
  for (int i = 0; i < triangles.size(); i++){

    // making vec3s for our lovely function
    vec3 s(start.x, start.y, start.z);
    vec3 d(dir.x, dir.y, dir.z);

    // is it an intersection?
    vec3 params = returnItersectionParams(triangles[i], s, d);
    if ( params.y >= 0
      && params.z >= 0
      && params.z + params.y <= 1
      && params.x >= 0) {
        // raising flag
        intersection = true;
        // maybe changing closest
        if (params.x < closestIntersection.distance) {
          closestIntersection.distance = params.x;
          vec3 pos = calculateIntersectionPoint(params, s, d);
          closestIntersection.position = vec4(pos.x, pos.y, pos.z, 1.0);
          closestIntersection.triangleIndex = i;
        }
      }
  }
  return intersection;
}

// function to composite a RGBA pixel across many layers
// first element of vector should be at the front
vec3 Composite(vector<vec4> layers) {
  vec3 colour(0,0,0);
  float totalTrans = 0;
  for (int k = 0; k<layers.size(); k++) {
    vec3 currentColour(layers[k].x, layers[k].y, layers[k].z);
    float transparency = layers[k].w * (1-totalTrans);
    colour = colour + transparency * currentColour;
    totalTrans = totalTrans + transparency;
  }

  return colour;
}

// function that gets a list of points evenly spread out between the entry and exit points
void Interpolate( vec4 a, vec4 b, vector<vec4>& result ) {

  vec4 step = (b-a) / float(max(RAY_MARCH_STEP-1,1));
  vec4 current = a;

  for( int i=0; i<RAY_MARCH_STEP; ++i ) {
    result.push_back(current);
    current = current + step;
  }
}

vec3 densityBasedColour(double density){
  float r = std::fmin(1.0, 0.9+(float)density);
  float g = 0.96;
  if (density < 0.1) g = std::fmin(0.96, (float)density);
  float b = 0;
  vec3 colour = vec3(r,g,b);

  return colour;
}

// Function that maps points in world space to their hypertexture RGBA
void lookUpColour(mat4& moveHypertexture, vec4 bottomLeft, vector<vec4>& points, vector<vec4>& layers, vector<vec4> lights, vec3 lightcolour, vec4 cameraPos, const vector<Triangle>& triangles, vec3 indirectLight, vec4 normal) {

  //normal calculation part1
  //get current ray direction
  vec4 rayDirection = points[1] - points[0];
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


  //used for normal calculation later
  vector<double>densityList;

  for (int i = 0; i < points.size(); i++) {
    // convert to hyper space
    vec4 hyperPoint = moveHypertexture*points[i];

    // look up in hyper
    double density = fireball(hyperPoint.x,hyperPoint.y,hyperPoint.z);
    densityList.push_back(density);

    // change colour based on density
    vec3 colour = densityBasedColour(density);

    if (NORMAL_FLAG){
      vec4 perpPoint1 = moveHypertexture*(points[i]+perpDirection1);
      vec4 perpPoint2 = moveHypertexture*(points[i]+perpDirection2);

      double perpDensity1 = fireball(perpPoint1.x,perpPoint1.y,perpPoint1.z);
      double perpDensity2 = fireball(perpPoint2.x,perpPoint2.y,perpPoint2.z);

      //evaluate change in fireball in three directions
      float densityChange1 = (float)perpDensity1 - (float)density;
      float densityChange2 = (float)perpDensity2 - (float)density;
      float densityChange3;
      if (i == 0){densityChange3 = 0;}
      else {densityChange3 = (float)density - (float)densityList[i-1];}

      //move to xyz space
      vec4 densityChange = densityChange1*perpDirection1 + densityChange2*perpDirection2 + densityChange3*rayDirection;

      //normalise
      vec4 newNormal = glm::normalize(densityChange);
      newNormal.w = 1;

      normal = newNormal;
    }
    // light calculation
    colour = (DirectLight(normal, points[i], lights, lightcolour, triangles, moveHypertexture, bottomLeft, cameraPos )
      + indirectLight)
      * colour;

    if (NORMAL_FLAG){
      colour = colour * vec3(2.2,2.2,2.2);
    }
    layers.push_back(vec4(colour.x,colour.y,colour.z,(float)density));
  }

}

// function that ray marches between the entry and exit points of the hypertexture bounding box
void rayMarch(mat4& moveHypertexture, vec4 bottomLeft, vector<vec4>& layers, vec4 frontPoint, vec4 backPoint, vector<vec4> lights, vec3 lightcolour, vec4 cameraPos, const vector<Triangle>& triangles, vec3 indirectLight, vec4 normal) {
    vector<vec4> points;
    Interpolate(frontPoint, backPoint, points);
    lookUpColour(moveHypertexture, bottomLeft, points, layers, lights, lightcolour, cameraPos, triangles, indirectLight, normal);
}
