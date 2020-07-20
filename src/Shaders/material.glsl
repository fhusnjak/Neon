struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 transmittance;
    vec3 emission;
    float shininess;
    float ior;// index of refraction
    float dissolve;// 1 == opaque; 0 == fully transparent
    int illum;
    int textureId;
};

struct Vertex
{
    vec3 pos;
    vec3 norm;
    vec3 color;
    vec2 texCoord;
    int matID;
};

struct Instance
{
    int objId;
    mat4 modelMatrix;
    mat4 modelMatrixIT;
    int texOffset;
};

vec3 computeDiffuse(Material mat, vec3 lightDir, vec3 normal)
{
    float dotNL = max(dot(normal, lightDir), 0.0);
    vec3 c = mat.diffuse * max(dotNL, 0.0);
    return mat.ambient + c;
}

const float PI = 3.14159265;

vec3 computeSpecular(Material mat, vec3 viewDir, vec3 lightDir, vec3 normal)
{
    const float kShininess = max(mat.shininess, 4.0);
    const float kEnergyConservation = (2.0 + kShininess) / (2.0 * PI);
    vec3 halfway = normalize(lightDir + viewDir);
    float specular = kEnergyConservation * pow(max(dot(normal, halfway), 0.0), kShininess);
    return vec3(mat.specular * specular);
}
