#ifndef MYOPENMESH_H
#define MYOPENMESH_H

#include <iostream>

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>

struct MyTraits : public OpenMesh::DefaultTraits
{
    typedef OpenMesh::VectorT<float, 4> Color;
};


typedef OpenMesh::PolyMesh_ArrayKernelT<>  MyMesh;
//typedef OpenMesh::TriMesh_ArrayKernelT<> MyMesh;
// typedef OpenMesh::TriMesh_ArrayKernelT<MyTraits> MyMesh;

//typedef OpenMesh::PolyMesh_ArrayKernelT<MyTraits> MyMesh;
//typedef OpenMesh::PolyMesh_ArrayKernelT<> MyQuadMesh;

template <typename Mesh>
Mesh load_mesh(std::string const& filename, bool request_vertex_colors = false)
{
    Mesh mesh;

    mesh.request_face_colors();

    if (request_vertex_colors)
    {
        mesh.request_vertex_colors();
    }

    mesh.request_vertex_texcoords2D();

    mesh.request_face_normals();
    mesh.request_vertex_normals();

    OpenMesh::IO::Options read_opts;
    read_opts += OpenMesh::IO::Options::FaceTexCoord;
    read_opts += OpenMesh::IO::Options::VertexTexCoord;
    read_opts += OpenMesh::IO::Options::VertexNormal;

    if (request_vertex_colors)
    {
        read_opts += OpenMesh::IO::Options::VertexColor;
        read_opts += OpenMesh::IO::Options::FaceColor;

        mesh.request_face_colors();
        mesh.request_vertex_colors();
    }

    if (!OpenMesh::IO::read_mesh(mesh, filename, read_opts))
    {
        std::cerr << "Can't read mesh from file: " << filename << std::endl;
        return mesh;
    }

    mesh.triangulate();

    mesh.update_face_normals();
    mesh.update_normals();

    if (request_vertex_colors && mesh.has_face_colors())
    {
        typename Mesh::ConstFaceIter
                fIt(mesh.faces_begin()),
                fEnd(mesh.faces_end());

        for (; fIt != fEnd; ++fIt)
        {
//            typename Mesh::Color color = mesh.color(*fIt);

//            std::cout << *fIt << " "
//                      << int(color[0]) << " "
//                      << int(color[1]) << " "
//                      << int(color[2]) << " "<< std::endl;

            typename Mesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(*fIt);

            mesh.set_color(*fvIt, mesh.color(*fIt));
            ++fvIt;
            mesh.set_color(*fvIt, mesh.color(*fIt));
            ++fvIt;
            mesh.set_color(*fvIt, mesh.color(*fIt));
        }
    }


    bool const flat_shading = false;

    if (flat_shading)
    {
        typename Mesh::ConstFaceIter
                fIt(mesh.faces_begin()),
                fEnd(mesh.faces_end());

        for (; fIt != fEnd; ++fIt)
        {
            typename Mesh::ConstFaceVertexIter fvIt = mesh.cfv_iter(*fIt);

            mesh.set_normal(*fvIt, mesh.normal(*fIt));
            ++fvIt;
            mesh.set_normal(*fvIt, mesh.normal(*fIt));
            ++fvIt;
            mesh.set_normal(*fvIt, mesh.normal(*fIt));
        }
    }

//    if (mesh.has_vertex_colors())
//    {
//        MyMesh::Color const* colors = mesh.vertex_colors();

//        for (int i = 0; i < mesh.n_vertices(); ++i)
//        {
//            std::cout << colors[i] << std::endl;
//        }
//    }

    std::cout << "Mesh loaded: " << filename << " " <<
                 "Vertices: " << mesh.n_vertices() << " " <<
                 "Faces: "    << mesh.n_faces() << " " <<
                 std::endl;

    std::cout << "vnormals? " << mesh.has_vertex_normals() << std::endl;
    std::cout << "vertex.texcoord2D? " << mesh.has_vertex_texcoords2D() << std::endl;

    return mesh;
}

#endif
