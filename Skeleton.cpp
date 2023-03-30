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
	layout(location = 0) in vec4 vp;	// Varying input: vp = vertex position is expected in attrib array 0
	void main() {
        gl_Position = vec4((vp.x) / (vp.w+1), (vp.y) / (vp.w+1), 0, 1);		// transform vp from modeling space to normalized device
        //gl_Position = vec4(vp.x, vp.y, 0, 1);		// transform vp from modeling space to normalized device
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
//Skalaris szorzat
float doth(vec4 a, vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + s * a.w * b.w;
}
//Megadja a hosszat
float lengthh(vec4 v) {
    return sqrtf(doth(v, v));
}
//Egyseghosszuva teszi
vec4 normalizeh(vec4 v) {
    return v * 1 / lengthh(v);
}

vec4 lerph(vec4 p, vec4 q, float t) {
    return p * (1 - t) + q * t;
}

vec4 slerph(vec4 p, vec4 q, float t) {
    float d = acos(dot(p, q)); // distance
    return p * (sin((1-t) * d) / sin(d)) + q * (sin(t * d) / sin(d));
}

float distanceh(vec4 p, vec4 q) {
    return acosh(-dot(p, q));
}

vec4 hlerph(vec4 p, vec4 q, float t) {
    float d = distanceh(p, q); // distance
    return p * (sinh((1-t) * d) / sinh(d)) + q * (sinh(t * d) / sinh(d));
}

vec4 transformToEuclidean(vec4 p) {
    return vec4(p.x / (p.w), p.y / (p.w), 0, 1);
}

vec4 transformToHyperbolic(vec4 p) {
    vec4 newP = p / sqrtf( 1 - powf(p.x, 2) - powf(p.y, 2));
    return newP;
}



// ---------------------------------------------------------------------------------------------

//pontok p*p=-1
bool isPoint(vec4 p) {
    return doth(p,p) == -1;
}

//vektor a p pontban p*v=0
bool isVector(vec4 p, vec4 v) {
    return doth(p, v) == 0;
}

//Egyenes, egységsebességű mozgás, r(t)=p*cosh(t)+v*sinh(t), ahol v a kezdősebesség és p a kezdőpont
vec4 calculatePosition(vec4 p, vec4 v, float t){
    return p * coshf(t) + v * sinhf(t);
}

vec4 calculateVelocity(vec4 p, vec4 v, float t){
    return p * sinhf(t) + v * coshf(t);
}

//Elfordítás, egységvektorok
vec4 calculateRotation(vec4 p, vec4 v, float t){
    return normalizeh(p * coshf(t) + v * sinhf(t));
}

//Merőleges
vec4 calculatePerpendicular(vec4 p, vec4 v, float t){
    return normalizeh(p * coshf(t) - v * sinhf(t));
}
bool isPerpendicular(vec4 v1, vec4 v2){ //   v * v = 0   és   v * p = 0
    return doth(v1, v2) == 0;
}

//Centrális pont visszavetítése
vec4 calculateReflection(vec4 p, vec4 v, float t){
    return normalizeh(p * coshf(t) - v * sinhf(t));
}

//Forgatás
vec4 rotate(vec4 v, float phi, vec4 axis) {
    // Calculate the perpendicular component of v
    vec4 v_perp = v - axis * (doth(v, axis) / doth(axis, axis));

    // Calculate the rotated vector
    vec4 result = v * cosh(phi) + v_perp * sinh(phi);

    return result;
}





struct Circle {
    unsigned int vao, vbo;
    std::vector<vec4> points;
    vec3 color;    // color
    vec4 center;  // center
    float R; 	   // radius

    Circle(vec4 _center, float _R, vec3 _color) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        center = _center; R = _R; color = _color;

       // float A2 = pow(_center.x, 2) + pow(_center.y, 2);

        //center = sqrtf(A2) / ( pow(cosh(R), 2) + (1-A2) + A2) ;


        //create points
        points.push_back(vec4(center.x, center.y, 0, 1));
        for (int i = 0; i < 360; i++) {
/*            float fi = (2.0f * i * M_PI) / 100;
            float x = cosf(fi);
            float y = sinf(fi);
            float w = sqrt(pow(x, 2)+pow(y, 2)+1);
            vec4* p = new vec4(cosf(fi) / (w+1), sinf(fi) / (w+1), 0, 1);*/

            float x = sin(i)* R + center.x;
            float y = cos(i)*R + center.y;
            float w = sqrt(pow(x, 2)+pow(y, 2)+1);
            vec4* p = new vec4(x / (w + 1), y / (w + 1), 0, 1);
            points.push_back(*p);
        }
    }

    void draw(){
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(vec4), &points[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glUniform3f(glGetUniformLocation(gpuProgram.getId(), "color"), color.x, color.y, color.z);
        glDrawArrays(GL_TRIANGLE_FAN, 0, points.size());
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
    vec4 center;
    float R;
    float mouthR;
    vec3 color;
    std::vector<vec4> goo;
    bool mouthSmaller;
    bool moveforward;
    bool turnleft;
    bool turnright;

    Hami(vec4 _center, float _R, vec3 _color){
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

    void create(vec4 _center, float _R, vec3 _color){
        body = new Circle(_center, _R, _color);
        eye1= new Circle(vec4(_center.x - _R / 2, _center.y + _R * 0.85f, 0, 1), _R / 4, vec3(1, 1, 1));
        eye2 = new Circle(vec4(_center.x + _R / 2, _center.y + _R * 0.85f, 0, 1), _R / 4, vec3(1, 1, 1));
        eye1center = new Circle(vec4(_center.x - _R / 2, _center.y + _R, 0, 1), _R / 8, vec3(0, 0, 0));
        eye2center = new Circle(vec4(_center.x + _R / 2, _center.y + _R, 0, 1), _R / 8, vec3(0, 0, 0));
        mouth = new Circle(vec4(_center.x, _center.y + _R, 0, 1), mouthR, vec3(0, 0, 0));
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
        goo.push_back(vec4(center.x, center.y, 0, 1));
        //Moving forward in the hyperbolic space
        center = center * coshf((float)time / 100000.0f) + vec4(0, -0.05f, 0, 1) * sinhf((float)time / 100000.0f);
        create(vec4(this->center.x, center.y, 0, 1), this->R, this->color);
        glutPostRedisplay();
    }

    void drawGoo(){
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, goo.size() * sizeof(vec4), &goo[0], GL_DYNAMIC_DRAW);
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
    hami1 = new Hami(vec4(0, 0, 0, 1), 0.15f, vec3(1, 0, 0));
    hami2 = new Hami(vec4(0.3f, 0, 0, 1), 0.08f, vec3(0, 1, 0));
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

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system

}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
    long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
    //call the hami animation function here
    //hami1->animate(time);
    //hami2->animate(time);
}