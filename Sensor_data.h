#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

struct Sensor_data
{
    int num_collected_molecules;
    int num_released_molecules;
    float average_temperature;
    float energy_consumption; // including tractors?
};

#endif // SENSOR_DATA_H
