#ifndef ICOSPHERE_H
#define ICOSPHERE_H

#ifdef WIN32
#include <Windows.h>
#endif
#include <GL/gl.h>
#include <vector>
#include <cassert>
#include <map>

#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

//--------------------------------------------------------------------------------
// icosahedron data
//--------------------------------------------------------------------------------
#define X_COORD .525731112119133606f
#define Z_COORD .850650808352039932f

//static GLfloat vdata[12][3];
//static GLuint tindices[20][3];


static const GLfloat vdata[12][3] = {
    {-X_COORD, 0.0f, Z_COORD}, {X_COORD, 0.0f, Z_COORD}, {-X_COORD, 0.0f, -Z_COORD}, {X_COORD, 0.0f, -Z_COORD},
    {0.0f, Z_COORD, X_COORD}, {0.0f, Z_COORD, -X_COORD}, {0.0f, -Z_COORD, X_COORD}, {0.0f, -Z_COORD, -X_COORD},
    {Z_COORD, X_COORD, 0.0f}, {-Z_COORD, X_COORD, 0.0f}, {Z_COORD, -X_COORD, 0.0f}, {-Z_COORD, -X_COORD, 0.0f}
};

static const GLuint tindices[20][3] = {
    {0,4,1}, {0,9,4}, {9,5,4}, {4,5,8}, {4,8,1},
    {8,10,1}, {8,3,10}, {5,3,8}, {5,2,3}, {2,7,3},
    {7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},
    {6,1,10}, {9,0,11}, {9,11,2}, {9,2,5}, {7,2,11} };
//--------------------------------------------------------------------------------

template <class Point, class Color>
class IcoSphere
{

public:
    IcoSphere(unsigned int levels = 0)
    {
        // init with an icosahedron
        for (int i = 0; i < 12; i++)
        {
            mVertices.push_back(Point(vdata[i][0],
                                      vdata[i][1],
                                      vdata[i][2])
                                );
        }

        // mIndices.push_back(boost::shared_ptr< std::vector<unsigned int> >(new std::vector<unsigned int>));
        mIndices.push_back(std::vector<unsigned int>());
        // std::vector<unsigned int>& indices = *mIndices.back();
        std::vector<unsigned int>& indices = mIndices.back();
        for (int i = 0; i < 20; i++)
        {
            for (int k = 0; k < 3; k++)
                indices.push_back(tindices[i][k]);
        }
        mListIds.push_back(0);

        while(mIndices.size() <= levels)
            _subdivide();

        original_vertices = mVertices;

        generate_face_normals();

    }

    void generate_face_normals()
    {
        mNormals.resize(mVertices.size());

        for (size_t level = 0; level < mIndices.size(); ++level)
        {
            assert(mIndices[level].size() % 3 == 0);

            for (size_t i = 0; i < mIndices[level].size(); i += 3)
            {
                int const i_0 = mIndices[level][i + 0];
                int const i_1 = mIndices[level][i + 1];
                int const i_2 = mIndices[level][i + 2];

                Point v_0 = mVertices[i_1] - mVertices[i_0];
                Point v_1 = mVertices[i_2] - mVertices[i_0];

                Point normal = cross(v_1, v_0).normalize();

                mNormals[i_0] = normal;
                mNormals[i_1] = normal;
                mNormals[i_2] = normal;
            }
        }
    }

    std::vector<Point> const& vertices() const { return mVertices; }

    //    const std::vector<unsigned int>& get_indices(int level) const
    //    {
    //        while (level>=int(mIndices.size()))
    //            const_cast<IcoSphere*>(this)->_subdivide();
    //        return *mIndices[level];
    //    }

    std::vector<Point> const& get_original_positions()
    {
        return original_vertices;
    }

    void set_vertex_position(int const index, Point const& p)
    {
        mVertices[index] = p;
    }

