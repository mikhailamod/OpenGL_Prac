#version 330 core

in vec3 fragmentColor;
// Output data
out vec3 objectColor;

void main()
{
	// Output color = red
	objectColor = fragmentColor;
}
