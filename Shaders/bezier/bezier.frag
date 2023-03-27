#version 410 core

out vec4 FragColor;

uniform int i;


//Ins
in vec4 fragWorldPos;
in vec3 fragWorldNor;

void main()
{
    FragColor = vec4(0.7, 0.7, 0.7, 1.0);
    FragColor = vec4(1.0);
    FragColor = vec4(i*0.2, i*0.2, i*0.2, 1.0);
    FragColor = vec4(vec3(fragWorldNor.x), 1.0);
}

