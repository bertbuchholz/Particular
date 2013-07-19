#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <Eigen/Core>
#include <functional>
#include <vector>

#include <Color.h>

struct Particle
{
    Eigen::Vector3f position;
    Eigen::Vector3f speed;
    Color color;
    float age; // between 0 and 1
};

class Particle_system
{
public:
    void animate(std::function<void(Particle&, float const)> particle_function, float const timestep)
    {
        for (Particle & p : _particles)
        {
            particle_function(p, timestep);
        }
    }

private:
    std::vector<Particle> _particles;
};

#endif // PARTICLE_SYSTEM_H
