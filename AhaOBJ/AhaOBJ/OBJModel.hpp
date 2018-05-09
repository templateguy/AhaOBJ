//
//  OBJModel.hpp
//  AhaOBJ
//
//  Created by Sinha, Saurabh on 5/8/18.
//  Copyright © 2018 Sinha, Saurabh. All rights reserved.
//

#pragma once


#include <string>
#include <vector>
#include <map>
#include <limits>
#include <cmath>
#include <OpenGL/gl3.h>
#include "tiny_obj_loader.h"
#include "trackball.h"
#include "Texture.hpp"
#include "Shader.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"


namespace aha
{
    class OBJModel
    {
    public:
        void loadModel(const std::string& fileName)
        {
            shader_.load("shaders/model.vs", "shaders/model.fs");
            shader_.use();
            base_ = getBaseDir_(fileName) + "/";
            parseFile_(fileName);
            appendDefaultMaterial_();
            printOBJModelInfo_();
            loadDiffuseTextures_();
            populateBuffer_();
            printMaxMin_();
        }
        
        void render(bool wireframe = false) const
        {
            if(!wireframe)
            {
                glEnable(GL_DEPTH_TEST);
                for(const auto& mesh : meshes_)
                {
                    if(mesh.vbo < 1)
                    {
                        continue;
                    }
                    
                    shader_.use();
                    
                    // view/projection transformations
                    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float) 768.0f / (float) 768.0f, 0.1f, 100.0f);
                    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                    shader_.setMat4("projection", projection);
                    shader_.setMat4("view", view);
                    
                    // render the loaded model
                    glm::mat4 model;
                    
                    //build_rotmatrix_(model, currQuat);
                    
                    // Fit to -1, 1
                    float maxExtent(getMaxExtent());
                    model = glm::scale(model, glm::vec3(1.0f / maxExtent, 1.0f / maxExtent, 1.0f / maxExtent));
                    
                    // Centerize object
                    model = glm::translate(model, glm::vec3(-0.5 * (max_[0] + min_[0]), -0.5 * (max_[1] + min_[1]), -0.5 * (max_[2] + min_[2])));
                    
                    shader_.setMat4("model", model);
                    
                    glBindVertexArray(mesh.vao);
                    
                    if((mesh.materialId < materials_.size()))
                    {
                        const std::string diffuse_texname = materials_[mesh.materialId].diffuse_texname;
                        std::string tex(base_ + diffuse_texname);
                        if(textures_.find(tex) != textures_.end())
                        {
                            glActiveTexture(GL_TEXTURE0);
                            auto l(glGetUniformLocation(shader_.ID, "texture_diffuse1"));
                            glUniform1i(l, 0);
                            glBindTexture(GL_TEXTURE_2D, textures_.at(tex)->getHandle());
                        }
                    }
                    glDrawArrays(GL_TRIANGLES, 0, 3 * static_cast <int> (mesh.numTriangles));
                    checkErrors("drawarrays");
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
        }
        
        const float* const getMax() const
        {
            return max_;
        }
        
        const float* const getMin() const
        {
            return min_;
        }
        
        float getMaxExtent() const
        {
            float maxExtent(0.5f * (max_[0] - min_[0]));
            if(maxExtent < 0.5f * (max_[1] - min_[1]))
            {
                maxExtent = 0.5f * (max_[1] - min_[1]);
            }
            if(maxExtent < 0.5f * (max_[2] - min_[2]))
            {
                maxExtent = 0.5f * (max_[2] - min_[2]);
            }
            return maxExtent;
        }
        
    protected:
        void parseFile_(const std::string& fileName)
        {
            std::string err;
            tinyobj::LoadObj(&attrib_, &shapes_, &materials_, &err, fileName.c_str(), base_.c_str());
            if(!err.empty())
            {
                std::cerr << err << std::endl;
            }
        }
        
        void appendDefaultMaterial_()
        {
            materials_.push_back(tinyobj::material_t());
        }
        
