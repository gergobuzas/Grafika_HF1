//=============================================================================================
// Mintaprogram: Z�ld h�romsz�g. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!!
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Buzas Gergo
// Neptun : E0PWAX
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 5);		// transform vp from modeling space to normalized device space
        //float w = sqrt(vp.x * vp.x + vp.y * vp.y + 1);
        //gl_Position = vec4((vp.x) / w, (vp.y) / w, 0, 1);		// transform vp from modeling space to normalized device
    }
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers

	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";


GPUProgram gpuProgram; // vertex and fragment shaders

unsigned int vao;	   // virtual world on the GPU
float s = -1; // 1: Euclidean; -1: Minkowski == hyperbolic
float Hdot(vec4 a, vec4 b) { // affects length and normalize
    return a.x * b.x + a.y * b.y + a.z * b.z + s * a.w * b.w;
}
float Hlength(vec4 v) { return sqrtf(dot(v, v)); }
vec4 Hnormalize(vec4 v) { return v * 1/Hlength(v); }
vec4 Hlerp(vec4 p, vec4 q, float t) { return p * (1-t) + q * t; }
vec4 Hslerp(vec4 p, vec4 q, float t) {
    float d = acos(dot(p, q)); // distance
    return p * (sin((1-t) * d) / sin(d)) + q * (sin(t * d) / sin(d));
}


vec4 Hhlerp(vec4 p, vec4 q, float t) {
    float d = acosh(-dot(p, q)); // distance
    return p * (sinh((1-t) * d) / sinh(d)) + q * (sinh(t * d) / sinh(d));
}

//Taken from: http://cg.iit.bme.hu/portal/sites/default/files/oktatott%20t%C3%A1rgyak/sz%C3%A1m%C3%ADt%C3%B3g%C3%A9pes%20grafika/2d%20k%C3%A9pszint%C3%A9zis/grafika2d.pdf
struct Circle {
    unsigned int vao, vbo;
    std::vector<vec2> points;
    vec3 color;    // color
    vec2 center;  // center
    float R; 	   // radius

    Circle(vec2 _center, float _R, vec3 _color) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        center = _center; R = _R; color = _color;

        //create points
        points.push_back(vec2(center.x, center.y));
        for (int i = 0; i < 360; i++) {
            points.push_back(vec2(sin(i) * R + center.x, cos(i) * R + center.y));
        }
    }

    void draw(){
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(vec2), &points[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glUniform3f(glGetUniformLocation(gpuProgram.getId(), "color"), color.x, color.y, color.z);
        glDrawArrays(GL_TRIANGLE_FAN, 0, points.size());
    }

    bool In(vec2 r) {
        vec2 newR = r - center;
        return (dot(newR, newR) - R * R < 0);
    }

    ~Circle(){
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
};


struct Hami{
    unsigned int vao, vbo;
    Circle* eye1;
    Circle* eye1center;
    Circle* eye2;
    Circle* eye2center;
    Circle* mouth;
    Circle* body;
    vec2 center;
    float R;
    float mouthR;
    vec3 color;
    std::vector<vec2> goo;
    bool mouthSmaller;
    bool moveforward;
    bool turnleft;
    bool turnright;

    Hami(vec2 _center, float _R, vec3 _color){
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        this->center = _center;
        this->R = _R;
        this->color = _color;
        this->mouthR = _R / 4;
        this->mouthSmaller = true;
        this->moveforward = false;
        this->turnleft = false;
        this->turnright = false;
        create(center, R, color);
    }

    void create(vec2 _center, float _R, vec3 _color){
        body = new Circle(_center, _R, _color);
        eye1= new Circle(vec2(_center.x - _R / 2, _center.y + _R * 0.85f), _R / 4, vec3(1, 1, 1));
        eye2 = new Circle(vec2(_center.x + _R / 2, _center.y + _R * 0.85f), _R / 4, vec3(1, 1, 1));
        eye1center = new Circle(vec2(_center.x - _R / 2, _center.y + _R), _R / 8, vec3(0, 0, 0));
        eye2center = new Circle(vec2(_center.x + _R / 2, _center.y + _R), _R / 8, vec3(0, 0, 0));
        mouth = new Circle(vec2(_center.x, _center.y + _R), mouthR, vec3(0, 0, 0));
    }

    void draw(){
        this->mouthSizeChange();
        this->drawGoo();
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        body->draw();
        mouth->draw();
        eye1->draw();
        eye1center->draw();
        eye2->draw();
        eye2center->draw();


    }

    void animate(long time){
        delete body;
        delete eye1;
        delete eye1center;
        delete eye2;
        delete eye2center;
        delete mouth;
        goo.push_back(vec2(center.x, center.y));
        //Moving forward in the hyperbolic space
        center = center * coshf((float)time / 100000.0f) + vec2(0, -0.05) * sinhf((float)time / 100000.0f);
        create(vec2(this->center.x, center.y), this->R, this->color);
        glutPostRedisplay();
    }

    void drawGoo(){
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, goo.size() * sizeof(vec2), &goo[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glUniform3f(glGetUniformLocation(gpuProgram.getId(), "color"), 1, 1, 1);
        glLineWidth(5);
        glDrawArrays(GL_LINE_STRIP, 0, goo.size());
    }

    void mouthSizeChange(){
        if (mouthSmaller) {
            mouthR -= 0.00002f;
            if (mouthR <= R / 10) {
                mouthSmaller = false;
            }
        }
        else {
            mouthR += 0.00002f;
            if (mouthR >= R / 3) {
                mouthSmaller = true;
            }
        }
    }

};

Hami* hami1;
Hami* hami2;


// Initialization, create an OpenGL context
void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);
    hami1 = new Hami(vec2(0, 0), 0.15f, vec3(1, 0, 0));
    hami2 = new Hami(vec2(0.3f, 0), 0.08f, vec3(0, 1, 0));
    // create program for the GPU
    gpuProgram.create(vertexSource, fragmentSource, "outColor");
}


// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0, 0, 0, 0);     // background color
    glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

    hami1->draw();
    hami2->draw();
    glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == 'e') hami1->moveforward = true;         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
    if (key == 'e') hami1->moveforward = false;         // if d, invalidate display, i.e. redraw
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
    // Convert to normalized device space
    float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
    float cY = 1.0f - 2.0f * pY / windowHeight;
    printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
    // Convert to normalized device space
    float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
    float cY = 1.0f - 2.0f * pY / windowHeight;

    char * buttonStat;
    switch (state) {
        case GLUT_DOWN: buttonStat = "pressed"; break;
        case GLUT_UP:   buttonStat = "released"; break;
    }

    switch (button) {
        case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
        case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
        case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
    }
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
    long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
    //call the hami animation function here
    hami1->animate(time);
    hami2->animate(time);
}
