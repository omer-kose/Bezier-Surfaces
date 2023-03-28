//OpenGL Core Libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>



//My headers
#include "Utilities.h"
#include "Shader.h"
#include "Camera.h"


//Utility Headers
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>



//Camera
Camera camera(glm::vec3(0.0f, 0.0f, 2.0f));


//Time parameters
double deltaTime = 0.0;
double lastFrame = 0.0;

//Window
GLFWwindow* window;

//Light data
std::vector<glm::vec3> lightPositions;
std::vector<glm::vec3> lightIntensities;


struct BezierSurface
{
    glm::vec3 P[16]; //Control points
    std::vector<glm::vec2> uv; //Sample points' (u, v) coordinates
    std::vector<glm::ivec3> tris;
    glm::vec3 translation;
    glm::vec3 scaling; //Scaling of each Bezier Surface is the same but anyway
    //OpenGL Params
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
};


//Scene Properies
std::vector<BezierSurface> bezierSurfaces;
int numPx; //Number of horizontal control points
int numPy; //Number of vertical control points
std::vector<std::vector<float>> CP;
float coordMultiplier = 1.0f;
int numSamples = 10;
float rotationAngle = -30.0f;


void updateDeltaTime()
{
	double currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}

//Callback function in case of resizing the window
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}



//Callback function for mouse position inputs
void mouse_callback(GLFWwindow* window, double xPos, double yPos)
{
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		camera.processMouseMovement(xPos, yPos, GL_TRUE);
	}

	camera.setLastX(xPos);
	camera.setLastY(yPos);	
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
{
	camera.processMouseScroll(yOffset);
}


