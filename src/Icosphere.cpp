#include "Icosphere.h"

/*

GLfloat vdata[12][3] = {
    {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},
    {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},
    {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0}
};

GLuint tindices[20][3] = {
    {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},
    {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},
    {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},
    {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };
*/

/*
void IcoSphere::draw(int level)
{
    while (level >= int(mIndices.size()))
    {
        _subdivide();
    }

    // int level = mIndices.size();

    // assert(mVertices.size() == mColors.size());

    if (mListIds[level]==0)
    {
        //mListIds[level] = glGenLists(1);
        //glNewList(mListIds[level], GL_COMPILE);

        glEnableClientState(GL_VERTEX_ARRAY);
        //glEnableClientState(GL_NORMAL_ARRAY);
        //glEnableClientState(GL_COLOR_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, mVertices[0].data());
        // glNormalPointer(GL_FLOAT, 0, mVertices[0].data());
        // glColorPointer(3, GL_FLOAT, 0, mColors[0].data());
        glDrawElements(GL_TRIANGLES, mIndices[level]->size(), GL_UNSIGNED_INT, &(mIndices[level]->at(0)));
        glDisableClientState(GL_VERTEX_ARRAY);
        // glDisableClientState(GL_NORMAL_ARRAY);
        //glDisableClientState(GL_COLOR_ARRAY);
        //glEndList();
    }

    glCallList(mListIds[level]);
}
*/

