#include <fstream>
#include "../Headers/ResourceManager.h"
#include <filesystem>

std::future<std::vector<Vertex>> ResourceManager::loadMeshFromFile(const std::string& filename) const
{
    auto it = _models.find(filename);
    if (it != _models.end()) {
        std::promise<std::vector<Vertex>> promise;
        promise.set_value(it->second);
        return promise.get_future();
    }

    return std::async(std::launch::async, [filename]() {
        std::vector<Vertex> vertices;
        std::ifstream reader(filename);

        //If failed to open file, return empty vertices if in release.
        //Otherwise, I want to know about it, so be very loud.
        if (!reader.is_open()) {
#ifdef _DEBUG
            std::string s = "Failed to open model file: " + filename + " (";
            SK::BYTE errorMsg[256];
            strerror_s(errorMsg, sizeof errorMsg, errno);
            s += errorMsg;
            s += ")";
            throw std::exception(s.c_str());
#endif
            return vertices;
        }

        SK::INT32 numVertices;
        reader >> numVertices;
        vertices.reserve(numVertices);
        for (SK::INT32 i = 0; i < numVertices; ++i) {
            Vertex v;
            reader >> v.Position.x;
            reader >> v.Position.y;
            reader >> v.Position.z;
            reader >> v.Colour.x;
            reader >> v.Colour.y;
            reader >> v.Colour.z;
            vertices.push_back(v);
        }
        reader.close();
        return vertices;
    });

}

std::future<std::vector<Vertex>> ResourceManager::loadCube() const
{
    return loadMeshFromFile("cube.sm");
    //return loadMeshFromFile("..\\cube.sm");
}

std::future<Shader> ResourceManager::loadShaderFromFile(const std::string& vsPath, const std::string& psPath) const
{
    auto it = _shaders.find(vsPath + psPath);
    if (it != _shaders.end()) {
        std::promise<Shader> promise;
        promise.set_value(it->second);
        return promise.get_future();
    }

    return std::async(std::launch::async, [this,vsPath, psPath]() {
        return Shader(vsPath, psPath, _device);
    });
}

std::future<Shader> ResourceManager::loadShaderFromFile(const std::string& path) const
{
    auto it = _shaders.find(path);
    if (it != _shaders.end()) {
        std::promise<Shader> promise;
        promise.set_value(it->second);
        return promise.get_future();
    }

    return std::async(std::launch::async, [this,path]() {
        try {
            auto shader = Shader(path, _device);
            return shader;
        }
        catch (const std::exception& e) {
            std::string s = "Error when creating shader, path: " + path;
            throw std::exception(s.c_str());
        }
    });
}