int setupDependencies()
{
	glfwInit();
	//Specify the version and the OpenGL profile. We are using version 3.3
	//Note that these functions set features for the next call of glfwCreateWindow
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

	//Create the window object
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Window", NULL, NULL);
	if (window == nullptr)
	{
		std::cout << "Failed to create the window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }
    
	//Specify the actual window rectangle for renderings.
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	//Register our size callback funtion to GLFW.
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	//GLFW will capture the mouse and will hide the cursor
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//Configure Global OpenGL State
	glEnable(GL_DEPTH_TEST);
	return 0;
}



//For testing purposes.
GLuint testRectangle()
{
	GLfloat vertices[] = 
	{
		 0.5f,  0.5f, 0.0f,  // top right
		 0.5f, -0.5f, 0.0f,  // bottom right
		-0.5f, -0.5f, 0.0f,  // bottom left
		-0.5f,  0.5f, 0.0f   // top left 
	};

	GLuint indices[] = 
	{  
		2, 1, 0,  // first Triangle
		2, 0, 3   // second Triangle
	};

	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	//Bind VAO
	glBindVertexArray(VAO);
	//Bind VBO, send data
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	//Bind EBO, send indices 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	
	//Configure Vertex Attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);

	//Data passing and configuration is done 
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

void renderTestRectangle(GLuint VAO, Shader shader)
{
	shader.use();
	glm::mat4 view = camera.getViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera.getFov()), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 PV = projection * view;
	shader.setMat4("PVM", PV * model);
	glBindVertexArray(VAO);
	//total 6 indices since we have triangles
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

}


/*
    Triangulates the surface and sets the (u, v) coordinates required for computations.
*/
void triangulate(BezierSurface& surf)
{
    //Clear uv and triangles as this function can be called multiple times in the program
    surf.uv.clear();
    surf.tris.clear();
    int si; //Sample index (temporary var)
    float sampleSpacing = numSamples - 1;
    surf.uv.reserve(numSamples * numSamples);
    surf.tris.reserve(2 * (numSamples-1) * (numSamples-1));
    //Create samples and triangulate at the same time
    for(int i = 0; i < numSamples; ++i)
    {
        for(int j = 0; j < numSamples; ++j)
        {
            //Create the sample
            surf.uv.push_back(glm::vec2(j / sampleSpacing, i / sampleSpacing));
            si = i * numSamples + j;
            if((i != numSamples - 1) && (j != numSamples - 1))
            {
                //Counter-clockwise orientation
                surf.tris.push_back(glm::ivec3(si, si + numSamples, si+numSamples+1));
                surf.tris.push_back(glm::ivec3(si, si + numSamples + 1, si + 1));
            }
        }
    }
}


void createOpenGLObjects(BezierSurface& surf)
{
    glGenVertexArrays(1, &surf.VAO);
    glGenBuffers(1, &surf.VBO);
    glGenBuffers(1, &surf.EBO);
}


void setupOpenGLBuffers(BezierSurface& surf)
{
    //Bind VAO
    glBindVertexArray(surf.VAO);
    //Bind VBO and send the data
    glBindBuffer(GL_ARRAY_BUFFER, surf.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * surf.uv.size(), surf.uv.data(), GL_STATIC_DRAW);
    
    //Bind EBO and send theindices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surf.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::ivec3) * surf.tris.size(), surf.tris.data(), GL_STATIC_DRAW);
    
    //Configure vertex attributes
    //UV
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    
    //Data passign and configuration is done
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


glm::vec3 determineBezierTileOffset()
{
    int numBezierX = numPx / 4;
    int numBezierY = numPy / 4;
    float s = coordMultiplier / std::max(numBezierY, numBezierX);
    glm::vec3 offset;
    if(numBezierX == numBezierY) //If the whole surface is square
    {
        offset = glm::vec3(0.5 * s - coordMultiplier / 2, coordMultiplier / 2 - 0.5 * s, 0.0);
    }
    else //Non-square
    {
        if(numBezierX > numBezierY) //X dominated case
        {
            offset = glm::vec3(0.5 * s - coordMultiplier / 2, -coordMultiplier / 2 + s * numBezierY - 0.5 * s, 0.0);
        }
        else //Y dominated case
        {
            offset = glm::vec3(0.5 * s - coordMultiplier / 2, coordMultiplier / 2 - 0.5 * s, 0.0);
        }
    }
    
    return offset;
}

void createBezierSurfaces()
{
    //Each 4x4 subblock represents a Bezier Surface iterate through and create the Surfaces
    float spacing = 1.0 / 3.0; //Spacing between the CP's in XY plane.
    //Number of Bezier surfaces along each axis
    int numBezierX = numPx / 4;
    int numBezierY = numPy / 4;
    //Scaling of each bezier surface. This is also equal to the side length of each surface.
    //Each surface has the same length and uniformly squared. So, each surface is actually
    //a square
    float s = coordMultiplier / std::max(numBezierY, numBezierX);
    glm::vec3 offset = determineBezierTileOffset(); //Offset to map the first surface to the top left.
    bezierSurfaces.reserve(numBezierX * numBezierY);
    for(int i = 0; i < numBezierY; ++i)
    {
        for(int j = 0; j < numBezierX; ++j)
        {
            BezierSurface surf;
            //Read 16 CP's and determine XY coordinates by partitioning the surface uniformly
            //Global indices
            int I = 4*i;
            int J = 4*j;
            int k = 0;
            for(int v = 0; v < 4; ++v)
            {
                for(int u = 0; u < 4; ++u)
                {
                    float z = CP[I + v][J + u];
                    //Uniformly lay out the Control Points. Also, centralize them around origin.
                    surf.P[k] = glm::vec3(u * spacing - 0.5, 0.5 - v * spacing, z);
                    ++k;
                }
            }
            
            //Set the scaling
            surf.scaling = glm::vec3(s, s, 1.0);
            //Set the translation to send the current bezier surface to the correct place
            surf.translation = offset + glm::vec3(j * s, -i * s, 0.0);
            bezierSurfaces.push_back(surf);
        }
    }
}

/*
    Reads the file and initializes the data structs needed
*/
void initScene(const char* fileName)
{
    std::ifstream fileStream(fileName);
    //Number of point lights
    int numPointLights;
    fileStream >> numPointLights;
    //Initialize the points lights
    lightPositions.resize(numPointLights);
    lightIntensities.resize(numPointLights);
    for(int i = 0; i < numPointLights; ++i)
    {
        fileStream >> lightPositions[i].x;
        fileStream >> lightPositions[i].y;
        fileStream >> lightPositions[i].z;
        fileStream >> lightIntensities[i].x;
        fileStream >> lightIntensities[i].y;
        fileStream >> lightIntensities[i].z;
    }
    //Number of CP's
    fileStream >> numPy >> numPx;
    //Now read the Control Points
    CP.resize(numPy);
    for(int i = 0; i < numPy; ++i)
    {
        CP[i].resize(numPx);
        for(int j = 0; j < numPx; ++j)
        {
            fileStream >> CP[i][j];
        }
    }
    
    createBezierSurfaces();
    
    //Triangulate surfaces and setup their OpenGL data
    for(int i = 0; i < bezierSurfaces.size(); ++i)
    {
        triangulate(bezierSurfaces[i]);
        createOpenGLObjects(bezierSurfaces[i]);
        setupOpenGLBuffers(bezierSurfaces[i]);
    }
    
}

/*
    Renders a single bezier surface
*/
void renderBezierSurface(BezierSurface& surf, Shader& shader, int i)
{
    shader.use();
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.getFov()), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f);
    //Transformation order: Scale-Translate-Rotate (rotation should be at the end this time)
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(1.0, 0.0, 0.0));
    model = glm::translate(model, surf.translation);
    model = glm::scale(model, surf.scaling);
    glm::mat4 PV = projection * view;
    //Set uniforms
    //Vertex Shader uniforms
    shader.setMat4("modelMat", model);
    shader.setMat4("PV", PV);
    shader.setVec3Array("P", 16, surf.P[0]);
    //glUniform3fv(glGetUniformLocation(shader.getID(), "P"), 16, glm::value_ptr(surf.P[0]));
    //Fragment Shader uniforms
    shader.setVec3("eyePos", camera.getPosition());
    shader.setInt("numLights", (int)lightPositions.size());
    shader.setVec3Array("lightPositions", (int)lightPositions.size(), lightPositions[0]);
    shader.setVec3Array("lightIntensities", (int)lightIntensities.size(), lightIntensities[0]);
    glBindVertexArray(surf.VAO);
    //total 6 indices since we have triangles
    glDrawElements(GL_TRIANGLES, 3 * surf.tris.size(), GL_UNSIGNED_INT, 0);
}


