#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

out vec4 fColor;

// --- MATRICES ---
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

// --- LIGHTING ---
// 1. Directional Light (Sun)
uniform vec3 lightDir;
uniform vec3 lightColor;

// 2. Point Light (Superlaser)
uniform vec3 pointLightPos;   // Position of the beam tip
uniform vec3 pointLightColor; // Color (Green)

// --- TEXTURES ---
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

// --- GLOBAL VARIABLES (To hold summed light) ---
vec3 ambientTotal;
vec3 diffuseTotal;
vec3 specularTotal;

// Constants
float ambientStrength = 0.2f;
float specularStrength = 0.5f;
float shininess = 32.0f;

void computeDirLight(vec4 fPosEye, vec3 normalEye, vec3 viewDir, vec3 texDiffuse, vec3 texSpecular)
{
    // 1. Direction (Sunlight is parallel, so we use fixed direction)
    // Convert lightDir to Eye Space
    vec3 lightDirN = normalize(vec3(view * vec4(lightDir, 0.0f)));

    // 2. Ambient
    ambientTotal += ambientStrength * lightColor * texDiffuse;

    // 3. Diffuse
    float diff = max(dot(normalEye, lightDirN), 0.0f);
    diffuseTotal += diff * lightColor * texDiffuse;

    // 4. Specular
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);
    specularTotal += specularStrength * spec * lightColor * texSpecular;
}

void computePointLight(vec4 fPosEye, vec3 normalEye, vec3 viewDir, vec3 texDiffuse, vec3 texSpecular)
{
    // 1. Position & Direction
    // Convert Point Light Position to Eye Space
    vec3 lightPosEye = vec3(view * vec4(pointLightPos, 1.0f));
    vec3 lightDirP = normalize(lightPosEye - fPosEye.xyz);
    float distance = length(lightPosEye - fPosEye.xyz);

    // 2. Attenuation (How fast light fades)
    // For a massive space laser, we want it to reach far. 
    // We use small values so it doesn't fade too quickly.
    float constant = 1.0f;
    float linear = 0.00045f; 
    float quadratic = 0.000075f;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

    // 3. Diffuse (Green)
    float diff = max(dot(normalEye, lightDirP), 0.0f);
    diffuseTotal += diff * pointLightColor * attenuation * texDiffuse;

    // 4. Specular (Green Reflection)
    vec3 reflectDir = reflect(-lightDirP, normalEye);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);
    specularTotal += specularStrength * spec * pointLightColor * attenuation * texSpecular;
}

void main() 
{
    // Initialize totals
    ambientTotal = vec3(0.0);
    diffuseTotal = vec3(0.0);
    specularTotal = vec3(0.0);

    // --- 1. PRE-CALCULATIONS (Eye Space) ---
    // We compute these ONCE here so both lights can use them
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 viewDir = normalize(-fPosEye.xyz); // Viewer is at (0,0,0) in Eye Space

    // Get Texture Colors
    vec4 texColor = texture(diffuseTexture, fTexCoords);
    vec3 texDiffuse = texColor.rgb;
    vec3 texSpecular = texture(specularTexture, fTexCoords).rgb;

    // --- 2. TRANSPARENCY CHECK ---
    if(texColor.x == 0)
        discard;

    // --- 3. COMPUTE LIGHTS ---
    computeDirLight(fPosEye, normalEye, viewDir, texDiffuse, texSpecular);
    
    // Only calculate Point Light if it has a color (optimization)
    if (length(pointLightColor) > 0.0) {
        computePointLight(fPosEye, normalEye, viewDir, texDiffuse, texSpecular);
    }

    // --- 4. COMBINE ---
    vec3 finalColor = min(ambientTotal + diffuseTotal + specularTotal, 1.0f);

    fColor = vec4(finalColor, 1.0f);
}