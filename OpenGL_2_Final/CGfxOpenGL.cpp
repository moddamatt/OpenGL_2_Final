#ifdef _WINDOWS
#include <windows.h>
#endif

#include "glee.h"
#include <cmath>
#include "CGfxOpenGL.h"
#include <cstdio>


extern long windowHeight;
extern long windowWidth;

const char heightmapFilename[] = "heightmap.raw";


const float EYE_HEIGHT = 60.0f;
const float TERRAIN_HEIGHT_SCALE = 1.0f;
const float TERRAIN_MAX_HEIGHT = 256 * TERRAIN_HEIGHT_SCALE;
const float CREATURE_MIN_DISTANCE = 140.0f;
const float CREATURE_SCALE = TERRAIN_SCALE * 0.9f;
const float PI = 3.14159265359;

xyz_t eye = { 0, 0, 0 };

xyz_t Rhino = { 0, 0, 0 };
xyz_t SkyCam = { 0, 0, 0 };

// returns a number ranging from -1.0 to 1.0
#define FRAND()   (((float)rand()-(float)rand())/RAND_MAX)


bool SphereInFrustum(sphere_t sphere, frustum_t frustum)
{
  GLfloat dist;
  for (int i = 0; i < 6; ++i)
  {
    dist = frustum.planes[i].A * sphere.center.x +
           frustum.planes[i].B * sphere.center.y +
           frustum.planes[i].C * sphere.center.z +
           frustum.planes[i].D;

    if (dist <= -sphere.radius)
      return false;
  }

  return true;
}
CGfxOpenGL::CGfxOpenGL()
{
  m_terrain = NULL;
  m_terrainUVs = NULL;
  m_terrainIndices = NULL;
  m_terrainColors = NULL;
  m_angle = 0.0;
  myBulletAngle = 0.0;
  m_rotateRight = false;
  m_rotateLeft = false;
  m_useCulling = false;
  m_fmove = false;
  m_bmove = false;
  m_rstrafe = false;
  m_lstrafe = false;
  m_jump = false;
  m_crouch = false;
  m_bullet = false;
}
CGfxOpenGL::~CGfxOpenGL()
{
}
bool CGfxOpenGL::Init()
{	
  GLeeInit();
  
  glLineWidth(3.0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);

  InitializeTerrain();
  InitializeCreatures();
  InitializeFollowers();

  CTargaImage image;

  image.Load("grass.tga");
  glGenTextures(1, &m_grassTexture);
  glBindTexture(GL_TEXTURE_2D, m_grassTexture);
  gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, image.GetWidth(), image.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, image.GetImage());
  image.Release();

  m_skybox.Initialize(5.0);
  m_skybox.LoadTextures("skybox/up.tga", "skybox/dn.tga", "skybox/ft.tga", "skybox/bk.tga", "skybox/lf.tga", "skybox/rt.tga");
  return true;
}
bool CGfxOpenGL::Shutdown()
{
  delete [] m_terrain;
  m_terrain = NULL;
  delete [] m_terrainUVs;
  m_terrainUVs = NULL;
  delete [] m_terrainColors;
  m_terrainColors = NULL;
  delete [] m_terrainIndices;
  m_terrainIndices = NULL;
  return true;
  m_skybox.Release();
  glDeleteTextures(1, &m_grassTexture);
}
void CGfxOpenGL::SetupProjection(int width, int height)
{
	if (height == 0)					// don't want a divide by zero
	{
		height = 1;					
	}

	glViewport(0, 0, width, height);		// reset the viewport to new dimensions
	glMatrixMode(GL_PROJECTION);			// set projection matrix current matrix
	glLoadIdentity();						// reset projection matrix

	// calculate aspect ratio of window
	gluPerspective(45.0f, (GLfloat)width/(GLfloat)height, 2.0f, 3000.0f);

	glMatrixMode(GL_MODELVIEW);				// set modelview matrix
	glLoadIdentity();						// reset modelview matrix

	m_windowWidth = width;
	m_windowHeight = height;
}
void CGfxOpenGL::PrepareMonster(float dt)
{
	float edge = TERRAIN_SIZE / 2 * TERRAIN_SCALE;
	float padding = 200.0;

	//in this project the model is going to run no matter what.  So let's get him moving right off the bat...
	//save monster position
	float monster_Xsave = myMonsterX;
	float monster_Zsave = myMonsterZ;
	
	myBulletX = myMonsterX;
	myBulletZ = myMonsterZ;
	//calculate new position
	//move forward

	if (m_bullet)
	{
		myMonster.SetAnimation(CMD2Model::ATTACK);
	}

	if (m_fmove)
	{
		//change x and z for the camera
		myMonsterX += 4 * cos(DEG2RAD(m_angle));
		myMonsterZ += 4 * sin(DEG2RAD(m_angle));
		
		if (m_crouch)
			myMonster.SetAnimation(CMD2Model::CROUCH_WALK);
		if (m_jump)
			myMonster.SetAnimation(CMD2Model::JUMP);
		if (!m_crouch && !m_jump)
			myMonster.SetAnimation(CMD2Model::RUN);
	}

	if (m_bmove)
	{
		//change x and z for the camera
		myMonsterX -= 4 * cos(DEG2RAD(m_angle));
		myMonsterZ -= 4 * sin(DEG2RAD(m_angle));

		if (m_crouch)
			myMonster.SetAnimation(CMD2Model::CROUCH_WALK);
		if (m_jump)
			myMonster.SetAnimation(CMD2Model::JUMP);
		if (!m_crouch && !m_jump)
			myMonster.SetAnimation(CMD2Model::RUN);
	}

	if (m_rstrafe)
	{
		myMonsterX += 4 * cos((DEG2RAD(m_angle) + PI / 2));
		myMonsterZ += 4 * sin((DEG2RAD(m_angle) + PI / 2));

		if (m_crouch)
			myMonster.SetAnimation(CMD2Model::CROUCH_WALK);
		if (m_jump)
			myMonster.SetAnimation(CMD2Model::JUMP);
		if (!m_crouch && !m_jump)
			myMonster.SetAnimation(CMD2Model::RUN);
	}

	if (m_lstrafe)
	{
		myMonsterX -= 4 * cos((DEG2RAD(m_angle) + PI / 2));
		myMonsterZ -= 4 * sin((DEG2RAD(m_angle) + PI / 2));

		if (m_crouch)
			myMonster.SetAnimation(CMD2Model::CROUCH_WALK);
		if (m_jump)
			myMonster.SetAnimation(CMD2Model::JUMP);
		if (!m_crouch && !m_jump)
			myMonster.SetAnimation(CMD2Model::RUN);
	}

	//if monster excedes the x edge
	if (myMonsterX >= (edge - padding) || myMonsterX <= (-edge + padding))
	{
		//move monster back to saved position
		myMonsterX = monster_Xsave;
		myMonsterZ = monster_Zsave;

	}
	else if (myMonsterZ >= (edge - padding) || myMonsterZ <= (-edge + padding))
	{
		//move monster back to saved position
		myMonsterZ = monster_Zsave;
		myMonsterX = monster_Xsave;

	}

	//jump
	if (m_jump) //we are jumping
	{

		if (jumpcount > 20)  //if jumpcount exceeds 20
			jumpfactor = -1; //then jumpfactor becomes negative

		jumpcount++;

		//move the camera up or down
		myMonsterY += jumpfactor; //increment y by jumpfactor	
		eye.y = myMonsterY;		
		myMonster.SetAnimation(CMD2Model::JUMP);
		//this code is for stopping the jump
		GLfloat ytemp = GetHeightAt(myMonsterX, myMonsterZ)+EYE_HEIGHT;
		if (myMonsterY < ytemp) //if the camera has gone too low
		{
			m_jump = false; //stop jumping
			myMonsterY = GetHeightAt(myMonsterX, myMonsterZ) + EYE_HEIGHT;
		}
		
		//we're not jumping, so just set the normal eye
		//height at the given x, z of the terrain
	
	}

	if (!m_fmove && !m_bmove && !m_lstrafe && !m_rstrafe && !m_jump && !m_crouch && !m_bullet)
		myMonster.SetAnimation(CMD2Model::IDLE);
	else if (!m_fmove && !m_bmove && !m_lstrafe && !m_rstrafe && m_crouch)
		myMonster.SetAnimation(CMD2Model::CROUCH_IDLE);


	//if monster excedes the x edge

	myMonster.Rotate(-myMonsterAngle);
	myMonster.Move(myMonsterX, GetHeightAt(myMonsterX, myMonsterZ), myMonsterZ);

	// animate the models
	myMonster.Animate(dt);
}
void CGfxOpenGL::Prepare(float dt)
{

	PrepareMonster(dt);
	m_rhinoModel.Animate(dt);
	m_demonModel.Animate(dt);

	//position the camera so that it is directly behind the monster
	eye.x = myMonsterX + -(cos(DEG2RAD(myMonsterAngle)) * 100);
	eye.y = GetHeightAt(eye.x, eye.z) + EYE_HEIGHT;
	eye.z = myMonsterZ + -(sin(DEG2RAD(myMonsterAngle)) * 100);

	//Rhinos Camera
	Rhino.x = m_rhinos[0][0] + -(cos(DEG2RAD(m_rhinos[0][3])) * 100);
	Rhino.y = GetHeightAt(m_rhinos[0][0], m_rhinos[0][2]) + EYE_HEIGHT;
	Rhino.z = m_rhinos[0][2] + -(sin(DEG2RAD(m_rhinos[0][3])) * 100);

	//SkyCam
	SkyCam.x = sin(DEG2RAD(m_angle)) * TERRAIN_SIZE / 2.0f;
	SkyCam.y = 1000;
	SkyCam.z = cos(DEG2RAD(m_angle)) * TERRAIN_SIZE / 2.0f;

	m_angle = myMonsterAngle;
	

}
void CGfxOpenGL::Render()
{
  glClearColor(0.7f, 0.7f, 0.9f, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);;

  // set up the view
  glLoadIdentity();
  
  eye.y = GetHeightAt(eye.x, eye.z) + EYE_HEIGHT;
  glPushMatrix();
  gluLookAt(eye.x, eye.y, eye.z, myMonsterX, eye.y, myMonsterZ, 0.0, 1.0, 0.0);
  //Fog Color
  GLfloat fogColor[] = { 0.0f, 0.0f, 0.0f };
  glFogfv(GL_FOG_COLOR, fogColor); 
	  
  //First view port for third person monster with fog
  glEnable(GL_FOG);
  glFogi(GL_FOG_MODE, GL_EXP); //changes the fog mode to exponetial
  glFogf(GL_FOG_DENSITY, 0.005f); //set the fog density
  glViewport(0, 0, m_windowWidth, m_windowHeight);

  m_skybox.Render(eye.x, eye.y, eye.z);
  DrawTerrain();
  myMonster.Render();
  DrawFollowers();
  //Turn fog of for other viewports
  glDisable(GL_FOG);
  glPopMatrix();

  //glPushMatrix();
  ////Follows the rhino, second viewport
  //glViewport((m_windowWidth / 2), m_windowHeight / 2, m_windowWidth / 2, m_windowHeight / 2);
  //gluLookAt(Rhino.x, Rhino.y, Rhino.z, m_rhinos[0][0], Rhino.y, m_rhinos[0][2], 0.0, 1.0, 0.0);
  //m_skybox.Render(Rhino.x, Rhino.y, Rhino.z);
  //DrawTerrain();
  //myMonster.Render();
  //DrawFollowers();
  //glPopMatrix();

  //glPushMatrix();
  ////Third viewport
  //GLfloat fogColor1[] = { 0.5f, 0.0f, 0.0f };
  //glFogfv(GL_FOG_COLOR, fogColor1);

  ////First view port for third person monster with fog
  //glEnable(GL_FOG);
  //glFogi(GL_FOG_MODE, GL_EXP); //changes the fog mode to exponetial
  //glFogf(GL_FOG_DENSITY, 0.0005f); //set the fog density
  //glViewport((m_windowWidth / 2), 0, m_windowWidth / 2, m_windowHeight / 2);
  //gluLookAt(SkyCam.x, SkyCam.y, SkyCam.z, myMonsterX, 0, myMonsterZ, 0.0, 1.0, 0.0);
  //m_skybox.Render(SkyCam.x, SkyCam.y, SkyCam.z);
  //DrawTerrain();
  //myMonster.Render();
  //DrawFollowers();
  //glPopMatrix();


}
bool CGfxOpenGL::InitializeTerrain()
{
  GLubyte *heightmap = new GLubyte[TERRAIN_SIZE * TERRAIN_SIZE];
  FILE *pFile = fopen(heightmapFilename, "rb");
  if (!pFile)
    return false;

  fread(heightmap, TERRAIN_SIZE * TERRAIN_SIZE, 1, pFile);
  fclose(pFile);

  m_terrain = new GLfloat[TERRAIN_SIZE * TERRAIN_SIZE * 3];
  m_terrainUVs = new GLfloat[TERRAIN_SIZE * TERRAIN_SIZE * 2];
  m_terrainColors = new GLfloat[TERRAIN_SIZE * TERRAIN_SIZE * 3];
  m_terrainIndices = new GLuint[TERRAIN_SIZE * 2 * (TERRAIN_SIZE -1)];

  int xyz, uv, rgb;
  int index = 0;
  for (int z = 0; z < TERRAIN_SIZE; ++z)
  {
    for (int x = 0; x < TERRAIN_SIZE; ++x)
    {
      xyz = rgb = 3 * (z * TERRAIN_SIZE + x);
      uv = 2 * (z * TERRAIN_SIZE + x);

      GLfloat height = (GLfloat)heightmap[z * TERRAIN_SIZE + x] * TERRAIN_HEIGHT_SCALE;
      m_terrain[xyz] = (GLfloat)(x - (GLfloat)TERRAIN_SIZE/2) * TERRAIN_SCALE;
      m_terrain[xyz+1] = height;
      m_terrain[xyz+2] = (GLfloat)(z - (GLfloat)TERRAIN_SIZE/2) * TERRAIN_SCALE;

      float color = 0.5f + 0.5f * height / TERRAIN_MAX_HEIGHT;

      m_terrainColors[rgb] = m_terrainColors[rgb+1] = m_terrainColors[rgb+2] = color;

      m_terrainUVs[uv] = (GLfloat)x/TERRAIN_SIZE*32;
      m_terrainUVs[uv+1] = (GLfloat)z/TERRAIN_SIZE*32;

      if (z < TERRAIN_SIZE - 1)
      {
        m_terrainIndices[index++] = TERRAIN_SIZE * (z + 1) + x;
        m_terrainIndices[index++] = TERRAIN_SIZE * z + x;
      }
    }
  }
  return true;
}
void CGfxOpenGL::DrawTerrain()
{
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, m_terrain);
  glTexCoordPointer(2, GL_FLOAT, 0, m_terrainUVs);
  glColorPointer(3, GL_FLOAT, 0, m_terrainColors);

  glBindTexture(GL_TEXTURE_2D, m_grassTexture);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  for (int i = 0; i < TERRAIN_SIZE - 1; ++i)
  {
    glDrawElements(GL_TRIANGLE_STRIP, TERRAIN_SIZE * 2, GL_UNSIGNED_INT, &m_terrainIndices[i * TERRAIN_SIZE * 2]);
  }


  /*
  // to render the entire terrain with a single OpenGL call, you could use the following
  // code during initialization to set up arrays for use with glMultiDrawElements()
  GLsizei count[TERRAIN_SIZE-1];
  GLuint *indices[TERRAIN_SIZE-1];

  for (int i = 0; i < TERRAIN_SIZE - 1; ++i)
  {
    count[i] = TERRAIN_SIZE * 2;
    indices[i] = &m_terrainIndices[i * TERRAIN_SIZE * 2];
  }

  // this would be the only line you would need during rendering
  glMultiDrawElements(GL_TRIANGLE_STRIP, count, GL_UNSIGNED_INT, (const void**)indices, TERRAIN_SIZE-1);
  */

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
}
bool CGfxOpenGL::InitializeCreatures()
{
	myMonster.Load("goblin\\tris.md2", "goblin\\goblin_white.tga");
  
	//monster will be in center of terrain at (0, 0)
	myMonsterX = 0.0f;
	myMonsterY = GetHeightAt(0, 0);
	myMonsterZ = 0.0f;
	myMonsterAngle = 0.0;

	//set monster animation to run
	myMonster.SetAnimation(CMD2Model::IDLE);

	myMonster.Rotate(myMonsterAngle);
	myMonster.Move(myMonsterX, myMonsterY, myMonsterZ);

	return true;
}
bool CGfxOpenGL::InitializeFollowers()
{
	srand(27);

	m_rhinoModel.Load("rhino\\tris.md2", "rhino\\rhino.tga");
	m_demonModel.Load("necromicus\\tris.md2", "necromicus\\necromicus.tga");

	for (int i = 0; i < NUM_CREATURES; ++i)
	{
		GLfloat x, z;
		do {
			x = FRAND() * (float)TERRAIN_SIZE / 2 * CREATURE_SCALE;
			z = FRAND() * (float)TERRAIN_SIZE / 2 * CREATURE_SCALE;
		} while (x * x + z * z < CREATURE_MIN_DISTANCE * CREATURE_MIN_DISTANCE);

		m_rhinos[i][0] = x;
		m_rhinos[i][1] = GetHeightAt(x, z);
		m_rhinos[i][2] = z;
		m_rhinos[i][3] = FRAND() * DEG2RAD(360.0f);

		do {
			x = FRAND() * (float)TERRAIN_SIZE / 2 * CREATURE_SCALE;
			z = FRAND() * (float)TERRAIN_SIZE / 2 * CREATURE_SCALE;
		} while (x * x + z * z < CREATURE_MIN_DISTANCE * CREATURE_MIN_DISTANCE);

		m_demons[i][0] = x;
		m_demons[i][1] = GetHeightAt(x, z);
		m_demons[i][2] = z;
		m_demons[i][3] = FRAND() * DEG2RAD(360.0f);
		}

	m_demonModel.SetAnimation(CMD2Model::RUN);
	m_rhinoModel.SetAnimation(CMD2Model::RUN);

	
	return true;
}
void CGfxOpenGL::DrawCreatures()
{
  // iterate through all the creatures, checking to see if the creature's bounding sphere
  // intersects the view volume when frustum culling is enabled

	//myMonster.Rotate(myMonsterAngle);
	//myMonster.Move(myMonsterX, myMonsterY, myMonsterZ);
	//myMonster.Render();

}
void CGfxOpenGL::DrawFollowers()
{
	float edge = TERRAIN_SIZE / 2 * TERRAIN_SCALE;
	float padding = 200.0;

	// iterate through all the creatures, checking to see if the creature's bounding sphere
	// intersects the view volume when frustum culling is enabled
	for (int i = 0; i < NUM_CREATURES; ++i)
	{
		float rhinos_Xsave = m_rhinos[i][0];
		float rhinos_Zsave = m_rhinos[i][2];
		float demons_Xsave = m_demons[i][0];
		float demons_Zsave = m_demons[i][2];

		m_rhinoModel.Rotate(m_rhinos[i][3]);
		m_rhinoModel.Move(m_rhinos[i][0] += 4 * cos(DEG2RAD(m_rhinos[i][3])), GetHeightAt(m_rhinos[i][0], m_rhinos[i][2]), m_rhinos[i][2] += 4 * sin(DEG2RAD(m_rhinos[i][3])));
		if (m_rhinos[i][0] >= (edge - padding) || m_rhinos[i][0] <= (-edge + padding))
		{
			//move monster back to saved position
			m_rhinos[i][0] = rhinos_Xsave;
			m_rhinos[i][2] = rhinos_Zsave;
			m_rhinos[i][3] = FRAND() * 130.0f;

		}
		else if (m_rhinos[i][2] >= (edge - padding) || m_rhinos[i][2] <= (-edge + padding))
		{
			//move monster back to saved position
			m_rhinos[i][0] = rhinos_Xsave;
			m_rhinos[i][2] = rhinos_Zsave;
			m_rhinos[i][3] = FRAND() * 130.0f;

		}
		if (!m_useCulling || SphereInFrustum(m_rhinoModel.GetBoundingSphere(), m_frustum))
		{
			m_rhinoModel.Render();
		}
		m_demonModel.Rotate(m_demons[i][3]);
		m_demonModel.Move(m_demons[i][0] += 4 * cos(DEG2RAD(m_demons[i][3])), GetHeightAt(m_demons[i][0], m_demons[i][2]), m_demons[i][2] += 4 * sin(DEG2RAD(m_demons[i][3])));
		if (m_demons[i][0] >= (edge - padding) || m_demons[i][0] <= (-edge + padding))
		{
			//move monster back to saved position
			m_demons[i][0] = demons_Xsave;
			m_demons[i][2] = demons_Zsave;
			m_demons[i][3] = FRAND() * 130.0f;

		}
		else if (m_demons[i][2] >= (edge - padding) || m_demons[i][2] <= (-edge + padding))
		{
			//move monster back to saved position
			m_demons[i][0] = demons_Xsave;
			m_demons[i][2] = demons_Zsave;
			m_demons[i][3] = FRAND() * 130.0f;
		}
		if (!m_useCulling || SphereInFrustum(m_demonModel.GetBoundingSphere(), m_frustum))
		{
			m_demonModel.Render();
		}
	}
}
// utility function to return the height at a given terrain location
GLfloat CGfxOpenGL::GetHeightAt(GLfloat x, GLfloat z)
{
  float scaledX = x / TERRAIN_SCALE + (float)TERRAIN_SIZE/2;
  float scaledZ = z / TERRAIN_SCALE + (float)TERRAIN_SIZE/2;

  int x0 = (int)scaledX;
  if (scaledX < 0)
    --x0;
  int z0 = (int)scaledZ;
  if (scaledZ < 0)
    --z0;
  int p0, p1, p2, p3;

  p0 = (z0 * TERRAIN_SIZE + x0) * 3 + 1;
  p1 = (z0 * TERRAIN_SIZE + x0 + 1) * 3 + 1;
  p2 = ((z0 + 1) * TERRAIN_SIZE + x0) * 3 + 1;
  p3 = ((z0 + 1) * TERRAIN_SIZE + x0 + 1) * 3 + 1;

  float fracX = scaledX - x0;
  float fracZ = scaledZ - z0;

  float xInterp0 = m_terrain[p0] + fracX * (m_terrain[p1] - m_terrain[p0]);
  float xInterp1 = m_terrain[p2] + fracX * (m_terrain[p3] - m_terrain[p2]);

  return xInterp0 + fracZ * (xInterp1 - xInterp0);
}
// extract a plane from a given matrix and row id
void ExtractPlane(plane_t &plane, GLfloat *mat, int row)
{
  int scale = (row < 0) ? -1 : 1;
  row = abs(row) - 1;

  // calculate plane coefficients from the matrix
  plane.A = mat[3] + scale * mat[row];
  plane.B = mat[7] + scale * mat[row + 4];
  plane.C = mat[11] + scale * mat[row + 8];
  plane.D = mat[15] + scale * mat[row + 12];

  // normalize the plane
  float length = sqrtf(plane.A * plane.A + plane.B * plane.B + plane.C * plane.C);
  plane.A /= length;
  plane.B /= length;
  plane.C /= length;
  plane.D /= length;
}
// determines the current view frustum
void CGfxOpenGL::CalculateFrustum()
{
  // get the projection and modelview matrices
  GLfloat projection[16];
  GLfloat modelview[16];

  glGetFloatv(GL_PROJECTION_MATRIX, projection);
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

  // use OpenGL to multiply them
  glPushMatrix();
  glLoadMatrixf(projection);
  glMultMatrixf(modelview);
  glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
  glPopMatrix();

  // extract each plane
  ExtractPlane(m_frustum.l, modelview, 1);
  ExtractPlane(m_frustum.r, modelview, -1);
  ExtractPlane(m_frustum.b, modelview, 2);
  ExtractPlane(m_frustum.t, modelview, -2);
  ExtractPlane(m_frustum.n, modelview, 3);
  ExtractPlane(m_frustum.f, modelview, -3);
}

void CGfxOpenGL::DrawCube()
{

	glBegin(GL_QUADS);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, 1.0f);

	glVertex3f(1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);

	glVertex3f(1.0f, 1.0f, 1.0f);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);

	glVertex3f(1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);

	glVertex3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);

	glVertex3f(1.0f, 1.0f, -1.0f);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();

}