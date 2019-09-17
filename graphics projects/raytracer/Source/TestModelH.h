#ifndef TEST_MODEL_CORNEL_BOX_H
#define TEST_MODEL_CORNEL_BOX_H

// Defines a simple test model: The Cornel Box

#include <glm/glm.hpp>
#include <vector>

/* ----------------------------------------------------------------------------*/
/* GLM things                                                              */

// GLM names
using namespace std;
using glm::vec3;
using glm::mat3;
using glm::vec4;
using glm::mat4;

// function to print vec3s
std::ostream& operator<<(std::ostream& out, vec3 v) {
  return out << "(" << v.x
             << "," << v.y
             << "," << v.z << ")";
}

// function to print vec4s
std::ostream& operator<<(std::ostream& out, vec4 v) {
  return out << "(" << v.x
             << "," << v.y
             << "," << v.z
             << "," << v.w << ")";
}
         
/* ----------------------------------------------------------------------------*/

// Used to describe a triangular surface:
class Triangle
{
public:
	glm::vec4 v0;
	glm::vec4 v1;
	glm::vec4 v2;
	glm::vec4 normal;
	glm::vec3 colour;
	bool hypertexture;

	Triangle( glm::vec4 v0, glm::vec4 v1, glm::vec4 v2, glm::vec3 colour, bool hypertexture )
		: v0(v0), v1(v1), v2(v2), colour(colour), hypertexture(hypertexture)
	{
		ComputeNormal();
	}

	void ComputeNormal()
	{
	  glm::vec3 e1 = glm::vec3(v1.x-v0.x,v1.y-v0.y,v1.z-v0.z);
	  glm::vec3 e2 = glm::vec3(v2.x-v0.x,v2.y-v0.y,v2.z-v0.z);
	  glm::vec3 normal3 = glm::normalize( glm::cross( e2, e1 ) );
	  normal.x = normal3.x;
	  normal.y = normal3.y;
	  normal.z = normal3.z;
	  normal.w = 1.0;
	}
};

struct Intersection
   {
       glm::vec4 position;
       float distance;
       int triangleIndex;
};

glm::vec3 black(  0,     0,     0     );

