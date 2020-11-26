#include "Scene_MeshPLY.h"
#include <fstream>

static void consumelineandexpect(std::ifstream& stream, const std::string& expect)
{
    std::string line;
    std::getline(stream, line);
    if (line != expect) throw std::exception(std::string("Expected '" + expect + "'.").c_str());
}

MeshPLY::MeshPLY(const std::string& filename)
{
    std::ifstream readit(filename);
    std::string line;
    consumelineandexpect(readit, "ply");
    consumelineandexpect(readit, "format ascii 1.0");
    consumelineandexpect(readit, "comment zipper output");
    std::getline(readit, line);
    if (line.substr(0, 15) != "element vertex ") throw std::exception("Expected 'element vertex <nnn>'.");
    int vertexcount = std::stoi(line.substr(15));
    consumelineandexpect(readit, "property float x");
    consumelineandexpect(readit, "property float y");
    consumelineandexpect(readit, "property float z");
    consumelineandexpect(readit, "property float confidence");
    consumelineandexpect(readit, "property float intensity");
    std::getline(readit, line);
    if (line.substr(0, 13) != "element face ") throw std::exception("Expected 'element face <nnn>'.");
    int facecount = std::stoi(line.substr(13));
    consumelineandexpect(readit, "property list uchar int vertex_indices");
    consumelineandexpect(readit, "end_header");
    m_vertices.reserve(vertexcount);
    for (int i = 0; i < vertexcount; ++i)
    {
        std::getline(readit, line, ' ');
        float x = std::stof(line);
        std::getline(readit, line, ' ');
        float y = std::stof(line);
        std::getline(readit, line, ' ');
        float z = std::stof(line);
        std::getline(readit, line, ' ');
        float confidence = std::stof(line);
        std::getline(readit, line);
        float intensity = std::stof(line);
        m_vertices.push_back({x, y, z});
    }
    m_faces.reserve(facecount);
    for (int i = 0; i < facecount; ++i)
    {
        std::getline(readit, line, ' ');
        int indexcount = std::stoi(line);
        if (indexcount != 3) throw std::exception("Input must be triangles only.");
        std::getline(readit, line, ' ');
        int a = std::stoi(line);
        std::getline(readit, line, ' ');
        int b = std::stoi(line);
        std::getline(readit, line);
        int c = std::stoi(line);
        m_faces.push_back({a, b, c});
    }
}

uint32_t MeshPLY::getVertexCount()
{
    return m_vertices.size();
}

uint32_t MeshPLY::getIndexCount()
{
    return 3 * m_faces.size();
}

void MeshPLY::copyVertices(void* to, uint32_t stride)
{
    void *begin = to;
    for (int i = 0; i < m_vertices.size(); ++i)
    {
        *reinterpret_cast<TVector3<float>*>(to) = m_vertices[i];
        to = reinterpret_cast<uint8_t*>(to) + stride;
    }
    int test = 0;
}

void MeshPLY::copyIndices(void* to, uint32_t stride)
{
    uint32_t* from = reinterpret_cast<uint32_t*>(&m_faces[0]);
    for (int i = 0; i < m_faces.size() * 3; ++i)
    {
        *reinterpret_cast<uint32_t*>(to) = *from;
        ++from;
        to = reinterpret_cast<uint8_t*>(to) + stride;
    }
}