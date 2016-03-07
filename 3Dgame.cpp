#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

double player_x_coordinate=-2.0,player_y_coordinate=-2.0,player_z_coordinate=0.0;
double eye_x=0,eye_y=0,eye_z=3;
double playerJump=0;
int player_level=0;
double blockMove=0,onBlock=0;
// array of holes

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID, fontProgramID, textureProgramID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
  float intp;
  float fracp = modff(hue/60.0, &intp);
  float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

  if (hue < 60)
    return glm::vec3(1,x,0);
  else if (hue < 120)
    return glm::vec3(x,1,0);
  else if (hue < 180)
    return glm::vec3(0,1,x);
  else if (hue < 240)
    return glm::vec3(0,x,1);
  else if (hue < 300)
    return glm::vec3(x,0,1);
  else
    return glm::vec3(1,0,x);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;



/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_UP:
                player_y_coordinate += 0.5;
                break;
            case GLFW_KEY_DOWN:
                player_y_coordinate -= 0.5;
                break;
            case GLFW_KEY_LEFT:
            	player_x_coordinate -= 0.5;	
                // do something ..
                break;
            case GLFW_KEY_RIGHT:
            	player_x_coordinate += 0.5;	
                // do something ..
                break;
            case GLFW_KEY_R:
                player_x_coordinate = -2;
                player_y_coordinate = -2;
                player_z_coordinate = 0;
                break;
            case GLFW_KEY_1:  // 
            	eye_y = 0;
                eye_x = 0;
                eye_z = 3;
                break;
            case GLFW_KEY_2: // top view
            	eye_y = -2;
                eye_x = 0;
                eye_z = 2;
                break;
            case GLFW_KEY_3:
                eye_x = player_x_coordinate;
                eye_y = player_y_coordinate;
                eye_z = 2;
                break;
            case GLFW_KEY_SPACE:
                if (playerJump == 0)
                    playerJump = 4;
                break;

            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
                triangle_rot_dir *= -1;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                rectangle_rot_dir *= -1;
            }
            break;
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
     Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
   // Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

VAO *triangle, *rectangle, *cube[11][11], *player;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}

