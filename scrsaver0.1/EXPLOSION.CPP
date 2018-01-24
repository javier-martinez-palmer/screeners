/****************************************************************************
 Particles.h  Particle and particle system base classes.
 
 Author   :   Javier Palmer
 Date     :   20/2/2005

*****************************************************************************/


/********************************* Includes *********************************/
#include "explosion.h"


/*****************************************************************************
 CExplosion::Constructor

 Nothing to do
*****************************************************************************/
CExplosion::CExplosion(int numParticles, CVector origin, float spread, GLuint texture, float r, float g, float b)
  : m_texture(texture), m_spread(spread), CParticleSystem(numParticles, origin)
{
//srand(GetTickCount());
  CParticleSystem::InitializeSystem();
  Emit(numParticles,r,g,b);
} // end CExplosion::Constructor


/*****************************************************************************
 CExplosion::InitializeParticle()

 Sets the initial particle properties for the explosion
*****************************************************************************/
void CExplosion::InitializeParticle(int index, float r, float g, float b)
{
  // start the particle at the sky at a random location in the emission zone
  m_particleList[index].m_pos.x = m_origin.x + (FRAND * m_spread)/2;
  m_particleList[index].m_pos.y = m_origin.y + (FRAND * m_spread)/2;
  m_particleList[index].m_pos.z = m_origin.z + (FRAND * m_spread)/2;

  // set the size of the particle
  m_particleList[index].m_size = PARTICLE_SIZE + FRAND * SIZE_VARIATION;

  // give the particle a random velocity
  m_particleList[index].m_velocity.x = PARTICLE_VELOCITY.x; + FRAND * VELOCITY_VARIATION.x;
  m_particleList[index].m_velocity.y = PARTICLE_VELOCITY.y; + FRAND * VELOCITY_VARIATION.y;
  m_particleList[index].m_velocity.z = PARTICLE_VELOCITY.z; + FRAND * VELOCITY_VARIATION.z;

  m_particleList[index].m_acceleration = PARTICLE_ACCELERATION;

  m_particleList[index].m_color[0] = r;
  m_particleList[index].m_color[1] = 0.5f + FRAND * 0.5f;
  m_particleList[index].m_color[2] = g;
  m_particleList[index].m_color[3] = b;

  m_particleList[index].m_energy = 8.5f + FRAND/2.0f;

  m_particleList[index].m_colorDelta[0] = 0.0;
  m_particleList[index].m_colorDelta[1] = -(m_particleList[index].m_color[1]/2.0f)/m_particleList[index].m_energy;
  m_particleList[index].m_colorDelta[2] = 0.0;
  m_particleList[index].m_colorDelta[3] = -1.0f/m_particleList[index].m_energy;

  m_particleList[index].m_sizeDelta = -m_particleList[index].m_size/m_particleList[index].m_energy;

} // end CExplosion::InitializeParticle


/*****************************************************************************
 CExplosion::Update

 Update the existing particles, killing them and creating new ones as needed
*****************************************************************************/
void CExplosion::Update(float *elapsedTime,int j)
{
  for (int i = 0; i < m_numParticles; )
  {
    // update the particle's position based on the elapsed time and velocity
/*    IF YOU WANT MOVE THE PARTICLES. HERE IS THE RIGHT PLACE
      m_particleList[i].m_pos.x = m_particleList[i].m_pos.x - m_particleList[i].m_velocity.x * elapsedTime[j];
      m_particleList[i].m_pos.x = m_particleList[i].m_pos.x + m_particleList[i].m_velocity.x * elapsedTime[j];

      m_particleList[i].m_pos.y = m_particleList[i].m_pos.y + m_particleList[i].m_velocity.y * elapsedTime[j];
      m_particleList[i].m_pos.y = m_particleList[i].m_pos.y - m_particleList[i].m_velocity.y * elapsedTime[j];

      m_particleList[i].m_pos.z = m_particleList[i].m_pos.z + m_particleList[i].m_velocity.z * elapsedTime[j];
      m_particleList[i].m_pos.z = m_particleList[i].m_pos.z - m_particleList[i].m_velocity.z * elapsedTime[j];
*/
	m_particleList[i].m_velocity = m_particleList[i].m_velocity + m_particleList[i].m_acceleration * elapsedTime[j];

    m_particleList[i].m_energy -= elapsedTime[j];
    m_particleList[i].m_size += m_particleList[i].m_sizeDelta * elapsedTime[j];

    m_particleList[i].m_color[3] += m_particleList[i].m_colorDelta[3] * elapsedTime[j];
    m_particleList[i].m_color[1] += m_particleList[i].m_colorDelta[1] * elapsedTime[j];

    // if the particle has hit the ground plane, kill it
    if (m_particleList[i].m_energy <= 0.0)
    {
      // move the last particle to the current positon, and decrease the count
      m_particleList[i] = m_particleList[--m_numParticles];
    }
    else
    {
      ++i;
    }
  }
} // end CExplosion::Update()


/*****************************************************************************
 CExplosion::Render()

 Draw the snowflake particles as textured quads
*****************************************************************************/
void CExplosion::Render()
{
  float viewMatrix[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, viewMatrix);

  CVector right(viewMatrix[0], viewMatrix[4], viewMatrix[8]);
  CVector up(viewMatrix[1], viewMatrix[5], viewMatrix[9]);

  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  GLfloat size;
  CVector pos;

  glBegin(GL_QUADS);
  for (int i = 0; i < m_numParticles; ++i)
  {
    size = m_particleList[i].m_size/2;
    pos = m_particleList[i].m_pos;
    glColor4fv(m_particleList[i].m_color);
    glTexCoord2f(0.0, 1.0); 
	glVertex3fv((pos + (up - right) * size).v);
	glTexCoord2f(0.0, 0.0); 
	glVertex3fv((pos + (right + up) * -size).v);
    glTexCoord2f(1.0, 0.0); 
	glVertex3fv((pos + (right - up) * size).v);
	glTexCoord2f(1.0, 1.0); 
	glVertex3fv((pos + (right + up) * size).v);

  }
  glEnd();
 // glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
} // end CExplosion::Update
