#ifndef __GL_COMPONENT
#define __GL_COMPONENT

#include "md2.h"
#include "skybox.h"

const int TERRAIN_SIZE = 65;
const float TERRAIN_SCALE = 64.0;
const float ROTATION_SPEED = 50.0f;

const int NUM_CREATURES = 30;

#define DEG2RAD(x) (x * PI) / 180.0f

struct plane_t
{
  GLfloat A, B, C, D;
};

union frustum_t
{
  struct
  {
    plane_t t, b, l, r, n, f;
  };
  plane_t planes[6];
};

class CGfxOpenGL
{
public:
	CGfxOpenGL();
	virtual ~CGfxOpenGL();

	bool Init();
	bool Shutdown();
	void SetupProjection(int width, int height);
	void PrepareMonster(float dt);
	void Prepare(float dt);
	void Render();
	void DrawCube();

  void BeginRotateRight() { m_rotateRight = true; }
  void BeginRotateLeft() { m_rotateLeft = true; }

  void EndRotateRight() { m_rotateRight = false; }
  void EndRotateLeft() { m_rotateLeft = false; }

  void BeginMoveForwardPos(){ m_fmove = true; }
  void EndMoveForwardPos(){ m_fmove = false; }

  void BeginMoveBackwardPos(){ m_bmove = true; }
  void EndMoveBackwardPos(){ m_bmove = false; }

  void BeginRightStrafePos(){ m_rstrafe = true; }
  void EndRightStrafePos(){ m_rstrafe = false; }

  void BeginLeftStrafePos(){ m_lstrafe = true; }
  void EndLeftStrafePos(){ m_lstrafe = false; }

  bool ToggleCrouch(){ m_crouch = !m_crouch; return m_crouch; }

  void startBullet(){ m_bullet = true; }
  void endBullet(){ m_bullet = false; }

  // Camera with mouse
  void MoveCameraRight() { myMonsterAngle += 3.0f; }
  void MoveCameraLeft() { myMonsterAngle -= 3.0f; }
  //void MoveCameraUp() { m_height += 3.0f; }
  //void MoveCameraDown() { m_height -= 3.0f; }


  bool ToggleCulling() { m_useCulling = !m_useCulling; return m_useCulling; }


  void SetJumpState()
  {
	  if (m_jump == false) //if not jumping
	  {
		  m_jump = true;  //set to jumping
		  jumpcount = 0;
		  jumpfactor = 1;
	  }
  }

private:
  void CalculateFrustum();
  bool InitializeTerrain();
	void DrawTerrain();

  bool InitializeCreatures();
  void DrawCreatures();
  GLfloat GetHeightAt(GLfloat x, GLfloat z);

  bool InitializeFollowers();
  void DrawFollowers();

  int m_windowWidth;
  int m_windowHeight;

  GLuint m_grassTexture;

  GLubyte m_heightmap[TERRAIN_SIZE * TERRAIN_SIZE];


  GLfloat *m_terrain;
  GLfloat *m_terrainUVs;
  GLfloat *m_terrainColors;
  GLuint *m_terrainIndices;

  CMD2Model m_rhinoModel;
  GLfloat m_rhinos[NUM_CREATURES][4];
  CMD2Model m_demonModel;
  GLfloat m_demons[NUM_CREATURES][4];

  CMD2Model myMonster;
  GLfloat myMonsterX, myMonsterY, myMonsterZ, myMonsterAngle, myBulletX, myBulletZ, myBulletAngle;

  GLfloat m_angle;
  bool m_rotateRight;
  bool m_rotateLeft;
  bool m_fmove; //forward movement
  bool m_bmove; //backward movement
  bool m_rstrafe; //right strafe movement
  bool m_lstrafe; //left strafe movement
  bool m_jump;
  bool m_crouch;
  bool m_bullet;
  int jumpcount;
  GLfloat jumpfactor;



  CSkybox m_skybox;

  frustum_t m_frustum;
  bool m_useCulling;
};

#endif