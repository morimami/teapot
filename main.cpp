/*  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include "GL/glew.h"				//for GL extensions checking
#define GLFW_NO_GLU
#include "GL/glfw.h"				//for the context
#include "glm.hpp"                 //for matrices/math
#include "gtc/type_ptr.hpp"         //glm
#include "gtc/matrix_transform.hpp"  //glm
#include "util.h"   //shader loading functions
#include "math.h"
#include <string>
#include <fstream>
#include <regex>

const int WINDOW_SIZE_X = 800;
const int WINDOW_SIZE_Y = 600;
const int ARCBALL_RADIUS = 1.0;
const char* filename = ("assets/teapot.obj");
static const double PI = 6*asin( 0.5 );

glm::mat4 modelMatrix;  //to move our object around
glm::mat4 viewMatrix;   //to move our camera around
glm::mat4 projectionMatrix; //for perspective projection
glm::mat4 modelViewMatrix;
glm::mat4 pvm;
glm::mat4 rotateMatrix_x;
glm::mat4 rotateMatrix_y;
glm::mat4 translateMatrix1;
glm::mat4 translateMatrix2;
glm::mat4 lightPV;

GLuint modelVAO;  //groups all our VBO's
GLuint positionVBO;
GLuint elementsVBO;
GLuint normalVBO;
GLuint shadowFBO;

GLuint shaderProgram;
GLuint shaderProgramPVM;  //position of the Uniform (for the matrices) after linking
GLuint shaderProgramModelViewMatrix;
GLuint shaderProgramLightPosition;
GLuint shaderProgramNormalMatrix;
GLuint shaderProgramShadowMatrix;
GLuint shaderProgramShadowMap;
GLuint shaderProgramSubroutineIdx;


using glm::mat4;
using glm::vec3;
using glm::vec4;

vec3 Normal[118];
vec4 lightPosition;
mat4 shadowMatrix;


int en,v;
int shadowMapWidth=512;
int shadowMapHeight=512;

void load_obj(const std::string filename, std::vector<vec3> &vertices, std::vector<vec3> &normals, std::vector<GLushort> &elements){
	std::ifstream in(filename, std::ios::in);
	if (!in) { std::cerr << "Cannot open " << filename << std::endl; exit(1); }
	
	std::string line;
	while (getline(in, line)) {
    if (line.substr(0,2) == "v ") {
		std::istringstream s(line.substr(2));
      glm::vec3 v; s >> v.x; s >> v.y; s >> v.z;
      vertices.push_back(v);
    }  else if (line.substr(0,2) == "f ") {
		std::istringstream str(line.substr(2));
      int iv[4];
      int it;
	  std::string buf;
	  int i=0;
	  str >> buf;
	  while(buf.length()>1 &&i<4){
  		  sscanf_s(buf.c_str(),"%d/%d",&iv[i],&it);
		  str >> buf;
		  iv[i]--;
		  i++;
	  }
	  if(iv[2]!=iv[3]){
		  elements.push_back(iv[0]); elements.push_back(iv[1]); elements.push_back(iv[2]);
		  elements.push_back(iv[2]); elements.push_back(iv[3]); elements.push_back(iv[0]);
	  } else {
		  elements.push_back(iv[0]); elements.push_back(iv[1]); elements.push_back(iv[2]);
	  }
    } 
    else if (line.substr(0,1) == "#") { /* ignoring this line */ }
    else { /* ignoring this line */ }
  }
 
  normals.resize(vertices.size(), glm::vec3(0.0, 0.0, 0.0));
  for (int i = 0; i < elements.size(); i+=3) {
    GLushort ia = elements[i];
    GLushort ib = elements[i+1];
    GLushort ic = elements[i+2];
    glm::vec3 normal = glm::normalize(glm::cross(
      glm::vec3(vertices[ib]) - glm::vec3(vertices[ia]),
      glm::vec3(vertices[ic]) - glm::vec3(vertices[ia])));
    normals[ia] += normal;
	normals[ib] += normal;
	normals[ic] += normal;
  }
}