        void printOBJModelInfo_()
        {
            printf("# of vertices  = %d\n", static_cast <int> (attrib_.vertices.size() / 3));
            printf("# of normals   = %d\n", static_cast <int> (attrib_.normals.size() / 3));
            printf("# of texcoords = %d\n", static_cast <int> (attrib_.texcoords.size() / 2));
            printf("# of materials = %d\n", static_cast <int> (materials_.size()));
            printf("# of shapes    = %d\n", static_cast <int> (shapes_.size()));
            for(size_t i = 0; i < materials_.size(); i++)
            {
                printf("material[%d].diffuse_texname = %s\n", int(i), materials_[i].diffuse_texname.c_str());
            }
        }
        
        void loadDiffuseTextures_()
        {
            for(auto& material : materials_)
            {
                if(material.diffuse_texname.length() > 0)
                {
                    std::string tex(base_ + material.diffuse_texname);
                    // Only load the texture if it is not already loaded
                    if(textures_.find(tex) == textures_.end())
                    {
                        auto texture(Texture::New(tex));
                        textures_.insert(std::make_pair(tex, texture));
                    }
                }
            }
        }
        
        void populateBuffer_()
        {
            size_t s{};
            for(auto& shape : shapes_)
            {
                Mesh_ mesh;
                std::vector <float> buffer;
                
                // Check for smoothing group and compute smoothing normals
                std::map <int, vec3_> smoothVertexNormals;
                if(hasSmoothingGroup_(shape) > 0)
                {
                    std::cout << "Compute smoothingNormal for shape [" << s << "]" << std::endl;
                    computeSmoothingNormals(attrib_, shape, smoothVertexNormals);
                }
                
                for(size_t f(0); f < shape.mesh.indices.size() / 3; ++f)
                {
                    tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
                    tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
                    tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];
                    
                    int currentMaterialId = shape.mesh.material_ids[f];
                    if((currentMaterialId < 0) || (currentMaterialId >= static_cast <int> (materials_.size())))
                    {
                        // Invaid material ID. Use default material.
                        currentMaterialId = static_cast <int> (materials_.size() - 1);  // Default material is added to the last item in `materials`.
                    }
                    
                    float diffuse[3];
                    for(size_t i = 0; i < 3; i++)
                    {
                        diffuse[i] = materials_[currentMaterialId].diffuse[i];
                    }
                    float tc[3][2];
                    if((attrib_.texcoords.size() <= 0) || (idx0.texcoord_index < 0) || (idx1.texcoord_index < 0) || (idx2.texcoord_index < 0))
                    {
                        // face does not contain valid uv index.
                        tc[0][0] = 0.0f;
                        tc[0][1] = 0.0f;
                        tc[1][0] = 0.0f;
                        tc[1][1] = 0.0f;
                        tc[2][0] = 0.0f;
                        tc[2][1] = 0.0f;
                    }
                    else
                    {
                        assert(attrib_.texcoords.size() > size_t(2 * idx0.texcoord_index + 1));
                        assert(attrib_.texcoords.size() > size_t(2 * idx1.texcoord_index + 1));
                        assert(attrib_.texcoords.size() > size_t(2 * idx2.texcoord_index + 1));
                        
                        // Flip Y coord.
                        tc[0][0] = attrib_.texcoords[2 * idx0.texcoord_index];
                        tc[0][1] = 1.0f - attrib_.texcoords[2 * idx0.texcoord_index + 1];
                        tc[1][0] = attrib_.texcoords[2 * idx1.texcoord_index];
                        tc[1][1] = 1.0f - attrib_.texcoords[2 * idx1.texcoord_index + 1];
                        tc[2][0] = attrib_.texcoords[2 * idx2.texcoord_index];
                        tc[2][1] = 1.0f - attrib_.texcoords[2 * idx2.texcoord_index + 1];
                    }
                    
                    // Vertices
                    float v[3][3];
                    for (int k = 0; k < 3; k++)
                    {
                        int f0 = idx0.vertex_index;
                        int f1 = idx1.vertex_index;
                        int f2 = idx2.vertex_index;
                        assert(f0 >= 0);
                        assert(f1 >= 0);
                        assert(f2 >= 0);
                        
                        v[0][k] = attrib_.vertices[3 * f0 + k];
                        v[1][k] = attrib_.vertices[3 * f1 + k];
                        v[2][k] = attrib_.vertices[3 * f2 + k];
                        min_[k] = std::min(v[0][k], min_[k]);
                        min_[k] = std::min(v[1][k], min_[k]);
                        min_[k] = std::min(v[2][k], min_[k]);
                        max_[k] = std::max(v[0][k], max_[k]);
                        max_[k] = std::max(v[1][k], max_[k]);
                        max_[k] = std::max(v[2][k], max_[k]);
                    }
                    
                    // Normals
                    float n[3][3];
                    bool invalidNormalIndex(false);
                    if(attrib_.normals.size() > 0)
                    {
                        int nf0 = idx0.normal_index;
                        int nf1 = idx1.normal_index;
                        int nf2 = idx2.normal_index;
                        
                        if((nf0 < 0) || (nf1 < 0) || (nf2 < 0))
                        {
                            // normal index is missing from this face.
                            invalidNormalIndex = true;
                        }
                        else
                        {
                            for(int k = 0; k < 3; k++)
                            {
                                assert(size_t(3 * nf0 + k) < attrib_.normals.size());
                                assert(size_t(3 * nf1 + k) < attrib_.normals.size());
                                assert(size_t(3 * nf2 + k) < attrib_.normals.size());
                                n[0][k] = attrib_.normals[3 * nf0 + k];
                                n[1][k] = attrib_.normals[3 * nf1 + k];
                                n[2][k] = attrib_.normals[3 * nf2 + k];
                            }
                        }
                    }
                    else
                    {
                        invalidNormalIndex = true;
                    }
                    
                    if(invalidNormalIndex && !smoothVertexNormals.empty())
                    {
                        // Use smoothing normals
                        int f0 = idx0.vertex_index;
                        int f1 = idx1.vertex_index;
                        int f2 = idx2.vertex_index;
                        
                        if(f0 >= 0 && f1 >= 0 && f2 >= 0)
                        {
                            n[0][0] = smoothVertexNormals[f0].v[0];
                            n[0][1] = smoothVertexNormals[f0].v[1];
                            n[0][2] = smoothVertexNormals[f0].v[2];
                            
                            n[1][0] = smoothVertexNormals[f1].v[0];
                            n[1][1] = smoothVertexNormals[f1].v[1];
                            n[1][2] = smoothVertexNormals[f1].v[2];
                            
                            n[2][0] = smoothVertexNormals[f2].v[0];
                            n[2][1] = smoothVertexNormals[f2].v[1];
                            n[2][2] = smoothVertexNormals[f2].v[2];
                            
                            invalidNormalIndex = false;
                        }
                    }
                    if(invalidNormalIndex)
                    {
                        // compute geometric normal
                        calculateNormal_(n[0], v[0], v[1], v[2]);
                        n[1][0] = n[0][0];
                        n[1][1] = n[0][1];
                        n[1][2] = n[0][2];
                        n[2][0] = n[0][0];
                        n[2][1] = n[0][1];
                        n[2][2] = n[0][2];
                    }
                    
                    // Push values into buffer
                    for(int k = 0; k < 3; k++)
                    {
                        buffer.push_back(v[k][0]);
                        buffer.push_back(v[k][1]);
                        buffer.push_back(v[k][2]);
                        buffer.push_back(n[k][0]);
                        buffer.push_back(n[k][1]);
                        buffer.push_back(n[k][2]);
                        // Combine normal and diffuse to get color.
                        float normal_factor = 0.2;
                        float diffuse_factor = 1 - normal_factor;
                        float c[3] = {n[k][0] * normal_factor + diffuse[0] * diffuse_factor, n[k][1] * normal_factor + diffuse[1] * diffuse_factor, n[k][2] * normal_factor + diffuse[2] * diffuse_factor};
                        float len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
                        if(len2 > 0.0f)
                        {
                            float len = sqrtf(len2);
                            
                            c[0] /= len;
                            c[1] /= len;
                            c[2] /= len;
                        }
                        buffer.push_back(c[0] * 0.5 + 0.5);
                        buffer.push_back(c[1] * 0.5 + 0.5);
                        buffer.push_back(c[2] * 0.5 + 0.5);
                        
                        buffer.push_back(tc[k][0]);
                        buffer.push_back(tc[k][1]);
                    }
                }
                
                // Generate vbo
                mesh.vbo = 0;
                mesh.numTriangles = 0;
                
                // OpenGL viewer does not support texturing with per-face material.
                if(shape.mesh.material_ids.size() > 0 && shape.mesh.material_ids.size() > s)
                {
                    mesh.materialId = shape.mesh.material_ids[0];  // use the material ID of the first face.
                }
                else
                {
                    mesh.materialId = materials_.size() - 1;  // = ID for default material.
                }
                printf("shape[%d] material_id %d\n", int(s), int(mesh.materialId));
                
                if(buffer.size() > 0)
                {
                    glGenVertexArrays(1, &mesh.vao);
                    glGenBuffers(1, &mesh.vbo);
                    glBindVertexArray(mesh.vao);
                    
                    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
                    glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float), &buffer.at(0), GL_STATIC_DRAW);
                    