//Function that will process the inputs, such as keyboard inputs
void processInput(GLFWwindow* window)
{
    //If pressed glfwGetKey return GLFW_PRESS, if not it returns GLFW_RELEASE
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    //Sample size controller
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        numSamples = std::min(numSamples+2, 80);
        //Changing numSamples changes the triangulation and thus the OpenGL Buffers.
        for(int i = 0; i < bezierSurfaces.size(); ++i)
        {
            triangulate(bezierSurfaces[i]);
            setupOpenGLBuffers(bezierSurfaces[i]);
        }
        
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        numSamples = std::max(numSamples-2, 2);
        for(int i = 0; i < bezierSurfaces.size(); ++i)
        {
            triangulate(bezierSurfaces[i]);
            setupOpenGLBuffers(bezierSurfaces[i]);
        }
    }
    
    //Size controller
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        coordMultiplier += 0.1;
        //Update the offset and the scale
        int numBezierX = numPx / 4;
        int numBezierY = numPy / 4;
        //Scaling of each bezier surface. This is also equal to the side length of each surface.
        //Each surface has the same length and uniformly squared. So, each surface is actually
        //a square
        float s = coordMultiplier / std::max(numBezierY, numBezierX);
        glm::vec3 offset = determineBezierTileOffset();
        for(int i = 0; i < numBezierY; ++i)
        {
            for(int j = 0; j < numBezierX; ++j)
            {
                BezierSurface& surf = bezierSurfaces[i * numBezierY + j];
                //Set the scaling
                surf.scaling = glm::vec3(s, s, 1.0);
                //Set the translation to send the current bezier surface to the correct place
                surf.translation = offset + glm::vec3(j * s, -i * s, 0.0);
            }
        }
        
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        coordMultiplier = std::max(coordMultiplier - 0.1, 0.1);
        //Update the offset and the scale
        int numBezierX = numPx / 4;
        int numBezierY = numPy / 4;
        //Scaling of each bezier surface. This is also equal to the side length of each surface.
        //Each surface has the same length and uniformly squared. So, each surface is actually
        //a square
        float s = coordMultiplier / std::max(numBezierY, numBezierX);
        glm::vec3 offset = determineBezierTileOffset();
        for(int i = 0; i < numBezierY; ++i)
        {
            for(int j = 0; j < numBezierX; ++j)
            {
                BezierSurface& surf = bezierSurfaces[i * numBezierY + j];
                //Set the scaling
                surf.scaling = glm::vec3(s, s, 1.0);
                //Set the translation to send the current bezier surface to the correct place
                surf.translation = offset + glm::vec3(j * s, -i * s, 0.0);
            }
        }
    }
    
    
    //Rotation Controller
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        rotationAngle += 10.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
    {
        rotationAngle -= 10.0f;
    }

}


int main()
{
	setupDependencies();
	Shader shader("Shaders/bezier/bezier.vert",
                  "Shaders/bezier/bezier.frag");


    initScene("input3.txt");
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		//Update deltaTime
		updateDeltaTime();
		// input
		processInput(window);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        for(int i = 0; i < bezierSurfaces.size(); ++i)
        {
            renderBezierSurface(bezierSurfaces[i], shader, i);
        }
        
		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}


	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();

	return 0;
}