void init() {
    std::string shaderpath;
    std::string assetpath;	

    //find the path where the shaders are stored ("shaderpath" file created by cmake with configure_file)
    if (fileExists("shaderpath")) { //cmake使ってない人はシェーダーをbinary/.exeのそばにおいたらいい
        shaderpath = readTextFile("shaderpath");
        shaderpath = shaderpath.substr(0, shaderpath.size()-1); //cmakeいれる「\n」を出す
        shaderpath.append("/"); //stringの最後に

    }
	if (fileExists("assetpath")) { //cmake使ってない人はシェーダーをbinary/.exeのそばにおいたらいい
        assetpath = readTextFile("assetpath");
        assetpath = assetpath.substr(0, assetpath.size()-1); //cmakeいれる「\n」を出す
        assetpath.append("/"); //stringの最後に

    }

    //read our shaders from disk and upload them to opengl
    GLuint vertexShader = CreateShader(GL_VERTEX_SHADER,readTextFile(shaderpath+"shader.vert"));
    GLuint fragmentShader = CreateShader(GL_FRAGMENT_SHADER,readTextFile(shaderpath+"shader.frag"));

    //compile/link shader program from the two shaders
    shaderProgram = CreateProgram(vertexShader, fragmentShader);

    //Get the position of the Uniform (for the matrices) after linking
    shaderProgramPVM = glGetUniformLocation(shaderProgram,"pvm");
    shaderProgramModelViewMatrix = glGetUniformLocation(shaderProgram,"ModelViewMatrix");
    shaderProgramLightPosition = glGetUniformLocation(shaderProgram,"LightPosition");
	shaderProgramNormalMatrix = glGetUniformLocation(shaderProgram,"NormalMatrix");
	shaderProgramShadowMatrix = glGetUniformLocation(shaderProgram,"ShadowMatrix");
	shaderProgramSubroutineIdx = glGetUniformLocation(shaderProgram,"SubroutineIdx");

    //Prepare the matrices
    projectionMatrix = glm::perspective(60.0f, (float)WINDOW_SIZE_X/(float)WINDOW_SIZE_Y,0.1f,1000.0f); //create perspective matrix
    viewMatrix = glm::translate(glm::mat4(),vec3(0,0,-5)); //a matrix that translates the camera backwards (i.e. world forward)
	lightPosition =  vec4(4.0,12.0,6.0,1.0f);
	modelMatrix = glm::scale<float>(mat4(1.0f),glm::vec3(-0.03,0.03,0.03));

    //hold vertex data to upload to VBO's
    std::vector<vec3> vertices;
	std::vector<vec3> normals;
    std::vector<GLushort> elements;

	load_obj(assetpath + "teapot.obj",vertices,normals,elements);
	en = elements.size();
	
	v = vertices.size();
	vertices.push_back(vec3(-400.0,0.0,400.0));
	normals.push_back(vec3(0.0,1.0,0.0));
	vertices.push_back(vec3(-400.0,0.0,-400.0));
	normals.push_back(vec3(0.0,1.0,0.0));
	vertices.push_back(vec3(400.0,0.0,-400.0));
	normals.push_back(vec3(0.0,1.0,0.0));
	vertices.push_back(vec3(400.0,0.0,400.0));
	normals.push_back(vec3(0.0,1.0,0.0));
	
    //upload to GPU
    glGenVertexArrays(1,&modelVAO);
    glGenBuffers(1,&positionVBO);
    glGenBuffers(1,&elementsVBO);
    glGenBuffers(1,&normalVBO);

    //bind our model VAO
    glBindVertexArray(modelVAO);

    //bind the position VBO and upload data
    glBindBuffer(GL_ARRAY_BUFFER,positionVBO);
    glBufferData(GL_ARRAY_BUFFER,vertices.size() *3*sizeof(GLfloat),vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(vertexLoc,3,GL_FLOAT,GL_FALSE,0,0);
    glEnableVertexAttribArray(vertexLoc);

	/*
    //bind the color VBO and upload data
    glBindBuffer(GL_ARRAY_BUFFER,colorVBO);
    glBufferData(GL_ARRAY_BUFFER,colors.size()*3*sizeof(GLfloat),colors.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(colorLoc,3,GL_FLOAT,GL_FALSE,0,0);
    glEnableVertexAttribArray(colorLoc);
	*/

    //bind the color VBO and upload data
    glBindBuffer(GL_ARRAY_BUFFER,normalVBO);
    glBufferData(GL_ARRAY_BUFFER,normals.size()*3*sizeof(GLfloat),normals.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(normalLoc,3,GL_FLOAT,GL_FALSE,0,0);
    glEnableVertexAttribArray(normalLoc);

    //bind the color VBO and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,elementsVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,elements.size()*sizeof(unsigned short),elements.data(),GL_STATIC_DRAW);
    
    glBindVertexArray(0);



	//シャドウマップの作成
	GLfloat border[]={1.0f,0.0f,0.0f,0.0f};

	//シャドウマップテクスチャ
	GLuint depthTex;

	glGenTextures(1,&depthTex);
	glBindTexture(GL_TEXTURE_2D,depthTex);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,shadowMapWidth,shadowMapHeight,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,NULL);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC,GL_LESS);

	//シャドウマップをテクスチャチャンネル0に割り当てる
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D,depthTex);

	//FBO作成と設定
	glGenFramebuffers(1,&shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER,shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depthTex,0);
	GLenum drawBuffers[]={GL_NONE};
	glDrawBuffers(1,drawBuffers);

	shaderProgramShadowMap = glGetUniformLocation(shaderProgram,"ShadowMap");
	glUniform1i(shaderProgramShadowMap, 0);

    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( result == GL_FRAMEBUFFER_COMPLETE) {
        printf("Framebuffer is complete.\n");
    } else {
        printf("Framebuffer is not complete.\n");
    }

	//デフォルトのフレームバッファに戻す
	glBindFramebuffer(GL_FRAMEBUFFER,0);


	glEnable(GL_DEPTH_TEST);


}