// Creates the rectangle object used in this sample code
void createRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -1.2,-1,0, // vertex 1
    1.2,-1,0, // vertex 2
    1.2, 1,0, // vertex 3

    1.2, 1,0, // vertex 3
    -1.2, 1,0, // vertex 4
    -1.2,-1,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createCube(double x_length,double y_length,double z_length,int x,int y)
{

	GLfloat vertex_buffer_data[] = {

     -x_length,-x_length,-x_length, // triangle 1 : begin

     -x_length,-x_length, x_length,

     -x_length, x_length, x_length, // triangle 1 : end

     x_length, x_length,-x_length, // triangle 2 : begin

     -x_length,-x_length,-x_length,

     -x_length, x_length,-x_length, // triangle 2 : end

     x_length,-x_length, x_length,

     -x_length,-x_length,-x_length,

     x_length,-x_length,-x_length,

     x_length, x_length,-x_length,

     x_length,-x_length,-x_length,

     -x_length,-x_length,-x_length,

     -x_length,-x_length,-x_length,

     -x_length, x_length, x_length,

     -x_length, x_length,-x_length,

     x_length,-x_length, x_length,

     -x_length,-x_length, x_length,

     -x_length,-x_length,-x_length,

     -x_length, x_length, x_length,

     -x_length,-x_length, x_length,

     x_length,-x_length, x_length,

     x_length, x_length, x_length,

     x_length,-x_length,-x_length,

     x_length, x_length,-x_length,

     x_length,-x_length,-x_length,

     x_length, x_length, x_length,

     x_length,-x_length, x_length,

     x_length, x_length, x_length,

     x_length, x_length,-x_length,

     -x_length, x_length,-x_length,

     x_length, x_length, x_length,

     -x_length, x_length,-x_length,

     -x_length, x_length, x_length,

     x_length, x_length, x_length,

     -x_length, x_length, x_length,

     x_length,-x_length, x_length,

 };

 GLfloat color_buffer_data[] = {

     0.583f,  0.771f,  0.014f,

     0.009f,  0.115f,  0.436f,

     0.327f,  0.483f,  0.844f,

     0.822f,  0.569f,  0.201f,

     0.435f,  0.002f,  0.223f,

     0.310f,  0.747f,  0.185f,

     0.597f,  0.770f,  0.761f,

     0.559f,  0.436f,  0.730f,

     0.359f,  0.583f,  0.152f,

     0.483f,  0.596f,  0.789f,

     0.559f,  0.861f,  0.039f,

     0.195f,  0.548f,  0.859f,

     0.014f,  0.184f,  0.576f,

     0.771f,  0.328f,  0.970f,

     0.406f,  0.015f,  0.116f,

     0.076f,  0.977f,  0.133f,

     0.971f,  0.572f,  0.833f,

     0.140f,  0.016f,  0.489f,

     0.997f,  0.513f,  0.064f,

     0.945f,  0.719f,  0.592f,

     0.543f,  0.021f,  0.978f,

     0.279f,  0.317f,  0.505f,

     0.167f,  0.020f,  0.077f,

     0.347f,  0.857f,  0.137f,

     0.055f,  0.953f,  0.042f,

     0.714f,  0.505f,  0.345f,

     0.783f,  0.290f,  0.734f,

     0.722f,  0.045f,  0.174f,

     0.302f,  0.455f,  0.848f,

     0.225f,  0.587f,  0.040f,

     0.517f,  0.713f,  0.338f,

     0.053f,  0.959f,  0.120f,

     0.393f,  0.021f,  0.362f,

     0.073f,  0.211f,  0.457f,

     0.820f,  0.883f,  0.371f,

     0.982f,  0.099f,  0.879f

 };

 cube[x][y] = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createPlayer(double x_length,double y_length,double z_length)
{
	GLfloat vertex_buffer_data[] = {

     -x_length,-x_length,-x_length, // triangle 1 : begin

     -x_length,-x_length, x_length,

     -x_length, x_length, x_length, // triangle 1 : end

     x_length, x_length,-x_length, // triangle 2 : begin

     -x_length,-x_length,-x_length,

     -x_length, x_length,-x_length, // triangle 2 : end

     x_length,-x_length, x_length,

     -x_length,-x_length,-x_length,

     x_length,-x_length,-x_length,

     x_length, x_length,-x_length,

     x_length,-x_length,-x_length,

     -x_length,-x_length,-x_length,

     -x_length,-x_length,-x_length,

     -x_length, x_length, x_length,

     -x_length, x_length,-x_length,

     x_length,-x_length, x_length,

     -x_length,-x_length, x_length,

     -x_length,-x_length,-x_length,

     -x_length, x_length, x_length,

     -x_length,-x_length, x_length,

     x_length,-x_length, x_length,

     x_length, x_length, x_length,

     x_length,-x_length,-x_length,

     x_length, x_length,-x_length,

     x_length,-x_length,-x_length,

     x_length, x_length, x_length,

     x_length,-x_length, x_length,

     x_length, x_length, x_length,

     x_length, x_length,-x_length,

     -x_length, x_length,-x_length,

     x_length, x_length, x_length,

     -x_length, x_length,-x_length,

     -x_length, x_length, x_length,

     x_length, x_length, x_length,

     -x_length, x_length, x_length,

     x_length,-x_length, x_length,

 };

 GLfloat color_buffer_data[] = {

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0,

     0.0,  0.0,  0.0

 };

 player = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);	

}


float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye (eye_x, eye_y, eye_z);
  //glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 1);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(eye, target, up); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  // Matrices.model = glm::mat4(1.0f);

  // /* Render your scene */

  // glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
  // glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  // glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  // Matrices.model *= triangleTransform; 
  // MVP = VP * Matrices.model; // MVP = p * V * M



  //  Don't change unless you are sure!!
  // glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // // draw3DObject draws the VAO given to it using current MVP matrix
  // draw3DObject(triangle);

  // // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // // glPopMatrix ();
  // Matrices.model = glm::mat4(1.0f);

  // glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
  // glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  // Matrices.model *= (translateRectangle * rotateRectangle);
  // MVP = VP * Matrices.model;
  // glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // // draw3DObject draws the VAO given to it using current MVP matrix
  // draw3DObject(rectangle);
  	glm::mat4 translateCube[11][11];
  	glm::mat4 rotateCube[11][11];