// Loads the Cornell Box. It is scaled to fill the volume:
// -1 <= x <= +1
// -1 <= y <= +1
// -1 <= z <= +1
// Assumes there is nly one hypertexture with a bottom left corner at bottomLeft
void LoadTestModel( std::vector<Triangle>& triangles, std::vector<Triangle>& hypertextures, glm::vec4& bottomLeft, glm::vec4& bottomRight, glm::vec4& topLeft, glm::vec4& bottomLeftBack )
{
	using glm::vec3;
	using glm::vec4;

	// Defines colours:
	vec3 red(    0.75f, 0.15f, 0.15f );
	vec3 yellow( 0.75f, 0.75f, 0.15f );
	vec3 green(  0.15f, 0.75f, 0.15f );
	vec3 cyan(   0.15f, 0.75f, 0.75f );
	vec3 blue(   0.15f, 0.15f, 0.75f );
	vec3 purple( 0.75f, 0.15f, 0.75f );
	vec3 white(  0.75f, 0.75f, 0.75f );
	vec3 gray (  0.5f, 0.5f, 0.5f );
	vec3 black(  0,     0,     0     );

	triangles.clear();
	triangles.reserve( 5*2*3 );

	hypertextures.clear();
	hypertextures.reserve( 5*2*3 );

	// ---------------------------------------------------------------------------
	// Room

	float L = 555;			// Length of Cornell Box side.

	vec4 A(L,0,0,1);
	vec4 B(0,0,0,1);
	vec4 C(L,0,L,1);
	vec4 D(0,0,L,1);

	vec4 E(L,L,0,1);
	vec4 F(0,L,0,1);
	vec4 G(L,L,L,1);
	vec4 H(0,L,L,1);

	// Floor:
	triangles.push_back( Triangle( C, B, A, green, false ) );
	triangles.push_back( Triangle( C, D, B, green, false ) );

	// Left wall
	triangles.push_back( Triangle( A, E, C, purple, false ) );
	triangles.push_back( Triangle( C, E, G, purple, false ) );

	// Right wall
	triangles.push_back( Triangle( F, B, D, yellow, false ) );
	triangles.push_back( Triangle( H, F, D, yellow, false ) );

	// Ceiling
	triangles.push_back( Triangle( E, F, G, cyan, false ) );
	triangles.push_back( Triangle( F, H, G, cyan, false ) );

	// Back wall
	triangles.push_back( Triangle( G, D, C, white, false ) );
	triangles.push_back( Triangle( G, H, D, white, false ) );

	// ---------------------------------------------------------------------------
	// Short block

	A = vec4(290,0,114,1);
	B = vec4(130,0, 65,1);
	C = vec4(240,0,272,1);
	D = vec4( 82,0,225,1);
	       
	E = vec4(290,165,114,1);
	F = vec4(130,165, 65,1);
	G = vec4(240,165,272,1);
	H = vec4( 82,165,225,1);

	// Front
	triangles.push_back( Triangle(E,B,A,red, false) );  // bottom left
	triangles.push_back( Triangle(E,F,B,red, false) ); // top right

	// Right
	triangles.push_back( Triangle(F,D,B,red, false) );
	triangles.push_back( Triangle(F,H,D,red, false) );

	// BACK
	triangles.push_back( Triangle(H,C,D,red, false) );
	triangles.push_back( Triangle(H,G,C,red, false) ); // top right, viewed from the rear

	// LEFT
	triangles.push_back( Triangle(G,E,C,red, false) );
	triangles.push_back( Triangle(E,A,C,red, false) );

	// TOP
	triangles.push_back( Triangle(G,F,E,red, false) );
	triangles.push_back( Triangle(G,H,F,red, false) );

	// ---------------------------------------------------------------------------
	// Hyper bounding Box

	A = vec4(290,165,114,1);
	B = vec4(130,165, 65,1);
	C = vec4(240,165,272,1);
	D = vec4( 82,165,225,1);
	       
	E = vec4(290,350,114,1);
	F = vec4(130,350, 65,1);
	G = vec4(240,350,272,1);
	H = vec4( 82,350,225,1);

	bottomLeft = A;
	bottomRight = B;
	topLeft = E;
	bottomLeftBack = C;

	// Front
	hypertextures.push_back( Triangle(E,B,A,purple, true) );
	hypertextures.push_back( Triangle(E,F,B,purple, true) );

	// Right
	hypertextures.push_back( Triangle(F,D,B,purple, true) );
	hypertextures.push_back( Triangle(F,H,D,purple, true) );

	// BACK
	hypertextures.push_back( Triangle(H,C,D,purple, true) );
	hypertextures.push_back( Triangle(H,G,C,purple, true) );

	// LEFT
	hypertextures.push_back( Triangle(G,E,C,purple, true) );
	hypertextures.push_back( Triangle(E,A,C,purple, true) );

	// TOP
	hypertextures.push_back( Triangle(G,F,E,purple, true) );
	hypertextures.push_back( Triangle(G,H,F,purple, true) );

	// ---------------------------------------------------------------------------
	// Tall block

	A = vec4(423,0,247,1);
	B = vec4(265,0,296,1);
	C = vec4(472,0,406,1);
	D = vec4(314,0,456,1);
	       
	E = vec4(423,330,247,1);
	F = vec4(265,330,296,1);
	G = vec4(472,330,406,1);
	H = vec4(314,330,456,1);

	// Front
	triangles.push_back( Triangle(E,B,A,blue, false) );
	triangles.push_back( Triangle(E,F,B,blue, false) );

	// Front
	triangles.push_back( Triangle(F,D,B,blue, false) );
	triangles.push_back( Triangle(F,H,D,blue, false) );

	// BACK
	triangles.push_back( Triangle(H,C,D,blue, false) );
	triangles.push_back( Triangle(H,G,C,blue, false) );

	// LEFT
	triangles.push_back( Triangle(G,E,C,blue, false) );
	triangles.push_back( Triangle(E,A,C,blue, false) );

	// TOP
	triangles.push_back( Triangle(G,F,E,blue, false) );
	triangles.push_back( Triangle(G,H,F,blue, false) );


	// ----------------------------------------------
	// Scale to the volume [-1,1]^3

	for( size_t i=0; i<triangles.size(); ++i )
	{
		triangles[i].v0 *= 2/L;
		triangles[i].v1 *= 2/L;
		triangles[i].v2 *= 2/L;

		triangles[i].v0 -= vec4(1,1,1,1);
		triangles[i].v1 -= vec4(1,1,1,1);
		triangles[i].v2 -= vec4(1,1,1,1);

		triangles[i].v0.x *= -1;
		triangles[i].v1.x *= -1;
		triangles[i].v2.x *= -1;

		triangles[i].v0.y *= -1;
		triangles[i].v1.y *= -1;
		triangles[i].v2.y *= -1;

		triangles[i].v0.w = 1.0;
		triangles[i].v1.w = 1.0;
		triangles[i].v2.w = 1.0;
		
		triangles[i].ComputeNormal();
	}

	for( size_t i=0; i<hypertextures.size(); ++i )
	{
		hypertextures[i].v0 *= 2/L;
		hypertextures[i].v1 *= 2/L;
		hypertextures[i].v2 *= 2/L;

		hypertextures[i].v0 -= vec4(1,1,1,1);
		hypertextures[i].v1 -= vec4(1,1,1,1);
		hypertextures[i].v2 -= vec4(1,1,1,1);

		hypertextures[i].v0.x *= -1;
		hypertextures[i].v1.x *= -1;
		hypertextures[i].v2.x *= -1;

		hypertextures[i].v0.y *= -1;
		hypertextures[i].v1.y *= -1;
		hypertextures[i].v2.y *= -1;

		hypertextures[i].v0.w = 1.0;
		hypertextures[i].v1.w = 1.0;
		hypertextures[i].v2.w = 1.0;
		
		hypertextures[i].ComputeNormal();
	}

	bottomLeft = bottomLeft* (2/L);
	bottomLeft = bottomLeft - vec4(1,1,1,1);
	bottomLeft.x = bottomLeft.x * -1;
	bottomLeft.y = bottomLeft.y * -1;
	bottomLeft.w = 1;

	bottomRight = bottomRight* (2/L);
	bottomRight = bottomRight - vec4(1,1,1,1);
	bottomRight.x = bottomRight.x * -1;
	bottomRight.y = bottomRight.y * -1;
	bottomRight.w = 1;

	topLeft = topLeft* (2/L);
	topLeft = topLeft - vec4(1,1,1,1);
	topLeft.x = topLeft.x * -1;
	topLeft.y = topLeft.y * -1;
	topLeft.w = 1;

	bottomLeftBack = bottomLeftBack* (2/L);
	bottomLeftBack = bottomLeftBack - vec4(1,1,1,1);
	bottomLeftBack.x = bottomLeftBack.x * -1;
	bottomLeftBack.y = bottomLeftBack.y * -1;
	bottomLeftBack.w = 1;

}


#endif
