/****************************************************************************
 Particles.h  Particle and particle system base classes.
 
 Author   :   Javier Palmer
 Date     :   20/2/2005

*****************************************************************************/
#ifndef __EXPLOSION_H_INCLUDED__
#define __EXPLOSION_H_INCLUDED__


/********************************* Includes *********************************/
#include "Particles.h"


/*************************** Constants and Macros ***************************/
const CVector PARTICLE_VELOCITY  (0.0f, 2.0f, 0.0f);
const CVector VELOCITY_VARIATION  (4.0f, 4.0f, 4.0f);
const CVector PARTICLE_ACCELERATION  (-0.2f, -0.5f, -0.2f);
const float     PARTICLE_SIZE      = 5.0f;
const float     SIZE_VARIATION     = 2.0f;

#define FRAND   (((float)rand()-(float)rand())/RAND_MAX)

/***************************** Data structures ******************************/
class CExplosion : public CParticleSystem
{
public:
  CExplosion(int maxParticles, CVector origin, float spread, GLuint texture, float r, float g, float b);

  void  Update(float *elapsedTime, int i);
  void  Render();

  bool  IsDead() { return m_numParticles == 0; }

protected:
  void    InitializeParticle(int index, float r, float g, float b);
  float   m_spread;

  GLuint  m_texture;     // explosion texture
};



#endif // __EXPLOSION_H_INCLUDED__