    void draw(int level = -1) const
    {
        if (level < 0)
        {
            level = int(mIndices.size()) - 1;
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        if (mColors.size() > 0)
        {
            glEnableClientState(GL_COLOR_ARRAY);
        }


        glVertexPointer(3, GL_FLOAT, 0, mVertices[0].data());
        // glNormalPointer(GL_FLOAT, 0, mVertices[0].data());
        glNormalPointer(GL_FLOAT, 0, mNormals[0].data());

        if (mColors.size() > 0)
        {
            glColorPointer(3, GL_FLOAT, 0, &mColors[0]);
        }

        // glDrawElements(GL_TRIANGLES, mIndices[level]->size(), GL_UNSIGNED_INT, &(mIndices[level]->at(0)));
        glDrawElements(GL_TRIANGLES, int(mIndices[level].size()), GL_UNSIGNED_INT, &(mIndices[level][0]));
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

        if (mColors.size() > 0)
        {
            glDisableClientState(GL_COLOR_ARRAY);
        }

        /*
        std::vector<unsigned int> const& indices = get_indices(level);

        glBegin(GL_TRIANGLES);

        for (std::size_t i = 0; i < indices.size(); i += 3)
        {
            if (mColors.size() > 0)
            {
                glColor3fv(mColors[indices[i]].data());
            }
            glNormal3fv(mVertices[indices[i    ]].data());
            glVertex3fv(mVertices[indices[i    ]].data());

            if (mColors.size() > 0)
            {
                glColor3fv(mColors[indices[i + 1]].data());
            }
            glNormal3fv(mVertices[indices[i + 1]].data());
            glVertex3fv(mVertices[indices[i + 1]].data());

            if (mColors.size() > 0)
            {
                glColor3fv(mColors[indices[i + 2]].data());
            }
            glNormal3fv(mVertices[indices[i + 2]].data());
            glVertex3fv(mVertices[indices[i + 2]].data());
        }

        glEnd();
        */
    }

    void set_vertex_colors(std::vector<Color> const& colors)
    {
        mColors = colors;
        assert(mColors.size() == mVertices.size());
    }

protected:
    void _subdivide()
    {
        typedef unsigned long long Key;
        std::map<Key,int> edgeMap;

        // const std::vector<unsigned int>& indices = *mIndices.back();
        // mIndices.push_back(boost::shared_ptr< std::vector<unsigned int> >(new std::vector<unsigned int>));
        // std::vector<unsigned int>& refinedIndices = *mIndices.back();

        mIndices.push_back(std::vector<unsigned int>());
        std::vector<unsigned int> const& indices = mIndices[mIndices.size() - 2];
        std::vector<unsigned int>& refinedIndices = mIndices[mIndices.size() - 1];

        size_t const end = indices.size();
        for (size_t i = 0; i < end; i += 3)
        {
            int ids0[3],  // indices of outer vertices
                    ids1[3];  // indices of edge vertices
            for (int k=0; k<3; ++k)
            {
                int k1 = (k+1)%3;
                int e0 = indices[i+k];
                int e1 = indices[i+k1];
                ids0[k] = e0;
                if (e1>e0)
                    std::swap(e0,e1);
                Key edgeKey = Key(e0) | (Key(e1)<<32);
                std::map<Key,int>::iterator it = edgeMap.find(edgeKey);
                if (it==edgeMap.end())
                {
                    ids1[k] = int(mVertices.size());
                    edgeMap[edgeKey] = ids1[k];
                    mVertices.push_back( (mVertices[e0]+mVertices[e1]).normalize() );
                }
                else
                    ids1[k] = it->second;
            }
            refinedIndices.push_back(ids0[0]); refinedIndices.push_back(ids1[0]); refinedIndices.push_back(ids1[2]);
            refinedIndices.push_back(ids0[1]); refinedIndices.push_back(ids1[1]); refinedIndices.push_back(ids1[0]);
            refinedIndices.push_back(ids0[2]); refinedIndices.push_back(ids1[2]); refinedIndices.push_back(ids1[1]);
            refinedIndices.push_back(ids1[0]); refinedIndices.push_back(ids1[1]); refinedIndices.push_back(ids1[2]);
        }
        mListIds.push_back(0);
    }

    std::vector<Point> mVertices;
    std::vector<Point> original_vertices;
    std::vector<Color> mColors;

    std::vector<Point> mNormals;

    // std::vector< boost::shared_ptr< std::vector<unsigned int> > > mIndices;
    std::vector< std::vector<unsigned int> > mIndices;
    std::vector<int> mListIds;
};




#endif // ICOSPHERE_H