// hole at i=3,j=4 and i=6,j=5 and i=7,j=7
// tiles moving up and down at i=2,j=6 and i=5,j=8  	
    double holes[10][2] = { {3,4}, {7,7}, {6,5} };          //contains info about holes 
    double movingBlocks[10][2] = { {5,5}, {8,7}, {9,8} };


	for (int i=0;i<10;i++){
		for (int j=0;j<10;j++)	
		{
		  Matrices.model = glm::mat4(1.0f);
		  translateCube[i][j] = glm::translate (glm::vec3(-2+i/2.0f, -2+j/2.0f, 0));        // glTranslatef
            for (int k=0;k<3;k++)   
                if (holes[k][0] == i && holes[k][1] == j)
                    translateCube[i][j] = glm::translate (glm::vec3(-2+i/2.0f, -2+j/2.0f, -1));        // glTranslatef\

            for (int k=0;k<3;k++)   
                if (movingBlocks[k][0] == i && movingBlocks[k][1] == j)
                    translateCube[i][j] = glm::translate (glm::vec3(-2+i/2.0f, -2+j/2.0f, (blockMove - 1.0f )));       // glTranslatef\

        rotateCube[i][j] = glm::rotate((float)(0*rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)

		  Matrices.model *= (translateCube[i][j] * rotateCube[i][j]);
		  MVP = VP * Matrices.model;
		  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
		  draw3DObject(cube[i][j]);
		}
	}
    
    for (int i=0;i<3;i++)
    {
        if ( (holes[i][0]/2.0f - 2) == player_x_coordinate && (holes[i][1]/2.0f - 2) == player_y_coordinate )
            player_z_coordinate = -1;
        if ( (movingBlocks[i][0]/2.0f - 2) == player_x_coordinate && 
            (movingBlocks[i][1]/2.0f - 2) == player_y_coordinate )
            if (player_z_coordinate < blockMove)
                player_x_coordinate = player_y_coordinate = -2;
            else
            {
               onBlock = 1;
               int blockNo = i; 
               player_z_coordinate = 1;
            }
    }


	// if (player_x_coordinate == -0.5 && player_y_coordinate==0)
		// player_z_coordinate = -1;
	// if (player_x_coordinate == 1.5 && player_y_coordinate==1.5)
		// player_z_coordinate = -1;
	// if (player_x_coordinate == 1 && player_y_coordinate==0.5)
		// player_z_coordinate = -1;


	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translatePlayer = glm::translate (glm::vec3(player_x_coordinate, player_y_coordinate, player_z_coordinate));        // glTranslatef
	glm::mat4 rotatePlayer = glm::rotate((float)(0*rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translatePlayer * rotatePlayer);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	draw3DObject(player);


  // Increment angles
  float increments = 1;

  //camera_rotation_angle++; // Simulating camera rotation
  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
    for (int i=0;i<10;i++){
		for (int j=0;j<10;j++){
			createCube(0.2,1,1,i,j);
		}
	}
	createPlayer(0.2,1,1);
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

void dontLetOut()
{
    if (player_x_coordinate < -2 )
        player_x_coordinate = -2;
    if (player_y_coordinate < -2)
        player_y_coordinate = -2;
    if (player_x_coordinate > 2.5 )
        player_x_coordinate = 2.5;
    if (player_y_coordinate > 2.5)
        player_y_coordinate = 2.5;
    
}


int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;

    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        dontLetOut();
        // OpenGL Draw commands
        draw();

        // if (player_z_coordinate == -1)
            // quit(window);

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        int countb=1;

        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            if (playerJump > 0)
            {
                if (playerJump > 2)
                    player_z_coordinate += 0.4;
                else
                    player_z_coordinate -= 0.4;

                playerJump -= 1;
            }
//            if (countb % 20 == 0)
            {
                blockMove += 0.2f;
                if (blockMove > 2)
                    blockMove = 0;

            }

            printf("player level :%d\n",player_level);
            last_update_time = current_time;
        }

    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