void display() {

	mat4 bias = glm::mat4(vec4(0.5f,0.0f,0.0f,0.0f),
					  vec4(0.0f,0.5f,0.0f,0.0f),
					  vec4(0.0f,0.0f,0.5f,0.0f),
					  vec4(0.5f,0.5f,0.5f,1.0f));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glUseProgram(shaderProgram);  //select shader

	modelMatrix = glm::scale<float>(mat4(1.0f),glm::vec3(-0.03,0.03,0.03));
	viewMatrix = glm::translate(glm::mat4(),vec3(0,-1,-5)) * rotateMatrix_y * rotateMatrix_x;
	modelViewMatrix = viewMatrix*modelMatrix;

	//シャドウマップ
	glBindFramebuffer(GL_FRAMEBUFFER,shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
    glViewport(0,0,shadowMapWidth,shadowMapHeight);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	mat4 lightV = glm::lookAt(vec3(lightPosition),vec3(0.0),vec3(0,1,0));
	lightPV = glm::perspective(60.0f, (float)shadowMapWidth/(float)shadowMapHeight,1.f,100.0f) * lightV;
    glUniformMatrix4fv(shaderProgramPVM, 1, GL_FALSE, glm::value_ptr(lightPV * modelMatrix)); //upload it
	glUniform1i(shaderProgramSubroutineIdx,0);
    glUniformMatrix4fv(shaderProgramShadowMatrix, 1, GL_FALSE, glm::value_ptr(shadowMatrix));

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.5f,10.0f);
	glBindVertexArray(modelVAO);
	glDrawArrays(GL_QUADS,v,4);
	glDrawElements(GL_TRIANGLES, en, GL_UNSIGNED_SHORT, ((GLubyte *)NULL + (0)));

    glDisable(GL_POLYGON_OFFSET_FILL);
	/*
	translateMatrix1 = glm::translate<float>(mat4(1.0f),glm::vec3(0.f, 0.f, 2.5f));
	translateMatrix2 = glm::translate<float>(mat4(1.0f),glm::vec3(0.f, 0.f, -2.5f));
	*/

	glBindFramebuffer(GL_FRAMEBUFFER,0);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_GEN_Q);
	glDisable(GL_TEXTURE_2D);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0,0,WINDOW_SIZE_X,WINDOW_SIZE_Y);

    pvm = projectionMatrix * viewMatrix * modelMatrix; //calculate pvm
    glUniformMatrix4fv(shaderProgramPVM, 1, GL_FALSE, glm::value_ptr(pvm)); //upload it
	glUniformMatrix4fv(shaderProgramModelViewMatrix, 1, GL_FALSE, glm::value_ptr(modelViewMatrix));
	glUniform4fv(shaderProgramLightPosition, 1, glm::value_ptr(viewMatrix * lightPosition));
	glUniformMatrix3fv(shaderProgramNormalMatrix, 1, GL_FALSE, glm::value_ptr(glm::mat3(modelViewMatrix)));
	shadowMatrix = bias * lightPV * modelMatrix;
	glUniformMatrix4fv(shaderProgramShadowMatrix, 1, GL_FALSE, glm::value_ptr(shadowMatrix));
	glUniform1i(shaderProgramSubroutineIdx,1);


    glBindVertexArray(modelVAO); //select mode
	glDrawArrays(GL_QUADS,v,4);
	glDrawElements(GL_TRIANGLES, en, GL_UNSIGNED_SHORT, ((GLubyte *)NULL + (0)));


    glfwSwapBuffers(); //swap buffers for display
}


