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
    float lengthV = 1.0f /  lengthh(v);
    vec4 newV =  v * lengthV;
    return newV;
}
vec4 lerph(vec4 p, vec4 q, float t) {
    return p * (1 - t) + q * t;
}
vec4 slerph(vec4 p, vec4 q, float t) {
    float d = acos(dot(p, q)); // distance
    return p * (sin((1-t) * d) / sin(d)) + q * (sin(t * d) / sin(d));
}
float distanceh(vec4 p, vec4 q) {
    return acosh(-doth(p, q));
}
vec4 hlerph(vec4 p, vec4 q, float t) {
    float d = distanceh(p, q); // distance
    return p * (sinh((1-t) * d) / sinh(d)) + q * (sinh(t * d) / sinh(d));
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
bool isPerpendicular(vec4 v1, vec4 v2){ //   v * v = 0   és   v * p = 0
    return doth(v1, v2) == 0;
}
//create a perpendicular vector ------- 1. Egy irányra merőleges irány állítása.
vec4 perpendicularVector(vec4 p, vec4 v) {
    vec3 newp = vec3(p.x, p.y, -p.w);
    vec3 newv = vec3(v.x, v.y, -v.w);
    vec3 returnedVec3 = cross(newp, newv);
    return vec4(returnedVec3.x, returnedVec3.y, 0, returnedVec3.z);
}

//Egy 𝒒 pont iránya és távolsága: 𝒒 = 𝒑 cosh 𝑡 + 𝒗0 sinh 𝑡  -------- 2. Adott pontból és sebesség vektorral induló pont helyének és sebesség vektorának számítása t idővel később.
vec4 pointAtTime(vec4 p, vec4 v, float t) {
    return p * coshf(t) + v * sinhf(t); //returns p
}
//Lépés 𝑡 távolságra ---- Iranyvektort ad, a pointAtTime derivaltja
vec4 velocityAtTime(vec4 p, vec4 v, float t) {
    return p * sinh(t) + v * cosh(t); //returns v
}

//3. Egy ponthoz képest egy másik pont irányának és távolságá
//   Egy olyan vektor kell, amivel p*v=0;
/*
vec4 directionAndDistance(vec4 p, vec4 q) {
    return p * cosh(distanceh(p, q)) + q * sinh(distanceh(p, q));
}*/

//4. Egy ponthoz képest adott irányban és távolságra lévő pont előállítása.  float distanceh(vec4 p, vec4 q) { return acosh(-dot(p, q)); }
/*vec4 pointAtDirectionAndDistance(vec4 p, vec4 v) {  //here we use the returned p and vs
    float d = distanceh(p, v);
    vec4 newp = (pointAtTime(p, normalizeh(v), d));
    printf("newp: %f, %f, %f, %f", newp.x, newp.y, newp.z, newp.w);
    return newp;
}*/
//5. Egy pontban egy vektor elforgatása adott szöggel. ----- Elfordítás (egységvektorok): 𝒗∗= 𝒗 cos 𝜑 + 𝒗⊥sin(𝜑)
vec4 rotateVector(vec4 p, vec4 v, float angle) {
    vec4 vPerp = normalizeh(perpendicularVector(p, v));
    return v * cosf(angle) + vPerp * sinf(angle);
}
//6. Egy közelítő pont és sebességvektorhoz a geometria szabályait teljesítő, közeli pont és sebesség választása.
/*vec4 closestPoint(vec4 p, vec4 v) {
    return p * coshf(1) + v * sinhf(1);
}*/


struct Circle {
    unsigned int vao, vbo;
    std::vector<vec4> points;
    vec3 color;    // color
    vec4 center;  // center
    vec4 direction; // direction
    float R; 	   // radius

    Circle(vec2 _center, float _R, vec3 _color) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        R = _R; color = _color;
        center = vec4(_center.x, _center.y, 0, sqrtf(_center.x * _center.x + _center.y * _center.y + 1.0f));
        direction = vec4(0, 1, 0, (0*center.x + 1*center.y) / center.w);
        create();
    }
    Circle(vec4 _center, float _R, vec3 _color) {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        R = _R; color = _color;
        center = _center;
        direction = vec4(0, 1, 0, (0*center.x + 1*center.y) / center.w);
        create();
    }

    void create() {
        points.clear();
        vec4 v = direction;
        v = normalizeh(v);
        vec4 vectorNext = v;
        vec4 pointOnCircleEdge;
        for (size_t i = 0; i <= 1000; i++)
        {
            pointOnCircleEdge = pointAtTime(center, vectorNext, R);
            points.push_back(pointOnCircleEdge);
            vectorNext = rotateVector(center, vectorNext, (2.0f * M_PI / 1000.0f) * i);
            vectorNext = normalizeh(vectorNext);
        }
    }
    void draw(){
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(vec4), &points[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
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
    vec4 direction;
    std::vector<vec4> goo;
    bool mouthSmaller;
    bool moveforward;
    bool turnleft;
    bool turnright;

    Hami(vec2 _center, float _R, vec3 _color){
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        this->center = vec4(_center.x, _center.y, 0, sqrtf(center.x * center.x + center.y * center.y + 1.0f));
        this->R = _R;
        this->color = _color;
        this->mouthR = _R / 4;
        this->mouthSmaller = true;
        this->moveforward = false;
        this->turnleft = false;
        this->turnright = false;
        this->direction = vec4(0, 1, 0, (0*center.x + 1*center.y) / center.w);
        this->direction = normalizeh(direction);
        create(center, R, color);
    }

    void create(vec4 _center, float _R, vec3 _color){
        body = new Circle(vec2(_center.x, _center.y), _R, _color);
        direction = normalizeh(direction);
        eye1= new Circle(pointAtTime(_center, normalizeh(rotateVector(center,direction, -0.55f)), _R), _R / 4, vec3(1, 1, 1));
        eye2= new Circle(pointAtTime(_center, normalizeh(rotateVector(center,direction, 0.55f)), _R), _R / 4, vec3(1, 1, 1));
        //eye1center = new Circle(pointAtTime(_center, rotateVector(_center, direction, -0.2f), _R), _R / 8, vec3(0, 0, 0));
        //eye2center = new Circle(pointAtTime(_center, rotateVector(_center, direction, 0.2f), _R), _R / 8, vec3(0, 0, 0));
        mouth = new Circle(pointAtTime(_center, direction, _R), mouthR, vec3(0, 0, 0));
    }

    void draw(){
        this->mouthSizeChange();
        this->drawGoo();
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        body->draw();
        mouth->draw();
        eye1->draw();
        //eye1center->draw();
        eye2->draw();
        //eye2center->draw();

    }

    void animate(long time){
        delete body;
        delete eye1;
        //delete eye1center;
        delete eye2;
        //delete eye2center;
        delete mouth;
        if (this->turnleft){
            direction = rotateVector(body->center, direction, 0.01f);
            direction = normalizeh(direction);
            //printf("%f %f %f %f\n\n", direction.x, direction.y, direction.z, direction.w);
        }
        if (this->turnright){
            direction = rotateVector(body->center, direction, -0.01f);
            direction = normalizeh(direction);
            //printf("%f %f %f %f\n\n", direction.x, direction.y, direction.z, direction.w);
        }
        if (this->moveforward){
            center = pointAtTime(center, direction, time * 0.001f);
            direction = velocityAtTime(center, direction, time * 0.001f);
            direction = doth(center,direction) * center + direction;
            direction = normalizeh(direction);
            //center.w = sqrtf(center.x * center.x + center.y * center.y + 1.0f);
            //printf("%f %f %f %f\n\n", center.x, center.y, center.z, center.w);
        }
        goo.push_back(vec4(center.x, center.y, 0, sqrtf(center.x * center.x + center.y * center.y + 1.0f)));
        mouthSizeChange();
        create(center, R, color);
        glutPostRedisplay();
    }

    void animateCircle(long time){
        delete body;
        delete eye1;
        //delete eye1center;
        delete eye2;
        //delete eye2center;
        delete mouth;
        direction = normalizeh(direction);
        direction = rotateVector(center, direction, 0.0009*time);
        direction = normalizeh(direction);
        center = pointAtTime(center, direction, time / 5000.0f);
        direction = velocityAtTime(center, direction, time / 5000.0f);
        direction = doth(center, direction) * center + direction;
        direction = normalizeh(direction);


        //direction = velocityAtTime(center, center, time * 0.0000005f);
        //direction = doth(center, direction) * center + direction;
        //normalizeh(direction);
        //center.w = sqrtf(center.x * center.x + center.y * center.y + 1.0f);
        //printf("%f %f %f %f\n\n", center.x, center.y, center.z, center.w);
        goo.push_back(vec4(center.x, center.y, 0, sqrtf(center.x * center.x + center.y * center.y + 1.0f)));
        mouthSizeChange();
        create(center, R, color);
        glutPostRedisplay();
    }


    void drawGoo(){
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, goo.size() * sizeof(vec4), &goo[0], GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
        glUniform3f(glGetUniformLocation(gpuProgram.getId(), "color"), 1, 1, 1);
        glLineWidth(5);
        glDrawArrays(GL_LINE_STRIP, 0, goo.size());
    }

    void mouthSizeChange(){
        if (mouthSmaller) {
            mouthR -= 0.0001f;
            if (mouthR <= R / 10) {
                mouthSmaller = false;
            }
        }
        else {
            mouthR += 0.0001f;
            if (mouthR >= R / 3) {
                mouthSmaller = true;
            }
        }
    }

};

Hami* hami1;
Hami* hami2;
Circle* PoincareDisk;

// Initialization, create an OpenGL context
void onInitialization() {
    glViewport(0, 0, windowWidth, windowHeight);
    hami1 = new Hami(vec2(0, 0), 0.3f, vec3(1, 0, 0));
    hami2 = new Hami(vec2(0.6f, 0.8), 0.3f, vec3(0, 1, 0));
    PoincareDisk = new Circle(vec2(0, 0), 5.4f, vec3(0, 0, 0));
    // create program for the GPU
    gpuProgram.create(vertexSource, fragmentSource, "outColor");
}


// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0.5f, 0.5f, 0.5f, 0);     // background color
    glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

    PoincareDisk->draw();
    hami2->draw();
    hami1->draw();

    glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
    if (key == 'e') hami1->moveforward = true;         // if d, invalidate display, i.e. redraw
    if (key == 'f') hami1->turnleft = true;
    if (key == 's') hami1->turnright = true;
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
    if (key == 'e') hami1->moveforward = false;         // if d, invalidate display, i.e. redraw
    if (key == 'f') hami1->turnleft = false;
    if (key == 's') hami1->turnright = false;
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system

}

// Idle event indicating that some time elapsed: do animation here
long prevTime = 0;
long difference;
void onIdle() {
    long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
    /*if (time-prevTime > 0){
        difference = time - prevTime;
    }
    printf("%ld\n\n", difference);
    for (int i = 0; i < difference; ++i) {
        hami1->animate(difference);
        hami2->animateCircle(difference);
    }
//    prevTime = time;*/
    hami1->animate(time-prevTime);
    hami2->animateCircle(time-prevTime);
    glutPostRedisplay();
    prevTime = time;
}