#version 410 core
layout (location = 0) in vec2 uv_in;


//Prestore n_choose_i's as we know them already.
int n_choose_i[4] = int[4](1, 3, 3, 1);

//Uniforms
uniform mat4 PVM;
uniform vec3 P[16]; //Control points. Layout is row major.


float bern(int i, float t)
{
    return n_choose_i[i] * pow(t, i) * pow(1-t, 3-i);
}


vec3 eval_bezier()
{
    float u = uv_in[0];
    float v = uv_in[1];
    //Go over the Control Points and compute Bernstein Polynomials
    vec3 p = vec3(0.0); //Must initalize, otherwise it does not work lol
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 4; ++j)
        {
            p += bern(i, v) * bern(j, u) * P[4*i + j];
        }
    }
    
    return p;
}

void main()
{
    vec3 p = eval_bezier();
    //p = vec3(uv_in, 0.0);
    gl_Position = PVM * vec4(p, 1.0);
}