int main()
{
	float s=0.f ,r= 0.f;
    //init GLFW
    if (!glfwInit()==GL_TRUE) {  std::cout << "glfw initialization failed";  return 1;  }

    //attributes for the context 3.3
    glfwOpenWindowHint(GLFW_VERSION_MAJOR,3);
    glfwOpenWindowHint(GLFW_VERSION_MINOR,3);
    glfwOpenWindowHint(GLFW_FSAA_SAMPLES,8);

    //create the context
    if (!glfwOpenWindow(WINDOW_SIZE_X,WINDOW_SIZE_Y,0,0,0,0,0,0,GLFW_WINDOW))
    {
        std::cout << "GLFW Init WIndow Failed, OpenGL 3.3 not supported?" << std::endl;
    }

    glfwSetWindowPos(400,250); //window Position
    glfwSetWindowTitle("Hello OpenGL!");

    glewInit();
	glewExperimental = GL_TRUE;
    glEnable(GL_DEPTH_TEST);
	
    //our vertex/shader loading function
    init();

	//scaleMatrix = glm::scale<float>(mat4(1.0f),glm::vec3(s,s,1.f));

    //rendering loop
    while(true) {
        display();
        if(glfwGetKey(GLFW_KEY_ESC)) {
            break;
        }
			if(glfwGetKey(GLFW_KEY_UP)){
			s+=1.0;
			rotateMatrix_x = glm::rotate<float>(mat4(1.0f),s,glm::vec3(1.f,0.f,0.f));
		}
		if(glfwGetKey(GLFW_KEY_DOWN)){
			s-=1.0;
			rotateMatrix_x = glm::rotate<float>(mat4(1.0f),s,glm::vec3(1.f,0.f,0.f));
		}
		if(glfwGetKey(GLFW_KEY_RIGHT)){
			r+=1.0;
			rotateMatrix_y = glm::rotate<float>(mat4(1.0f),r,glm::vec3(0.f,1.f,0.f));
		}
		if(glfwGetKey(GLFW_KEY_LEFT)){
			r-=1.0;
			rotateMatrix_y = glm::rotate<float>(mat4(1.0f),r,glm::vec3(0.f,1.f,0.f));
		}
    }

    glfwTerminate();
    return 0;
}
