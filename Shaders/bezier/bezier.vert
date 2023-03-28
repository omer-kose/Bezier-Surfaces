#version 410 core
layout (location = 0) in vec2 uv_in;


//Prestore n_choose_i's as we know them already.
//int n_choose_i[4] = int[4](1, 3, 3, 1); //This initialization did not work on Windows Machine I tried
int n_choose_i[4]; //Just declare here for compatibility

//Uniforms
uniform mat4 modelMat;
uniform mat4 PV;
uniform vec3 P[16]; //Control points. Layout is row major.

//Outs
out vec4 fragWorldPos;
out vec3 fragWorldNor;


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
    for(int i = 0; i < 4; ++i) //along v
    {
        for(int j = 0; j < 4; ++j) //along u
        {
            p += bern(i, v) * bern(j, u) * P[4*i + j];
        }
    }
    
    return p;
}

vec3 eval_dU()
{
    float u = uv_in[0];
    float v = uv_in[1];
    vec3 vCurve[4]; //v curve values
    //Evaluate the constant scalars of the dU's
    for(int j = 0; j < 4; ++j)
    {
        //Evaluate the curve along P
        vec3 p = vec3(0.0f);
        for(int i = 0; i < 4; ++i)
        {
            p += bern(i, v) * P[4*i + j];     
        }

        vCurve[j] = p;
    }

    //Compute dU:
    return -3 * (1 - u) * (1 - u) * vCurve[0] + 
    (3 * (1 - u) * (1 - u) - 6 * u * (1 - u)) * vCurve[1] + 
    (6 * u * (1 - u) - 3 * u * u) * vCurve[2] + 
    3 * u * u * vCurve[3]; 
}


vec3 eval_dV()
{
    float u = uv_in[0];
    float v = uv_in[1];
    vec3 uCurve[4]; //u curve values
    //Evaluate the constant scalars of the dV's
    for(int i = 0; i < 4; ++i)
    {
        //Evaluate the curve along P
        vec3 p = vec3(0.0f);
        for(int j = 0; j < 4; ++j)
        {
            p += bern(j, u) * P[4*i + j];
        }

        uCurve[i] = p;
    }

    return -3 * (1 - v) * (1 - v) * uCurve[0] + 
    (3 * (1 - v) * (1 - v) - 6 * v * (1 - v)) * uCurve[1] + 
    (6 * v * (1 - v) - 3 * v * v) * uCurve[2] + 
    3 * v * v * uCurve[3]; 

}

void main()
{
    //Initialize the global array 
    n_choose_i[0] = 1;
    n_choose_i[1] = 3;
    n_choose_i[2] = 3;
    n_choose_i[3] = 1;

    vec3 p = eval_bezier();
    //p = vec3(uv_in-0.5, 0.0);
    vec3 dU = eval_dU();
    vec3 dV = eval_dV();
    vec3 n = normalize(cross(dV, dU));

    fragWorldPos = modelMat * vec4(p, 1.0);
	fragWorldNor = inverse(transpose(mat3x3(modelMat))) * n;

    gl_Position = PV * modelMat * vec4(p, 1.0);
}
