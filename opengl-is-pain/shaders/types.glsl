// #version 410 core

struct LightIncidence
{
	vec3 lightDir; 
 	vec3 vNormal;  	
 	vec3 vViewPosition; 
};

struct LightAttributes
{
   // Light values
   vec3 ambient;
   vec3 diffuse;
   vec3 specular;

   // Multipliers 
   float kA;
   float kD;
   float kS;
};

struct PointLight
{
   vec3 position;
   
   LightAttributes lightAttrs;
};

struct DirectionalLight
{
   vec3 direction;
   
   LightAttributes lightAttrs;
};

struct SpotLight
{
   vec3 position;
   vec3 direction;
   float cutoffAngle;
   
   LightAttributes lightAttrs;
};