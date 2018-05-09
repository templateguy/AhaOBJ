//
//  ahaOBJ.hpp
//  AhaOBJ
//
//  Created by Sinha, Saurabh on 5/7/18.
//  Copyright © 2018 Sinha, Saurabh. All rights reserved.
//

#pragma once


#include <string>
#include <vector>
#include <fstream>


namespace aha
{
    struct Coord
    {
        Coord(float x, float y, float z) : x(x), y(y), z(z)
        {
            ;
        }
        
        float x{};
        float y{};
        float z{};
    };
    
    struct Face
    {
        Face(int faceNum, int f1, int f2, int f3) : faceNum(faceNum), four(false)
        {
            faces[0] = f1;
            faces[1] = f2;
            faces[2] = f3;
        }
        
        Face(int faceNum, int f1, int f2, int f3, int f4) : faceNum(faceNum), four(true)
        {
            faces[0] = f1;
            faces[1] = f2;
            faces[2] = f3;
            faces[3] = f4;
        }
        
        int faceNum{};
        bool four{};
        int faces[4];
    };
    
    class OBJ
    {
    public:
        OBJ() = default;
        
        OBJ(const std::string& fileName)
        {
            load(fileName);
        }
        
        bool load(const std::string& fileName)
        {
            std::vector<std::string*> coord;
            std::vector<Coord*> vertex;
            std::vector<Face*> faces;
            std::vector<Coord*> normals;
            
            std::ifstream in(fileName);
            if(!in.is_open())
            {
                std::cout << "Nor oepened" << std::endl;
                return false;
            }
            
            char buf[256];
            while(!in.eof())
            {
                in.getline(buf,256);
                coord.push_back(new std::string(buf));
            }
            //go through all of the elements of coord, and decide what kind of element is that
            for(int i=0;i<coord.size();i++)
            {
                if(coord[i]->c_str()[0]=='#')   //if it is a comment (the first character is #)
                    continue;       //we don't care about that
                else if(coord[i]->c_str()[0]=='v' && coord[i]->c_str()[1]==' ') //if vector
                {
                    float tmpx,tmpy,tmpz;
                    sscanf(coord[i]->c_str(),"v %f %f %f",&tmpx,&tmpy,&tmpz);       //read in the 3 float coordinate to tmpx,tmpy,tmpz
                    vertex.push_back(new Coord(tmpx, tmpy, tmpz));       //and then add it to the end of our vertex list
                }
                else if(coord[i]->c_str()[0]=='v' && coord[i]->c_str()[1]=='n')        //if normal vector
                {
                    float tmpx,tmpy,tmpz;   //do the same thing
                    sscanf(coord[i]->c_str(),"vn %f %f %f",&tmpx,&tmpy,&tmpz);
                    normals.push_back(new Coord(tmpx, tmpy, tmpz));
                }
                else if(coord[i]->c_str()[0]=='f')     //if face
                {
                    int a,b,c,d,e;
                    if(count(coord[i]->begin(),coord[i]->end(),' ')==3)     //if it is a triangle (it has 3 space in it)
                    {
                        sscanf(coord[i]->c_str(),"f %d//%d %d//%d %d//%d",&a,&b,&c,&b,&d,&b);
                        faces.push_back(new Face(b, a, c, d));     //read in, and add to the end of the face list
                    }
                    else
                    {
                        sscanf(coord[i]->c_str(),"f %d//%d %d//%d %d//%d %d//%d",&a,&b,&c,&b,&d,&b,&e,&b);
                        faces.push_back(new Face(b, a, c, d, e));   //do the same, except we call another constructor, and we use different pattern
                    }
                }
            }
            //raw
            list_ = glGenLists(1);      //generate a uniqe
            glNewList(list_, GL_COMPILE);      //and create it
            for(int i = 0; i < faces.size(); i++)
            {
                if(faces[i]->four)      //if it's a quad draw a quad
                {
                    glBegin(GL_QUADS);
                    //basically all I do here, is use the facenum (so the number of the face) as an index for the normal, so the 1st normal owe to the first face
                    //I subtract 1 because the index start from 0 in C++
                    glNormal3f(normals[faces[i]->faceNum - 1]->x, normals[faces[i]->faceNum - 1]->y, normals[faces[i]->faceNum - 1]->z);
                    //draw the faces
                    glVertex3f(vertex[faces[i]->faces[0] - 1]->x, vertex[faces[i]->faces[0] - 1]->y, vertex[faces[i]->faces[0] - 1]->z);
                    glVertex3f(vertex[faces[i]->faces[1] - 1]->x, vertex[faces[i]->faces[1] - 1]->y, vertex[faces[i]->faces[1] - 1]->z);
                    glVertex3f(vertex[faces[i]->faces[2] - 1]->x, vertex[faces[i]->faces[2] - 1]->y, vertex[faces[i]->faces[2] - 1]->z);
                    glVertex3f(vertex[faces[i]->faces[3] - 1]->x, vertex[faces[i]->faces[3] - 1]->y, vertex[faces[i]->faces[3] - 1]->z);
                    glEnd();
                }
                else
                {
                    glBegin(GL_TRIANGLES);
                    glNormal3f(normals[faces[i]->faceNum - 1]->x, normals[faces[i]->faceNum - 1]->y, normals[faces[i]->faceNum - 1]->z);
                    glVertex3f(vertex[faces[i]->faces[0] - 1]->x, vertex[faces[i]->faces[0] - 1]->y, vertex[faces[i]->faces[0] - 1]->z);
                    glVertex3f(vertex[faces[i]->faces[1] - 1]->x, vertex[faces[i]->faces[1] - 1]->y, vertex[faces[i]->faces[1] - 1]->z);
                    glVertex3f(vertex[faces[i]->faces[2] - 1]->x, vertex[faces[i]->faces[2] - 1]->y, vertex[faces[i]->faces[2] - 1]->z);
                    glEnd();
                }
            }
            glEndList();
            //delete everything to avoid memory leaks
            for(int i = 0; i < coord.size(); i++)
                delete coord[i];
            for(int i = 0; i < faces.size(); i++)
                delete faces[i];
            for(int i = 0; i < normals.size(); i++)
                delete normals[i];
            for(int i = 0; i < vertex.size(); i++)
                delete vertex[i];
            return true;
        }
        
        void render() const
        {
            glCallList(cube_);
        }
        
    protected:
        unsigned int list_;
        unsigned int cube_;
    };
}