                    GLsizei stride = (3 + 3 + 3 + 2) * sizeof(float);
                    
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*) 0);
                    // vertex normals
                    glEnableVertexAttribArray(1);
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*) (3 * sizeof(float)));
                    // vertex color
                    glEnableVertexAttribArray(2);
                    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*) (6 * sizeof(float)));
                    // vertex texture coords
                    glEnableVertexAttribArray(3);
                    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*) (9 * sizeof(float)));
                    
                    mesh.numTriangles = static_cast <int> (buffer.size()) / (3 + 3 + 3 + 2) / 3;  // 3:vtx, 3:normal, 3:col, 2:texcoord
                    
                    printf("shape[%d] # of triangles = %d\n", static_cast <int> (s), static_cast <int> (mesh.numTriangles));
                }
                
                meshes_.push_back(mesh);
                ++s;
            }
        }
        
        void printMaxMin_()
        {
            printf("bmin = %f, %f, %f\n", min_[0], min_[1], min_[2]);
            printf("bmax = %f, %f, %f\n", max_[0], max_[1], max_[2]);
        }
        
        static void calculateNormal_(float N[3], float v0[3], float v1[3], float v2[3])
        {
            const float v10[] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
            const float v20[] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
            
            N[0] = v20[1] * v10[2] - v20[2] * v10[1];
            N[1] = v20[2] * v10[0] - v20[0] * v10[2];
            N[2] = v20[0] * v10[1] - v20[1] * v10[0];
            
            const float len2(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]);
            if(len2 > 0.0f)
            {
                float len = sqrtf(len2);
                
                N[0] /= len;
                N[1] /= len;
                N[2] /= len;
            }
        }
        
        struct vec3_
        {
            float v[3] = {0.0f, 0.0f, 0.0f};
        };
        
        void normalizeVector_(vec3_& v)
        {
            float len2 = v.v[0] * v.v[0] + v.v[1] * v.v[1] + v.v[2] * v.v[2];
            if(len2 > 0.0f)
            {
                float len = sqrtf(len2);
                
                v.v[0] /= len;
                v.v[1] /= len;
                v.v[2] /= len;
            }
        }
        
        // Check if`mesh_t` contains smoothing group id.
        bool hasSmoothingGroup_(const tinyobj::shape_t& shape)
        {
            for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++)
            {
                if(shape.mesh.smoothing_group_ids[i] > 0)
                {
                    return true;
                }
            }
            return false;
        }
        
        void computeSmoothingNormals(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape, std::map<int, vec3_>& smoothVertexNormals)
        {
            smoothVertexNormals.clear();
            std::map <int, vec3_>::iterator iter;
            
            for(size_t f = 0; f < shape.mesh.indices.size() / 3; f++)
            {
                // Get the three indexes of the face (all faces are triangular)
                tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
                tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
                tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];
                
                // Get the three vertex indexes and coordinates
                int vi[3];      // indexes
                float v[3][3];  // coordinates
                
                for (int k = 0; k < 3; k++)
                {
                    vi[0] = idx0.vertex_index;
                    vi[1] = idx1.vertex_index;
                    vi[2] = idx2.vertex_index;
                    assert(vi[0] >= 0);
                    assert(vi[1] >= 0);
                    assert(vi[2] >= 0);
                    
                    v[0][k] = attrib.vertices[3 * vi[0] + k];
                    v[1][k] = attrib.vertices[3 * vi[1] + k];
                    v[2][k] = attrib.vertices[3 * vi[2] + k];
                }
                
                // Compute the normal of the face
                float normal[3];
                calculateNormal_(normal, v[0], v[1], v[2]);
                
                // Add the normal to the three vertexes
                for(size_t i = 0; i < 3; ++i)
                {
                    iter = smoothVertexNormals.find(vi[i]);
                    if(iter != smoothVertexNormals.end())
                    {
                        // add
                        iter->second.v[0] += normal[0];
                        iter->second.v[1] += normal[1];
                        iter->second.v[2] += normal[2];
                    }
                    else
                    {
                        smoothVertexNormals[vi[i]].v[0] = normal[0];
                        smoothVertexNormals[vi[i]].v[1] = normal[1];
                        smoothVertexNormals[vi[i]].v[2] = normal[2];
                    }
                }
            }
            
            // Normalize the normals, that is, make them unit vectors
            for(iter = smoothVertexNormals.begin(); iter != smoothVertexNormals.end(); iter++)
            {
                normalizeVector_(iter->second);
            }
        }
        
        void build_rotmatrix_(glm::mat4& m, const float q[4]) const
        {
            m[0][0] = 1.0 - 2.0 * (q[1] * q[1] + q[2] * q[2]);
            m[0][1] = 2.0 * (q[0] * q[1] - q[2] * q[3]);
            m[0][2] = 2.0 * (q[2] * q[0] + q[1] * q[3]);
            m[0][3] = 0.0;
            
            m[1][0] = 2.0 * (q[0] * q[1] + q[2] * q[3]);
            m[1][1] = 1.0 - 2.0 * (q[2] * q[2] + q[0] * q[0]);
            m[1][2] = 2.0 * (q[1] * q[2] - q[0] * q[3]);
            m[1][3] = 0.0;
            
            m[2][0] = 2.0 * (q[2] * q[0] - q[1] * q[3]);
            m[2][1] = 2.0 * (q[1] * q[2] + q[0] * q[3]);
            m[2][2] = 1.0 - 2.0 * (q[1] * q[1] + q[0] * q[0]);
            m[2][3] = 0.0;
            
            m[3][0] = 0.0;
            m[3][1] = 0.0;
            m[3][2] = 0.0;
            m[3][3] = 1.0;
        }
        
        static std::string getBaseDir_(const std::string& filepath)
        {
            if(filepath.find_last_of("/\\") != std::string::npos)
            {
                return filepath.substr(0, filepath.find_last_of("/\\"));
            }
            return "";
        }
        
        static void checkErrors(std::string desc)
        {
            GLenum e = glGetError();
            if(e != GL_NO_ERROR)
            {
                fprintf(stderr, "OpenGL error in \"%s\": %d (%d)\n", desc.c_str(), e, e);
                exit(20);
            }
        }
        
        struct Mesh_
        {
            unsigned int vao{};
            unsigned int vbo{};
            size_t numTriangles{};
            size_t materialId{};
        };
        
        std::string base_{};
        
        std::vector <Mesh_> meshes_;
        std::map <std::string, std::shared_ptr <Texture>> textures_;
        
        // tinyobj objects
        tinyobj::attrib_t attrib_;
        std::vector <tinyobj::shape_t> shapes_;
        std::vector <tinyobj::material_t> materials_;
        float min_[3] = {std::numeric_limits <float>::max(), std::numeric_limits <float>::max(), std::numeric_limits <float>::max()};
        float max_[3] = {-std::numeric_limits <float>::max(), -std::numeric_limits <float>::max(), -std::numeric_limits <float>::max()};
        float currQuat[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        
        // Shader
        aha::Shader shader_;
    };
}
