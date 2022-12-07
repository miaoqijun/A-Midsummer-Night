#version 330 core
layout(points) in;                                                                  
layout(triangle_strip) out;                                                         
layout(max_vertices = 4) out;                                                       
                                                                                    
uniform mat4 projection; 
uniform mat4 view; 
uniform mat4 model; 
uniform vec3 gCameraPos;

in GS_OUT
{
    vec4 ourColor;
    float ourScale;
    float ourChoice;
} gs_in[];

out vec4 fColor;
out vec2 TexCoord;
out float fchoice;

void main()                                                                         
{                                                                                   
    vec3 Pos = gl_in[0].gl_Position.xyz;                                            
    vec3 toCamera = normalize(gCameraPos - Pos);                                    
    vec3 up = vec3(0.0, 1.0, 0.0);                                                  
    vec3 right = cross(toCamera, up) * gs_in[0].ourScale;                              
    
    fColor = gs_in[0].ourColor; 
    fchoice = gs_in[0].ourChoice;
    Pos -= right;                                                                   
    gl_Position = projection * view * model * vec4(Pos, 1.0);                                             
    TexCoord = vec2(0.0, 0.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    Pos.y += gs_in[0].ourScale;                                                        
    gl_Position = projection * view * model * vec4(Pos, 1.0);                                             
    TexCoord = vec2(0.0, 1.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    Pos.y -= gs_in[0].ourScale;                                                        
    Pos += right;                                                                   
    gl_Position = projection * view * model * vec4(Pos, 1.0);                                             
    TexCoord = vec2(1.0, 0.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    Pos.y += gs_in[0].ourScale;                                                        
    gl_Position = projection * view * model * vec4(Pos, 1.0);                                             
    TexCoord = vec2(1.0, 1.0);                                                      
    EmitVertex();                                                                   
                                                                                    
    EndPrimitive();   
}               