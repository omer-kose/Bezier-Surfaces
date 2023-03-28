#version 410 core


vec3 Iamb = vec3(0.8, 0.8, 0.8); // ambient light intensity
vec3 ka = vec3(0.3, 0.3, 0.3);   // ambient reflectance coefficient
vec3 kd = vec3(0.8, 0.8, 0.8);     // diffuse reflectance coefficient
vec3 ks = vec3(0.8, 0.8, 0.8);   // specular reflectance coefficient
int phongExponent = 400;

uniform vec3 eyePos;
//At most 5 lights we will have. I wont create a struct for this homework
//I will directly pass position and intensities as seperate arrays
uniform vec3 lightPositions[5];
uniform vec3 lightIntensities[5];
uniform int numLights; //Actual number of lights in the scene


out vec4 FragColor;


//Ins
in vec4 fragWorldPos;
in vec3 fragWorldNor;

vec3 computeLightColor(int lightIndex)
{
    // Compute lighting. We assume lightPos and eyePos are in world
    // coordinates. fragWorldPos and fragWorldNor are the interpolated
    // coordinates by the rasterizer.
    vec3 lightPos = lightPositions[lightIndex];
    vec3 L = normalize(lightPos - vec3(fragWorldPos));
    vec3 V = normalize(eyePos - vec3(fragWorldPos));
    vec3 H = normalize(L + V);
    vec3 N = normalize(fragWorldNor);

    float NdotL = dot(N, L); // for diffuse component
    float NdotH = dot(N, H); // for specular component
    
    vec3 I = lightIntensities[lightIndex];
    vec3 diffuseColor = I * kd * max(0, NdotL);
    vec3 specularColor = I * ks * pow(max(0, NdotH), phongExponent);

    float distToLightSq = dot(lightPos - vec3(fragWorldPos), lightPos - vec3(fragWorldPos));
    return (diffuseColor + specularColor) / distToLightSq;
}

void main()
{
    vec3 ambientColor = Iamb * ka;
    //Loop over every light and accumulate the color
    vec3 c = vec3(0.0f);
    for(int i = 0; i < numLights; ++i)
    {
        c += computeLightColor(i);
    }
    
    FragColor = vec4(c + ambientColor, 1.0);
}

